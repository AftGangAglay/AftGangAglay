/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/script.h>
#include <aga/log.h>
#include <aga/error.h>
#include <aga/utility.h>
#include <aga/pack.h>
#include <aga/python.h>

#include <agan/agan.h>

#include <python/graminit.h>

#include <apro.h>

void aga_script_engine_trace(void) {
	struct py_object* exc;
	struct py_object* val;

	if(py_error_occurred()) {
		aga_size_t i;
		struct py_object* v = py_traceback_get();
		if(!v) return;

		py_error_get(&exc, &val);
		for(i = 0; i < aga_log_context.len; ++i) {
			FILE* s = aga_log_context.targets[i];

			aga_log_header(s, __FILE__, AGA_ERR);
			if(exc && fputs(py_string_get(exc), s) == EOF) {
				aga_error_system(__FILE__, "fputs");
			}
			if(fputs(": ", s) == EOF) aga_error_system(__FILE__, "fputs");
			if(val && fputs(py_string_get(val), s) == EOF) {
				aga_error_system(__FILE__, "fputs");
			}
			if(putc('\n', s) == EOF) aga_error_system(__FILE__, "putc");

			if(fprintf(s, "Stack backtrace (innermost last):\n") < 0) {
				aga_error_system(__FILE__, "fprintf");
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

enum aga_result aga_pyresult(enum py_result result) {
	switch(result) {
		default: return AGA_RESULT_ERROR;
		case PY_RESULT_OK: return AGA_RESULT_OK;
		case PY_RESULT_EOF: return AGA_RESULT_BAD_PARAM;
		case PY_RESULT_TOKEN: return AGA_RESULT_BAD_PARAM;
		case PY_RESULT_SYNTAX: return AGA_RESULT_BAD_PARAM;
		case PY_RESULT_OOM: return AGA_RESULT_OOM;
		case PY_RESULT_DONE: return AGA_RESULT_OK;
	}
}

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4305) /* Type cast truncates. */
# pragma warning(disable: 4826) /* Sign extending cast. */
#endif

void* aga_script_pointer_new(void* p) {
	return py_int_new((py_value_t) p);
}

/*
 * TODO: This is not following the `aga_script_<type>' pattern used elsewhere
 * 		 And does not typecheck/nullcheck.
 */
void* aga_script_pointer_get(void* op) {
	return (void*) py_int_get(op);
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

/* TODO: Separate interpreter state and environments in our script API. */
static enum aga_result aga_compilescript(
		struct py_env* env, const char* script,
		struct aga_resource_pack* pack, struct py_object** dict) {

	enum aga_result result;
	void* fp;
	aga_size_t size;
	struct py_object* module;
	struct py_object* eval;
	struct py_node* node;
	enum py_result res;
	struct py_code* code;

	result = aga_resource_stream(pack, script, &fp, &size);
	if(result) return result;

	if(!(module = py_module_add(env, "__main__"))) return AGA_RESULT_ERROR;

	res = py_parse_file(
			fp, script, &py_grammar, PY_GRAMMAR_FILE_INPUT, 0, 0, &node);
	if(res != PY_RESULT_DONE) return aga_pyresult(res);

	if(!(*dict = ((struct py_module*) module)->attr)) return AGA_RESULT_ERROR;
	if(!(code = py_compile(node, script))) return AGA_RESULT_ERROR;

	py_tree_delete(node);

	eval = py_code_eval(env, code, *dict, *dict, 0);
	if(py_error_occurred()) {
		aga_script_engine_trace();
		return AGA_RESULT_ERROR;
	}

	py_object_decref(eval);
	py_object_decref(code);

	return AGA_RESULT_OK;
}

enum aga_result aga_script_engine_new(
		struct aga_script_engine* eng, const char* script,
		struct aga_resource_pack* pack, const char* pypath, void* user) {

	enum aga_result result;
	enum py_result pyres;

	if(!eng) return AGA_RESULT_BAD_PARAM;
	if(!script) return AGA_RESULT_BAD_PARAM;

	eng->py = aga_calloc(1, sizeof(struct py));
	if(!eng->py) return AGA_RESULT_OOM;

	eng->py->user = user;

	eng->env = aga_calloc(1, sizeof(struct py_env));
	if(!eng->env) return AGA_RESULT_OOM;

	/* TODO: Cleanup these. */
	pyres = py_new(eng->py, pypath);
	if(pyres != PY_RESULT_OK) return aga_pyresult(pyres);

	pyres = py_env_new(eng->py, eng->env);
	if(pyres != PY_RESULT_OK) return aga_pyresult(pyres);

	pyres = py_builtin_init(eng->env);
	if(pyres != PY_RESULT_OK) return aga_pyresult(pyres);

	pyres = py_math_init(eng->env);
	if(pyres != PY_RESULT_OK) return aga_pyresult(pyres);

	result = aga_mkmod(eng->env, (void**) &eng->agan);
	if(result) return result;

	result = aga_compilescript(
			eng->env, script, pack, (struct py_object**) &eng->global);
	if(result) return result;

	return AGA_RESULT_OK;
}

enum aga_result aga_script_engine_delete(struct aga_script_engine* eng) {
	if(!eng) return AGA_RESULT_BAD_PARAM;

	py_import_done(eng->env);
	py_builtin_done();
	py_done_dict();

	py_error_clear();
	py_errors_done();

	py_grammar_delete_accels();

	return AGA_RESULT_OK;
}

enum aga_result aga_script_engine_set_pointer(
		struct aga_script_engine* eng, const char* key, void* value) {

	struct py_object* v;

	if(!eng) return AGA_RESULT_BAD_PARAM;
	if(!key) return AGA_RESULT_BAD_PARAM;

	if(!(v = aga_script_pointer_new(value))) return AGA_RESULT_ERROR;

	if(py_dict_insert(eng->agan, key, v) == -1) return AGA_RESULT_ERROR;

	return AGA_RESULT_OK;
}

enum aga_result aga_script_engine_lookup(
		struct aga_script_engine* eng, struct aga_script_class* class,
		const char* name) {

	struct py_object* cl;

	if(!eng) return AGA_RESULT_BAD_PARAM;
	if(!class) return AGA_RESULT_BAD_PARAM;
	if(!name) return AGA_RESULT_BAD_PARAM;

	if(!(class->class = py_dict_lookup(eng->global, name))) {
		return AGA_RESULT_ERROR;
	}

	cl = class->class;
	if(cl->type != PY_TYPE_CLASS) return AGA_RESULT_ERROR;

	return AGA_RESULT_OK;
}

enum aga_result aga_script_instance_new(
		struct aga_script_class* class, struct aga_script_instance* inst) {

	if(!class) return AGA_RESULT_BAD_PARAM;
	if(!inst) return AGA_RESULT_BAD_PARAM;

	inst->class = class;
	if(!(inst->object = py_class_member_new(class->class))) {
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_script_instance_delete(struct aga_script_instance* inst) {
	if(!inst) return AGA_RESULT_BAD_PARAM;

	py_object_decref((struct py_object*) inst->object);

	return AGA_RESULT_OK;
}

enum aga_result aga_script_instance_call(
		struct aga_script_engine* eng, struct aga_script_instance* inst,
		const char* name) {

	struct py_object* proc;
	struct py_object* methodcall;

	if(!inst) return AGA_RESULT_BAD_PARAM;
	if(!name) return AGA_RESULT_BAD_PARAM;

	apro_stamp_start(APRO_SCRIPT_INSTCALL_RISING);

	if(!(proc = py_class_get_attr(inst->class->class, name))) {
		return AGA_RESULT_ERROR;
	}

	methodcall = py_class_method_new(proc, inst->object);
	if(py_error_occurred()) {
		aga_script_engine_trace();
		return AGA_RESULT_ERROR;
	}

	apro_stamp_end(APRO_SCRIPT_INSTCALL_RISING);

	apro_stamp_start(APRO_SCRIPT_INSTCALL_EXEC);

	py_call_function(eng->env, methodcall, 0);
	if(py_error_occurred()) {
		aga_script_engine_trace();
		return AGA_RESULT_ERROR;
	}

	apro_stamp_end(APRO_SCRIPT_INSTCALL_EXEC);

	py_object_decref(methodcall);

	return AGA_RESULT_OK;
}
