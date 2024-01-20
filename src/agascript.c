/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
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
#include <agapack.h>
#include <agascripthelp.h>

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

typedef object* aga_pyobject_t;

aga_pyobject_t call_function(aga_pyobject_t func, aga_pyobject_t arg);

#ifdef _DEBUG
/* The extra debug info this enables is a bit too verbose. */
int debugging = 0;
#endif

struct aga_nativeptr {
	OB_HEAD
	void* ptr;
	af_size_t len;
};

static void aga_nativeptr_dealloc(aga_pyobject_t _) { (void) _; }

static const typeobject aga_nativeptr_type = {
	OB_HEAD_INIT(&Typetype)
	0,
	"nativeptr", sizeof(struct aga_nativeptr), 0,
	aga_nativeptr_dealloc, 0, 0, 0, 0, 0, 0, 0, 0
};

#define is_nativeptrobject(o) ((o)->ob_type == &aga_nativeptr_type)

static aga_pyobject_t newnativeptrobject(void) {
	return (aga_pyobject_t)
		NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
}

static void aga_scripttrace(void) {
	aga_pyobject_t exc;
	aga_pyobject_t val;

	if(err_occurred()) {
		af_size_t i;
		aga_pyobject_t v = tb_fetch();
		if(!v) return;

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
static void aga_scripterrf(const char* fmt, ...) {
	va_list l;
	aga_fixed_buf_t buf = { 0 };

	va_start(l, fmt);

	if(vsprintf(buf, fmt, l) < 0) {
		aga_af_errno(__FILE__, "vsprintf");
		err_setstr(RuntimeError, (char*) fmt);
	}
	else err_setstr(RuntimeError, buf);

	va_end(l);
}
*/

/*
enum af_err aga_addscriptmod(struct aga_res* res) {
	void* fp;

	aga_pyobject_t d, m, result;
	node* n;
	int err;
	char* name;
	af_size_t len = af_strlen(res->conf->name);

	AF_PARAM_CHK(res);

	AF_CHK(aga_resseek(res, &fp));
	AF_VERIFY(name = calloc(len - 2, 1), AF_ERR_MEM);
	af_memcpy(name, res->conf->name, len - 2);

	err = parse_file(fp, res->conf->name, file_input, &n);
	if(err != E_DONE) {
		err_input(err);
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	if(!(m = add_module(name))) {
		freetree(n);
		return AF_ERR_UNKNOWN;
	}

	if(!(d = getmoduledict(m))) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	result = run_node(n, name, d, d);
	if(err_occurred()) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	if(result) { DECREF(result); }

	return AF_ERR_NONE;
}
*/

static af_bool_t aga_script_aferr(const char* proc, enum af_err err) {
	aga_fixed_buf_t buf = { 0 };

	if(!err) return AF_FALSE;

	if(sprintf(buf, "%s: %s", proc, aga_af_errname(err)) < 0) {
		aga_af_errno(__FILE__, "sprintf");
		return AF_TRUE;
	}
	err_setstr(RuntimeError, buf);

	return AF_TRUE;
}

static af_bool_t aga_script_glerr(const char* proc) {
	aga_fixed_buf_t buf = { 0 };
	af_uint_t err;
	af_uint_t tmp = glGetError();
	const char* s;
	if(!tmp) return AF_FALSE;

	do {
		err = tmp;
		s = (const char*) gluErrorString(err);
		aga_log(__FILE__, "err: %s: %s", proc, s);
	} while((tmp = glGetError()));

	if(sprintf(buf, "%s: %s", proc, s) < 0) {
		aga_af_errno(__FILE__, "sprintf");
		return AF_TRUE;
	}
	err_setstr(RuntimeError, buf);

	return AF_TRUE;
}

/*
 * NOTE: We need a bit of global state here to get engine system contexts etc.
 * 		 Into script land because this version of Python's state is spread
 * 		 Across every continent.
 */
static aga_pyobject_t agan_dict;

static void* aga_getscriptptr(const char* key) {
	aga_pyobject_t ptr;

	if(!key) {
		err_setstr(RuntimeError, "unexpected null pointer");
		return 0;
	}

	ptr = dictlookup(agan_dict, (char*) key);
	if(!ptr) {
		err_setstr(RuntimeError, "failed to resolve script nativeptr");
		return 0;
	}

	if(ptr->ob_type != &aga_nativeptr_type) {
		err_badarg();
		return 0;
	}

	return ((struct aga_nativeptr*) ptr)->ptr;
}

FILE* pyopen_r(const char* path) {
	struct aga_respack* pack;
	af_size_t i;

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;

	for(i = 0; i < pack->len; ++i) {
		struct aga_res* res = &pack->db[i];
		if(af_streql(path, res->conf->name)) {
			enum af_err result;
			void* fp;

			result = aga_resseek(res, &fp);
			if(aga_script_aferr("aga_resseek", result)) return 0;

			return fp;
		}
	}

	return fopen(path, "rb");
}

void pyclose(FILE* fp) {
	struct aga_respack* pack;

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return;
	if(fp == pack->fp) return;

	fclose(fp);
}

#include "agascriptglue.h"

static enum af_err aga_compilescript(
		const char* script, struct aga_respack* pack, aga_pyobject_t* dict) {

	void* fp;
	af_size_t size;
	aga_pyobject_t module, result;
	node* node;
	int err;
	codeobject* code;

	AF_CHK(aga_resfptr(pack, script, &fp, &size));

	AF_VERIFY((module = add_module("__main__")), AF_ERR_UNKNOWN);

	err = parsefile(fp, (char*) script, &gram, file_input, 0, 0, &node);
	AF_VERIFY(err == E_DONE, AF_ERR_UNKNOWN);

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
		struct aga_scripteng* eng, const char* script, int argc,
		char** argv, struct aga_respack* pack, const char* pypath) {

    AF_PARAM_CHK(eng);
    AF_PARAM_CHK(script);
    AF_PARAM_CHK(argv);

	initall();
	AF_CHK(aga_mkmod((aga_pyobject_t*) &eng->agandict));

	setpythonpath((char*) pypath);
	setpythonargv(argc, argv);

	AF_CHK(aga_setscriptptr(eng, AGA_SCRIPT_PACK, pack));
	AF_CHK(aga_compilescript(script, pack, (aga_pyobject_t*) &eng->global));

	return AF_ERR_NONE;
}

enum af_err aga_killscripteng(struct aga_scripteng* eng) {
	AF_PARAM_CHK(eng);

	flushline();
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

	aga_pyobject_t nativeptr;

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

	if(!is_classobject((aga_pyobject_t) class->class)) return AF_ERR_UNKNOWN;

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

	DECREF((aga_pyobject_t) inst->object);

	return AF_ERR_NONE;
}

enum af_err aga_instcall(struct aga_scriptinst* inst, const char* name) {
	aga_pyobject_t proc;
	aga_pyobject_t methodcall;

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
