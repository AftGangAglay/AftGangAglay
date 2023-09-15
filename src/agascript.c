/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <afeirsa/afeirsa.h>

#include <agacore.h>
#include <agascript.h>

/* NOTE: This is cursed beyond cursed but old C code do be like that. */
#define main __attribute__((weak)) main
#include <pythonmain.c>
#include <config.c>
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

static void aga_scriptchk(void) {
	object* exc;
	object* val;

	if(err_occurred()) {
		err_get(&exc, &val);
		fputs("python: ", stderr);
		printobject(exc, stderr, PRINT_RAW);
		fputs(": ", stderr);
		printobject(val, stderr, PRINT_RAW);
		putc('\n', stderr);
		printtraceback(stderr);
		aga_fatal("");
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
		struct aga_ctx* ctx, const char* script, const char* pypath) {

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(script);
	AF_PARAM_CHK(pypath);

	script_ctx = ctx;

	initall();
	AF_CHK(aga_mkmod());

	setpythonpath((char*) pypath); /* libpython predates `const' */
	setpythonargv(0, 0);

	{
		FILE* f;

		object* module;
		object* dict;
		object* result;
		node* node;
		int err;

		codeobject* code;

		if(!(f = fopen(script, "r"))) aga_errno_chk("fopen");

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
		aga_scriptchk();
		DECREF(result);

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
			ctx->scripteng.classes = calloc(len, sizeof(struct aga_scriptclass));
			AF_VERIFY(ctx->scripteng.classes, AF_ERR_MEM);

			ctx->scripteng.len = 0;

			for(i = 0; i < len; ++i) {
				object* key;
				object* value;
				char* key_name;

				AF_VERIFY(key = getlistitem(keys, i), AF_ERR_UNKNOWN);
				AF_VERIFY(key_name = getstringvalue(key), AF_ERR_UNKNOWN);

				AF_VERIFY(value = dictlookup(dict, key_name), AF_ERR_UNKNOWN);

				if(is_classobject(value)) {
					struct aga_scriptclass* class =
						&ctx->scripteng.classes[ctx->scripteng.len++];
					af_size_t key_len = strlen(key_name);

					class->class = value;

					class->name = malloc(key_len + 1);
					AF_VERIFY(class->name, AF_ERR_MEM);
					af_memcpy(class->name, key_name, key_len + 1);
				}
			}

			DECREF(keys);
		}

		if(fclose(f) == EOF) aga_errno_chk("fclose");
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
	aga_scriptchk();

	AF_CHK(aga_instcall(inst, "create"));

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
	aga_scriptchk();

	methodcall = newclassmethodobject(proc, inst->object);
	aga_scriptchk();

	call_function(methodcall, NULL);
	aga_scriptchk();

	DECREF(methodcall);

	return AF_ERR_NONE;
}
