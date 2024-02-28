/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agascript.h>
#include <agalog.h>
#include <agaerr.h>
#include <agapack.h>
#include <agapyinc.h>

#include <graminit.h> /* For `file_input'. */

#include <agan/agan.h>

void aga_script_trace(void) {
	aga_pyobject_t exc;
	aga_pyobject_t val;

	if(err_occurred()) {
		aga_size_t i;
		aga_pyobject_t v = tb_fetch();
		if(!v) return;

		err_get(&exc, &val);
		for(i = 0; i < aga_logctx.len; ++i) {
			FILE* s = aga_logctx.targets[i];
			aga_loghdr(s, __FILE__, AGA_ERR);
			printobject(exc, s, PRINT_RAW);
			fputs(": ", s);
			printobject(val, s, PRINT_RAW);
			if(putc('\n', s) == EOF) aga_errno(__FILE__, "putc");
			printtraceback(s);
			if(fprintf(s, "Stack backtrace (innermost last):\n") < 0) {
				aga_errno(__FILE__, "fprintf");
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

aga_bool_t aga_script_err(const char* proc, enum aga_result err) {
	aga_fixed_buf_t buf = { 0 };

	if(!err) return AGA_FALSE;

	if(sprintf(buf, "%s: %s", proc, aga_aga_errname(err)) < 0) {
		aga_errno(__FILE__, "sprintf");
		return AGA_TRUE;
	}
	err_setstr(RuntimeError, buf);

	return AGA_TRUE;
}

void* aga_getscriptptr(const char* key) {
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

	if(ptr->ob_type != &agan_nativeptr_type) {
		err_badarg();
		return 0;
	}

	return ((struct agan_nativeptr*) ptr)->ptr;
}

static enum aga_result aga_compilescript(
		const char* script, struct aga_respack* pack, aga_pyobject_t* dict) {

	void* fp;
	aga_size_t size;
	aga_pyobject_t module, result;
	node* node;
	int err;
	codeobject* code;

	AGA_CHK(aga_resfptr(pack, script, &fp, &size));

	AGA_VERIFY((module = add_module("__main__")), AGA_RESULT_ERROR);

	err = parsefile(fp, (char*) script, &gram, file_input, 0, 0, &node);
	AGA_VERIFY(err == E_DONE, AGA_RESULT_ERROR);

	AGA_VERIFY(*dict = getmoduledict(module), AGA_RESULT_ERROR);
	AGA_VERIFY(code = compile(node, (char*) script), AGA_RESULT_ERROR);

	freetree(node);

	result = eval_code(code, *dict, *dict, 0);
	if(err_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}
	if(result) { DECREF(result); }

	DECREF(code);
	flushline();

	return AGA_RESULT_OK;
}

enum aga_result aga_mkscripteng(
		struct aga_scripteng* eng, const char* script, int argc, char** argv,
		struct aga_respack* pack, const char* pypath) {

	AGA_PARAM_CHK(eng);
	AGA_PARAM_CHK(script);
	AGA_PARAM_CHK(argv);

	initall();
	AGA_CHK(aga_mkmod((void**) &eng->agandict));

	setpythonpath((char*) pypath);
	setpythonargv(argc, argv);

	AGA_CHK(aga_setscriptptr(eng, AGA_SCRIPT_PACK, pack));
	AGA_CHK(aga_compilescript(script, pack, (aga_pyobject_t*) &eng->global));

	return AGA_RESULT_OK;
}

enum aga_result aga_killscripteng(struct aga_scripteng* eng) {
	AGA_PARAM_CHK(eng);

	flushline();
	doneimport();
	donebuiltin();
	donesys();
	donedict();

	err_clear();
	doneerrors();

	freeaccel();

	return AGA_RESULT_OK;
}

enum aga_result aga_setscriptptr(
		struct aga_scripteng* eng, const char* key, void* value) {

	aga_pyobject_t nativeptr;

	AGA_PARAM_CHK(eng);
	AGA_PARAM_CHK(key);
	AGA_PARAM_CHK(value);

	if(!(nativeptr = newobject((typeobject * ) & agan_nativeptr_type))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}
	((struct agan_nativeptr*) nativeptr)->ptr = value;

	if(dictinsert(eng->agandict, (char*) key, nativeptr) == -1) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_findclass(
		struct aga_scripteng* eng, struct aga_scriptclass* class,
		const char* name) {

	AGA_PARAM_CHK(eng);
	AGA_PARAM_CHK(class);
	AGA_PARAM_CHK(name);

	if(!(class->class = dictlookup(eng->global, (char*) name))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	if(!is_classobject((aga_pyobject_t) class->class)) return AGA_RESULT_ERROR;

	return AGA_RESULT_OK;
}

enum aga_result aga_mkscriptinst(
		struct aga_scriptclass* class, struct aga_scriptinst* inst) {

	AGA_PARAM_CHK(class);
	AGA_PARAM_CHK(inst);

	inst->class = class;
	inst->object = newclassmemberobject(class->class);
	if(err_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_killscriptinst(struct aga_scriptinst* inst) {
	AGA_PARAM_CHK(inst);

	DECREF((aga_pyobject_t) inst->object);

	return AGA_RESULT_OK;
}

enum aga_result aga_instcall(struct aga_scriptinst* inst, const char* name) {
	aga_pyobject_t proc;
	aga_pyobject_t methodcall;

	AGA_PARAM_CHK(inst);
	AGA_PARAM_CHK(name);

	proc = getattr(inst->class->class, (char*) name);
	if(err_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	methodcall = newclassmethodobject(proc, inst->object);
	if(err_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	call_function(methodcall, 0);
	if(err_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	DECREF(methodcall);

	return AGA_RESULT_OK;
}
