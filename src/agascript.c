/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agascript.h>
#include <agalog.h>
#include <agaerr.h>
#include <agaio.h>
#include <agaimg.h>
#include <agadraw.h>
#include <agawin.h>
#include <agastartup.h>
#include <agasnd.h>
#include <agastd.h>

#include <afeirsa/afgl.h>

/* NOTE: This is cursed beyond cursed but old C code do be like that. */
#define main fake_main
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

struct aga_nativeptr {
	OB_HEAD
	void* ptr;
	af_size_t len;
};

static void aga_nativeptr_dealloc(object* _) { (void) _; }

static const typeobject aga_nativeptr_type = {
	OB_HEAD_INIT(&Typetype)
	0,
	"nativeptr", sizeof(struct aga_nativeptr), 0,
	aga_nativeptr_dealloc, 0, 0, 0, 0, 0, 0, 0, 0
};

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

#include "agascriptglue.h"

static enum af_err aga_compilescript(const char* script, object** dict) {
	FILE* f;
	object* module;
	object* result;
	node* node;
	int err;
	codeobject* code;

	if(!(f = fopen(script, "r"))) {
		return aga_af_patherrno(__FILE__, "fopen", script);
	}

	AF_VERIFY((module = add_module("__main__")), AF_ERR_UNKNOWN);

	err = parsefile(f, (char*) script, &gram, file_input, 0, 0, &node);
	AF_VERIFY(err == E_DONE, AF_ERR_UNKNOWN);

	if(fclose(f) == EOF) (void) aga_af_errno(__FILE__, "fclose");

	AF_VERIFY(*dict = getmoduledict(module), AF_ERR_UNKNOWN);
	AF_VERIFY(code = compile(node, (char*) script), AF_ERR_UNKNOWN);

	freetree(node);

	result = eval_code(code, *dict, *dict, 0);
	if(err_occurred()) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	if(result) { DECREF(result); }

	DECREF(code);
	flushline();

	return AF_ERR_NONE;
}

enum af_err aga_mkscripteng(
		struct aga_scripteng* eng, const char* script,
		const char* pypath, int argc, char** argv) {

	object* aga;

    AF_PARAM_CHK(eng);
    AF_PARAM_CHK(script);
    AF_PARAM_CHK(pypath);
    AF_PARAM_CHK(argv);

	initall();
	AF_CHK(aga_mkmod((object**) &eng->agandict));

    aga_log(__FILE__, "Using python path `%s'", pypath);

    setpythonpath((char *) pypath);
	setpythonargv(argc, argv);

	AF_CHK(aga_compilescript(script, (object**) &eng->global));

	if(!(aga = dictlookup(eng->global, "aga"))) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	if(!(aga_dict = getmoduledict(aga))) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	return AF_ERR_NONE;
}

enum af_err aga_killscripteng(struct aga_scripteng* eng) {
	AF_PARAM_CHK(eng);

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

enum af_err aga_setscriptptr(
		struct aga_scripteng* eng, const char* key, void* value) {

	object* nativeptr;

	AF_PARAM_CHK(eng);
	AF_PARAM_CHK(key);
	AF_PARAM_CHK(value);

	if(!(nativeptr = newobject((typeobject*) &aga_nativeptr_type))) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	((struct aga_nativeptr*) nativeptr)->ptr = value;

	if(dictinsert(eng->agandict, (char*) key, nativeptr) == -1) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	return AF_ERR_NONE;
}

enum af_err aga_findclass(
		struct aga_scripteng* eng, struct aga_scriptclass* class,
		const char* name) {

	AF_PARAM_CHK(eng);
	AF_PARAM_CHK(class);
	AF_PARAM_CHK(name);

	if(!(class->class = dictlookup(eng->global, (char*) name))) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	if(!is_classobject((object*) class->class)) return AF_ERR_UNKNOWN;

	return AF_ERR_NONE;
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

	call_function(methodcall, 0);
	if(err_occurred()) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	DECREF(methodcall);

	return AF_ERR_NONE;
}
