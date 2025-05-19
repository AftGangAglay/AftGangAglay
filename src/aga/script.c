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
