/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/std.h>
#include <aga/script.h>
#include <aga/log.h>
#include <aga/pack.h>
#include <aga/utility.h>
#include <aga/error.h>

#include <agan/agan.h>

/*
 * Defines a few misc. functions Python wants and a few scriptglue helpers
 * Declared in `agapyinc'.
 */

void py_fatal(const char* msg) {
	aga_log(__FILE__, "Python Fatal Error: %s", msg);
	abort();
}

void* py_open_r(const char* path) {
	void* fp;
	aga_size_t i;

	for(i = 0; i < aga_global_pack->len; ++i) {
		struct aga_resource* res = &aga_global_pack->db[i];
		if(aga_streql(path, res->conf->name)) {
			enum aga_result result;

			result = aga_resource_seek(res, &fp);
			if(result) {
				aga_error_check_soft(__FILE__, "aga_resource_seek", result);
				return 0;
			}

			return fp;
		}
	}

	if(!(fp = fopen(path, "rb"))) aga_error_system_path(__FILE__, "fopen", path);
	return fp;
}

void py_close(void* fp) {
	if(!fp) return;
	if(fp == aga_global_pack->fp) return;

	if(fclose(fp) == EOF) aga_error_system(__FILE__, "fclose");
}

aga_bool_t aga_arg_list(
		const struct py_object* args, enum py_type type) {

	return args && args->type == type;
}

aga_bool_t aga_vararg_list(
		const struct py_object* args, enum py_type type, aga_size_t len) {

	return aga_arg_list(args, type) && py_varobject_size(args) == len;
}

aga_bool_t aga_vararg_list_typed(
		const struct py_object* args, enum py_type type, aga_size_t len,
		enum py_type membtype) {

	unsigned i;
	aga_bool_t b = aga_vararg_list(args, type, len);

	if(!b) return AGA_FALSE;

	for(i = 0; i < len; ++i) {
		if(type == PY_TYPE_LIST) {
			if(py_list_get(args, i)->type != membtype) return AGA_FALSE;
		}
		else if(type == PY_TYPE_TUPLE) {
			if(py_tuple_get(args, i)->type != membtype) return AGA_FALSE;
		}
	}

	return AGA_TRUE;
}

aga_bool_t aga_arg(
		struct py_object** v, struct py_object* args, aga_size_t n,
		enum py_type type) {

	return (*v = py_tuple_get(args, n)) && (*v)->type == type;
}

aga_bool_t aga_vararg(
		struct py_object** v, struct py_object* args, aga_size_t n,
		enum py_type type, aga_size_t len) {

	return aga_arg(v, args, n, type) && py_varobject_size(*v) == len;
}

aga_bool_t aga_vararg_typed(
		struct py_object** v, struct py_object* args, aga_size_t n,
		enum py_type type, aga_size_t len, enum py_type membtype) {

	unsigned i;
	aga_bool_t b = aga_vararg(v, args, n, type, len);

	if(!b) return AGA_FALSE;

	for(i = 0; i < len; ++i) {
		if(type == PY_TYPE_LIST) {
			if(py_list_get(*v, i)->type != membtype) return AGA_FALSE;
		}
		else if(type == PY_TYPE_TUPLE) {
			if(py_tuple_get(*v, i)->type != membtype) return AGA_FALSE;
		}
	}

	return AGA_TRUE;
}

void* aga_arg_error(const char* proc, const char* types) {
	aga_fixed_buf_t buf = { 0 };

	strcat(buf, proc);
	strcat(buf, "() arguments must be ");
	strcat(buf, types);
	py_error_set_string(py_type_error, buf);

	return 0;
}
