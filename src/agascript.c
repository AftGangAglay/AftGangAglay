/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agascript.h>
#include <agalog.h>
#include <agaerr.h>
#include <agapack.h>
#include <agapyinc.h>

#include <agan/agan.h>

void aga_script_trace(void) {
	struct py_object* exc;
	struct py_object* val;

	if(py_error_occurred()) {
		aga_size_t i;
		struct py_object* v = py_traceback_get();
		if(!v) return;

		py_error_get(&exc, &val);
		for(i = 0; i < aga_logctx.len; ++i) {
			FILE* s = aga_logctx.targets[i];

			aga_loghdr(s, __FILE__, AGA_ERR);
			if(exc && fputs(py_string_get(exc), s) == EOF) {
				aga_errno(__FILE__, "fputs");
			}
			if(fputs(": ", s) == EOF) aga_errno(__FILE__, "fputs");
			if(val && fputs(py_string_get(val), s) == EOF) {
				aga_errno(__FILE__, "fputs");
			}
			if(putc('\n', s) == EOF) aga_errno(__FILE__, "putc");

			if(fprintf(s, "Stack backtrace (innermost last):\n") < 0) {
				aga_errno(__FILE__, "fprintf");
			}
			if(py_traceback_print(v, s) == -1) {
				/*
				 * We obviously don't want to traceback here to avoid endless
				 * loops.
				 */
				aga_log(__FILE__, "err: py_traceback_print() failed");
			}
		}

		py_object_decref(v);
	}
}

aga_bool_t aga_script_err(const char* proc, enum aga_result err) {
	aga_fixed_buf_t buf = { 0 };

	if(!err) return AGA_FALSE;

	if(sprintf(buf, "%s: %s", proc, aga_aga_errname(err)) < 0) {
		aga_errno(__FILE__, "sprintf");
		return AGA_TRUE;
	}
	py_error_set_string(py_runtime_error, buf);

	return AGA_TRUE;
}

void* aga_getscriptptr(const char* key) {
	struct py_object* ptr;

	if(!key) {
		py_error_set_string(py_runtime_error, "unexpected null pointer");
		return 0;
	}

	ptr = py_dict_lookup(agan_dict, key);
	if(!ptr) {
		py_error_set_string(
				py_runtime_error, "failed to resolve script nativeptr");
		return 0;
	}

	if(ptr->type != &agan_nativeptr_type) {
		py_error_set_badarg();
		return 0;
	}

	return ((struct agan_nativeptr*) ptr)->ptr;
}

static enum aga_result aga_compilescript(
		const char* script, struct aga_respack* pack, struct py_object** dict) {

	void* fp;
	aga_size_t size;
	struct py_object* module;
	struct py_object* result;
	struct py_node* node;
	int res;
	struct py_code* code;

	AGA_CHK(aga_resfptr(pack, script, &fp, &size));

	AGA_VERIFY((module = py_add_module("__main__")), AGA_RESULT_ERROR);

	res = py_parse_file(
			fp, script, &py_grammar, PY_GRAMMAR_FILE_INPUT, 0, 0, &node);
	AGA_VERIFY(res == PY_RESULT_DONE, AGA_RESULT_ERROR);

	AGA_VERIFY(*dict = py_module_get_dict(module), AGA_RESULT_ERROR);
	AGA_VERIFY(code = py_compile(node, script), AGA_RESULT_ERROR);

	py_tree_delete(node);

	result = py_code_eval(code, *dict, *dict, 0);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}
	if(result) py_object_decref(result);
	py_object_decref(code);

	return AGA_RESULT_OK;
}

enum aga_result aga_mkscripteng(
		struct aga_scripteng* eng, const char* script,
		struct aga_respack* pack, const char* pypath) {

	AGA_PARAM_CHK(eng);
	AGA_PARAM_CHK(script);

	/* TODO: EH */
	py_import_init();
	py_builtin_init();
	py_math_init();

	AGA_CHK(aga_mkmod((void**) &eng->agandict));

	if(!(py_path = py_path_new(pypath))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	AGA_CHK(aga_setscriptptr(eng, AGA_SCRIPT_PACK, pack));
	AGA_CHK(aga_compilescript(script, pack, (struct py_object**) &eng->global));

	return AGA_RESULT_OK;
}

enum aga_result aga_killscripteng(struct aga_scripteng* eng) {
	AGA_PARAM_CHK(eng);

	py_import_done();
	py_builtin_done();
	py_done_dict();

	py_error_clear();
	py_errors_done();

	py_grammar_delete_accels();

	return AGA_RESULT_OK;
}

enum aga_result aga_setscriptptr(
		struct aga_scripteng* eng, const char* key, void* value) {

	struct py_object* nativeptr;

	AGA_PARAM_CHK(eng);
	AGA_PARAM_CHK(key);
	AGA_PARAM_CHK(value);

	if(!(nativeptr = py_object_new((struct py_type*) &agan_nativeptr_type))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}
	((struct agan_nativeptr*) nativeptr)->ptr = value;

	if(py_dict_insert(eng->agandict, key, nativeptr) == -1) {
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

	if(!(class->class = py_dict_lookup(eng->global, name))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	if(!py_is_class((struct py_object*) class->class)) return AGA_RESULT_ERROR;

	return AGA_RESULT_OK;
}

enum aga_result aga_mkscriptinst(
		struct aga_scriptclass* class, struct aga_scriptinst* inst) {

	AGA_PARAM_CHK(class);
	AGA_PARAM_CHK(inst);

	inst->class = class;
	inst->object = py_classmember_new(class->class);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_killscriptinst(struct aga_scriptinst* inst) {
	AGA_PARAM_CHK(inst);

	py_object_decref((struct py_object*) inst->object);

	return AGA_RESULT_OK;
}

enum aga_result aga_instcall(struct aga_scriptinst* inst, const char* name) {
	struct py_object* proc;
	struct py_object* methodcall;

	AGA_PARAM_CHK(inst);
	AGA_PARAM_CHK(name);

	proc = py_class_get_attr(inst->class->class, name);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	methodcall = py_classmethod_new(proc, inst->object);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	py_call_function(methodcall, 0);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	py_object_decref(methodcall);

	return AGA_RESULT_OK;
}
