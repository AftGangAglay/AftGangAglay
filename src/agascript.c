/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agascript.h>
#include <agascripthelp.h>
#include <agalog.h>
#include <agaerr.h>
#include <agapack.h>
#include <agapyinc.h>

#include <agan/agan.h>

#include <python/graminit.h>

#include <apro.h>

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

static enum aga_result aga_compilescript(
		const char* script, struct aga_respack* pack,
		struct py_object** dict) {

	enum aga_result result;
	void* fp;
	aga_size_t size;
	struct py_object* module;
	struct py_object* eval;
	struct py_node* node;
	int res;
	struct py_code* code;

	result = aga_resfptr(pack, script, &fp, &size);
	if(result) return result;

	if(!(module = py_module_add("__main__"))) return AGA_RESULT_ERROR;

	res = py_parse_file(
			fp, script, &py_grammar, PY_GRAMMAR_FILE_INPUT, 0, 0, &node);
	if(res != PY_RESULT_DONE) return AGA_RESULT_ERROR;

	if(!(*dict = ((struct py_module*) module)->attr)) return AGA_RESULT_ERROR;
	if(!(code = py_compile(node, script))) return AGA_RESULT_ERROR;

	py_tree_delete(node);

	eval = py_code_eval(code, *dict, *dict, 0);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	py_object_decref(eval);
	py_object_decref(code);

	return AGA_RESULT_OK;
}

enum aga_result aga_mkscripteng(
		struct aga_scripteng* eng, const char* script, struct aga_respack* pack,
		const char* pypath) {

	enum aga_result result;

	if(!eng) return AGA_RESULT_BAD_PARAM;
	if(!script) return AGA_RESULT_BAD_PARAM;

	/* TODO: EH */
	py_import_init();
	py_builtin_init();
	py_math_init();

	result = aga_mkmod((void**) &eng->agandict);
	if(result) return result;

	if(!(py_path = py_path_new(pypath))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	result = aga_setscriptptr(eng, AGA_SCRIPT_PACK, pack);
	if(result) return result;

	result = aga_compilescript(
			script, pack, (struct py_object**) &eng->global);
	if(result) return result;

	return AGA_RESULT_OK;
}

enum aga_result aga_killscripteng(struct aga_scripteng* eng) {
	if(!eng) return AGA_RESULT_BAD_PARAM;

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

	struct py_object* v;

	if(!eng) return AGA_RESULT_BAD_PARAM;
	if(!key) return AGA_RESULT_BAD_PARAM;

	if(!(v = aga_script_mkptr(value))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	if(py_dict_insert(eng->agandict, key, v) == -1) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_findclass(
		struct aga_scripteng* eng, struct aga_scriptclass* class,
		const char* name) {

	struct py_object* cl;

	if(!eng) return AGA_RESULT_BAD_PARAM;
	if(!class) return AGA_RESULT_BAD_PARAM;
	if(!name) return AGA_RESULT_BAD_PARAM;

	if(!(class->class = py_dict_lookup(eng->global, name))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	cl = class->class;
	if(cl->type != PY_TYPE_CLASS) return AGA_RESULT_ERROR;

	return AGA_RESULT_OK;
}

enum aga_result aga_mkscriptinst(
		struct aga_scriptclass* class, struct aga_scriptinst* inst) {

	if(!class) return AGA_RESULT_BAD_PARAM;
	if(!inst) return AGA_RESULT_BAD_PARAM;

	inst->class = class;
	inst->object = py_class_member_new(class->class);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_killscriptinst(struct aga_scriptinst* inst) {
	if(!inst) return AGA_RESULT_BAD_PARAM;

	py_object_decref((struct py_object*) inst->object);

	return AGA_RESULT_OK;
}

enum aga_result aga_instcall(struct aga_scriptinst* inst, const char* name) {
	struct py_object* proc;
	struct py_object* methodcall;

	if(!inst) return AGA_RESULT_BAD_PARAM;
	if(!name) return AGA_RESULT_BAD_PARAM;

	apro_stamp_start(APRO_SCRIPT_INSTCALL_RISING);

	proc = py_class_get_attr(inst->class->class, name);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	methodcall = py_class_method_new(proc, inst->object);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	apro_stamp_end(APRO_SCRIPT_INSTCALL_RISING);

	apro_stamp_start(APRO_SCRIPT_INSTCALL_EXEC);

	py_call_function(methodcall, 0);
	if(py_error_occurred()) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	apro_stamp_end(APRO_SCRIPT_INSTCALL_EXEC);

	py_object_decref(methodcall);

	return AGA_RESULT_OK;
}
