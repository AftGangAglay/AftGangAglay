/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/script.h>
#include <aga/pack.h>
#include <aga/python.h>

#include <agan/agan.h>

#include <python/graminit.h>

#include <apro.h>

#include <asys/log.h>
#include <asys/memory.h>

/*
 * TODO: Once we have precompiled binaries -- make python consume bytecode
 * 		 Buffered stream-wise instead of preallocating entire bytecode buffer.
 */

void aga_script_engine_trace(void) {
	struct py_object* traceback;
	struct py_object* exception;
	struct py_object* value;

	if(!py_error_occurred()) return;

	if(!(traceback = py_traceback_get())) return;

	py_error_get(&exception, &value);

	if(!exception || !value) return;

	asys_log(
			__FILE__, "%s: %s",
			py_string_get(exception), py_string_get(value));

	asys_log(__FILE__, "Stack backtrace (innermost last):");

	py_traceback_print(traceback);
	py_object_decref(traceback);
}

enum asys_result aga_pyresult(enum py_result result) {
	switch(result) {
		default: return ASYS_RESULT_ERROR;
		case PY_RESULT_OK: return ASYS_RESULT_OK;
		case PY_RESULT_EOF: return ASYS_RESULT_EOF;
		case PY_RESULT_TOKEN: return ASYS_RESULT_INVALID_TOKEN;
		case PY_RESULT_SYNTAX: return ASYS_RESULT_BAD_SYNTAX;
		case PY_RESULT_OOM: return ASYS_RESULT_OOM;
		case PY_RESULT_DONE: return ASYS_RESULT_OK;
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
static enum asys_result aga_script_compile(
		struct py_env* env, const char* script,
		struct aga_resource_pack* pack, struct py_object** dict) {

	enum asys_result result;
	enum py_result py_result;

	struct asys_stream* stream;
	asys_size_t size;
	struct py_object* module;
	struct py_node* node;
	struct py_code* code;

	result = aga_resource_stream(pack, script, &stream, &size);
	if(result) return result;

	if(!(module = py_module_add(env, "__main__"))) return ASYS_RESULT_ERROR;

	py_result = py_parse_file(
					stream, script, &py_grammar, PY_GRAMMAR_FILE_INPUT, &node);

	if(py_result != PY_RESULT_DONE) return aga_pyresult(py_result);

	if(!(*dict = ((struct py_module*) module)->attr)) return ASYS_RESULT_ERROR;
	if(!(code = py_compile(node, script))) return ASYS_RESULT_ERROR;

	py_tree_delete(node);

	py_object_decref(py_code_eval(env, code, *dict, *dict, 0));
	if(py_error_occurred()) {
		aga_script_engine_trace();
		return ASYS_RESULT_ERROR;
	}

	py_object_decref(code);

	return ASYS_RESULT_OK;
}

enum asys_result aga_script_engine_new(
		struct aga_script_engine* eng, const char* script,
		struct aga_resource_pack* pack, const char* pypath, void* user) {

	enum asys_result result;
	enum py_result pyres;

	if(!eng) return ASYS_RESULT_BAD_PARAM;
	if(!script) return ASYS_RESULT_BAD_PARAM;

	eng->py = asys_memory_allocate_zero(1, sizeof(struct py));
	if(!eng->py) return ASYS_RESULT_OOM;

	eng->py->user = user;

	eng->env = asys_memory_allocate_zero(1, sizeof(struct py_env));
	if(!eng->env) return ASYS_RESULT_OOM;

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

	result = aga_script_compile(
			eng->env, script, pack, (struct py_object**) &eng->global);

	if(result) return result;

	return ASYS_RESULT_OK;
}

enum asys_result aga_script_engine_delete(struct aga_script_engine* eng) {
	if(!eng) return ASYS_RESULT_BAD_PARAM;

	py_import_done(eng->env);
	py_builtin_done();
	py_done_dict();

	py_error_clear();
	py_errors_done();

	py_grammar_delete_accels();

	return ASYS_RESULT_OK;
}

enum asys_result aga_script_engine_set_pointer(
		struct aga_script_engine* eng, const char* key, void* value) {

	struct py_object* v;

	if(!eng) return ASYS_RESULT_BAD_PARAM;
	if(!key) return ASYS_RESULT_BAD_PARAM;

	if(!(v = aga_script_pointer_new(value))) return ASYS_RESULT_ERROR;

	if(py_dict_insert(eng->agan, key, v) == -1) return ASYS_RESULT_ERROR;

	return ASYS_RESULT_OK;
}

/* TODO: Decref this lookup once finished. */
enum asys_result aga_script_engine_lookup(
		struct aga_script_engine* eng, struct aga_script_class* class,
		const char* name) {

	struct py_object* cl;

	if(!eng) return ASYS_RESULT_BAD_PARAM;
	if(!class) return ASYS_RESULT_BAD_PARAM;
	if(!name) return ASYS_RESULT_BAD_PARAM;

	class->class = py_dict_lookup(eng->global, name);
	if(!class->class) {
		asys_log(__FILE__, "Could not find class `%s' in game script", name);
		return ASYS_RESULT_ERROR;
	}

	cl = class->class;
	if(cl->type != PY_TYPE_CLASS) {
		asys_log(__FILE__, "Identifier `%s' was not `class'", name);
		return ASYS_RESULT_ERROR;
	}

	/* TODO: Leaky error states. */
	class->create_attr = py_class_get_attr(class->class, "create");
	if(!class->create_attr) return ASYS_RESULT_ERROR;

	class->update_attr = py_class_get_attr(class->class, "update");
	if(!class->update_attr) return ASYS_RESULT_ERROR;

	class->close_attr = py_class_get_attr(class->class, "close");
	if(!class->close_attr) return ASYS_RESULT_ERROR;

	return ASYS_RESULT_OK;
}

enum asys_result aga_script_instance_new(
		struct aga_script_class* class, struct aga_script_instance* inst) {

	if(!class) return ASYS_RESULT_BAD_PARAM;
	if(!inst) return ASYS_RESULT_BAD_PARAM;

	inst->class = class;

	inst->object = py_class_member_new(class->class);
	if(!inst->object) return ASYS_RESULT_ERROR;

	inst->create_call = py_class_method_new(class->create_attr, inst->object);
	if(!inst->create_call) return ASYS_RESULT_ERROR;

	inst->update_call = py_class_method_new(class->update_attr, inst->object);
	if(!inst->update_call) return ASYS_RESULT_ERROR;

	inst->close_call = py_class_method_new(class->close_attr, inst->object);
	if(!inst->close_call) return ASYS_RESULT_ERROR;

	return ASYS_RESULT_OK;
}

enum asys_result aga_script_instance_delete(struct aga_script_instance* inst) {
	if(!inst) return ASYS_RESULT_BAD_PARAM;

	py_object_decref(inst->object);

	py_object_decref(inst->create_call);
	py_object_decref(inst->update_call);
	py_object_decref(inst->close_call);

	return ASYS_RESULT_OK;
}

enum asys_result aga_script_instance_call(
		struct aga_script_engine* eng, struct aga_script_instance* inst,
		enum aga_script_instance_method method) {

	void* call;

	if(!eng) return ASYS_RESULT_BAD_PARAM;
	if(!inst) return ASYS_RESULT_BAD_PARAM;

	apro_stamp_start(APRO_SCRIPT_INSTCALL);

	switch(method) {
		case AGA_SCRIPT_CREATE: call = inst->create_call; break;
		case AGA_SCRIPT_UPDATE: call = inst->update_call; break;
		case AGA_SCRIPT_CLOSE: call = inst->close_call; break;

		default: return ASYS_RESULT_BAD_PARAM;
	}

	py_call_function(eng->env, call, 0);
	if(py_error_occurred()) {
		aga_script_engine_trace();
		return ASYS_RESULT_ERROR;
	}

	apro_stamp_end(APRO_SCRIPT_INSTCALL);

	return ASYS_RESULT_OK;
}
