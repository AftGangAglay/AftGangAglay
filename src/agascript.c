/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agascript.h>
#include <agalog.h>
#include <agaio.h>
#include <agaimg.h>
#include <agastd.h>
#include <agadraw.h>

#include <afeirsa/afeirsa.h>
#include <afeirsa/afgl.h>

/* NOTE: This is cursed beyond cursed but old C code do be like that. */
#define main __attribute__((weak)) main
#include <pythonmain.c>
#include <config.c>
#include <traceback.h>
#include <modsupport.h>
#undef main

int setpythonpath(char* path);
int setpythonargv(int argc, char** argv);
int flushline(void);
void donebuiltin(void);
void donesys(void);
void donedict(void);
void doneerrors(void);
void freeaccel(void);

object* call_function(object* func, object* arg);

#ifdef _DEBUG
/* The extra debug info this enables is a bit too verbose. */
int debugging = 0;
#endif

static void aga_scripttrace(void) {
	object* exc;
	object* val;

	if(err_occurred()) {
		af_size_t i;
		object* v = tb_fetch();

		err_get(&exc, &val);
		for(i = 0; i < aga_logctx.len; ++i) {
			FILE* s = aga_logctx.targets[i];
			aga_loghdr(s, __FILE__, AGA_ERR);
			printobject(exc, s, PRINT_RAW);
			fputs(": ", s);
			printobject(val, s, PRINT_RAW);
			if(putc('\n', s) == EOF) aga_af_errno(__FILE__, "putc");
			printtraceback(s);
			if(fprintf(s, "Stack backtrace (innermost last):\n") < 0) {
				aga_af_errno(__FILE__, "fprintf");
			}
			if(tb_print(v, s) == -1) {
				/*
				 * We obviously don't want to traceback here to avoid endless
				 * loops.
				 */
				aga_log(__FILE__, "err: tb_print() failed");
			}
		}

		DECREF(v);
	}
}

/*
 * NOTE: This exists like this to avoid needing another mess of script
 * 		 Includes and to reduce the impact of the less-than-desirable global
 * 		 Context state here.
 */
static struct aga_ctx* script_ctx;
#include "agascriptglue.h"

enum af_err aga_mkscripteng(
		struct aga_ctx* ctx, struct aga_scripteng* scripteng,
		const char* script, const char* pypath, int argc, char** argv) {

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(script);
	AF_PARAM_CHK(pypath);

	script_ctx = ctx;

	initall();
	AF_CHK(aga_mkmod());

	{
		char* envpath = getenv("PYTHONPATH");
		if(envpath) {
			af_size_t envpath_len = strlen(envpath);
			af_size_t pypath_len = strlen(pypath);
			af_size_t sz = envpath_len + 1 + pypath_len + 1;
			char* path = malloc(sz);
			if(!path) {
				aga_log(__FILE__, "Using python path %s", pypath);
				setpythonpath((char *) pypath);
			}
			else {
				path[sz - 1] = 0;
				path[envpath_len] = ':';
				af_memcpy(path, envpath, envpath_len);
				af_memcpy(path + envpath_len + 1, pypath, pypath_len);
				aga_log(__FILE__, "Using python path %s", path);
				setpythonpath((char*) path);
				free(path);
			}
		}
		else {
			aga_log(__FILE__, "Using python path %s", pypath);
			setpythonpath((char *) pypath);
		}
	}

	setpythonargv(argc, argv);

	{
		FILE* f;

		object* module;
		object* dict;
		object* result;
		node* node;
		int err;

		codeobject* code;

		if(!(f = fopen(script, "r"))) {
			return aga_af_patherrno(__FILE__, "fopen", script);
		}

		module = add_module("__main__");
		AF_VERIFY(module, AF_ERR_UNKNOWN);

		err = parsefile(f, (char*) script, &gram, file_input, 0, 0, &node);
		AF_VERIFY(err == E_DONE, AF_ERR_UNKNOWN);

		dict = getmoduledict(module);
		AF_VERIFY(dict, AF_ERR_UNKNOWN);

		code = compile(node, (char*) script);
		AF_VERIFY(code, AF_ERR_UNKNOWN);

		freetree(node);

		result = eval_code(code, dict, dict, 0);
		if(err_occurred()) {
			aga_scripttrace();
			return AF_ERR_UNKNOWN;
		}
		if(result) { DECREF(result); }

		scripteng->global = dict;

		/* TODO: I don't like hardcoding scriptglue into base agascript */
		{
			object* aga = dictlookup(dict, "aga");
			object* agadict;
			if(!aga) {
				aga_scripttrace();
				return AF_ERR_UNKNOWN;
			}
			if(!(agadict = getmoduledict(aga))) {
				aga_scripttrace();
				return AF_ERR_UNKNOWN;
			}
			if(!(ctx->transform_class = dictlookup(agadict, "transform"))) {
				aga_scripttrace();
				return AF_ERR_UNKNOWN;
			}
		}

		DECREF(code);
		flushline();

		{
			object* keys;
			int len;
			int i;

			AF_VERIFY(keys = getdictkeys(dict), AF_ERR_UNKNOWN);
			len = getlistsize(keys);

			/*
			 * NOTE: This is slightly wasteful but avoids a load of `realloc's.
			 *		 Most scripts are probably pretty light on non-class
			 *		 globals anyway.
			 */
			scripteng->classes = calloc(len, sizeof(struct aga_scriptclass));
			AF_VERIFY(scripteng->classes, AF_ERR_MEM);

			scripteng->len = 0;

			for(i = 0; i < len; ++i) {
				object* key;
				object* value;
				char* key_name;

				AF_VERIFY(key = getlistitem(keys, i), AF_ERR_UNKNOWN);
				AF_VERIFY(key_name = getstringvalue(key), AF_ERR_UNKNOWN);

				AF_VERIFY(value = dictlookup(dict, key_name), AF_ERR_UNKNOWN);

				if(is_classobject(value)) {
					struct aga_scriptclass* class =
						&scripteng->classes[scripteng->len++];
					af_size_t key_len = strlen(key_name);

					class->class = value;

					class->name = malloc(key_len + 1);
					AF_VERIFY(class->name, AF_ERR_MEM);
					af_memcpy(class->name, key_name, key_len + 1);
				}
			}

			DECREF(keys);
		}

		if(fclose(f) == EOF) {
			if(err_occurred()) {
				aga_scripttrace();
				return AF_ERR_UNKNOWN;
			}
		}
	}

	return AF_ERR_NONE;
}

enum af_err aga_killscripteng(struct aga_scripteng* eng) {
	af_size_t i;

	AF_PARAM_CHK(eng);

	for(i = 0; i < eng->len; ++i) free(eng->classes[i].name);
	free(eng->classes);

	flushline();
	AF_CHK(aga_killmod());
	doneimport();
	donebuiltin();
	donesys();
	donedict();

	err_clear();
	doneerrors();

	freeaccel();

	return AF_ERR_NONE;
}

enum af_err aga_findclass(
		struct aga_scripteng* eng, struct aga_scriptclass** class,
		const char* name) {

	struct aga_scriptclass* current;

	AF_PARAM_CHK(eng);
	AF_PARAM_CHK(class);

	for(current = eng->classes; current < eng->classes + eng->len; ++current) {
		if(af_streql(current->name, name)) {
			*class = current;
			return AF_ERR_NONE;
		}
	}

	return AF_ERR_BAD_PARAM;
}

enum af_err aga_mkscriptinst(
		struct aga_scriptclass* class, struct aga_scriptinst* inst) {

	AF_PARAM_CHK(class);
	AF_PARAM_CHK(inst);

	inst->class = class;

	inst->object = newclassmemberobject(class->class);
	if(err_occurred()) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	return AF_ERR_NONE;
}

enum af_err aga_killscriptinst(struct aga_scriptinst* inst) {
	AF_PARAM_CHK(inst);

	DECREF((object*) inst->object);

	return AF_ERR_NONE;
}

enum af_err aga_instcall(struct aga_scriptinst* inst, const char* name) {
	object* proc;
	object* methodcall;

	AF_PARAM_CHK(inst);
	AF_PARAM_CHK(name);

	proc = getattr(inst->class->class, (char*) name);
	if(err_occurred()) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	methodcall = newclassmethodobject(proc, inst->object);
	if(err_occurred()) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	call_function(methodcall, NULL);
	if(err_occurred()) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	DECREF(methodcall);

	return AF_ERR_NONE;
}
