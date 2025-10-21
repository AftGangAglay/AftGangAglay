/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/script.h>
#include <aga/pack.h>

#include <agan/agan.h>

#include <asys/log.h>
#include <asys/error.h>
#include <asys/string.h>

/*
 * Defines a few misc. functions Python wants and a few scriptglue helpers
 * Declared in `agapyinc'.
 */

void py_fatal(const char* msg) {
	asys_result_fatal(__FILE__, msg, ASYS_RESULT_ERROR);
}

enum asys_result py_open_r(
		struct py_env* env, const char* path, struct asys_stream** stream) {

	enum asys_result result;

	struct aga_resource* resource;

	result = aga_resource_pack_lookup(
			AGA_GET_USERDATA(env)->resource_pack, path, &resource);

	if(result) return result;

	result = aga_resource_seek(resource, stream);
	if(result) return result;

	return ASYS_RESULT_OK;
}

asys_bool_t aga_arg_list(
		const struct py_object* args, enum py_type type) {

	return args && args->type == type;
}

asys_bool_t aga_vararg_list(
		const struct py_object* args, enum py_type type, asys_size_t len) {

	return aga_arg_list(args, type) && py_varobject_size(args) == len;
}

asys_bool_t aga_vararg_list_typed(
		const struct py_object* args, enum py_type type, asys_size_t len,
		enum py_type membtype) {

	unsigned i;
	asys_bool_t b = aga_vararg_list(args, type, len);

	if(!b) return ASYS_FALSE;

	for(i = 0; i < len; ++i) {
		if(type == PY_TYPE_LIST) {
			if(py_list_get(args, i)->type != membtype) return ASYS_FALSE;
		}
		else if(type == PY_TYPE_TUPLE) {
			if(py_tuple_get(args, i)->type != membtype) return ASYS_FALSE;
		}
	}

	return ASYS_TRUE;
}

asys_bool_t aga_arg(
		struct py_object** v, struct py_object* args, asys_size_t n,
		enum py_type type) {

	return (*v = py_tuple_get(args, (unsigned) n)) && (*v)->type == type;
}

asys_bool_t aga_arg_func(
		struct py_object** v, struct py_object* args, asys_size_t n) {

	return (*v = py_tuple_get(args, (unsigned) n)) &&
			((*v)->type == PY_TYPE_FUNC ||
					(*v)->type == PY_TYPE_METHOD ||
					(*v)->type == PY_TYPE_CLASS_METHOD);
}

asys_bool_t aga_vararg(
		struct py_object** v, struct py_object* args, asys_size_t n,
		enum py_type type, asys_size_t len) {

	return aga_arg(v, args, n, type) && py_varobject_size(*v) == len;
}

asys_bool_t aga_vararg_typed(
		struct py_object** v, struct py_object* args, asys_size_t n,
		enum py_type type, asys_size_t len, enum py_type membtype) {

	unsigned i;
	asys_bool_t b = aga_vararg(v, args, n, type, len);

	if(!b) return ASYS_FALSE;

	for(i = 0; i < len; ++i) {
		if(type == PY_TYPE_LIST) {
			if(py_list_get(*v, i)->type != membtype) return ASYS_FALSE;
		}
		else if(type == PY_TYPE_TUPLE) {
			if(py_tuple_get(*v, i)->type != membtype) return ASYS_FALSE;
		}
	}

	return ASYS_TRUE;
}

void* aga_arg_error(const char* function, const char* types) {
	asys_fixed_buffer_t buffer = { 0 };

	asys_string_concatenate(buffer, function);
	asys_string_concatenate(buffer, "() arguments must be ");
	asys_string_concatenate(buffer, types);

	py_error_set_string(py_type_error, buffer);

	return 0;
}
