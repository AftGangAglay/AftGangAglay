/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agastd.h>
#include <agascript.h>
#include <agalog.h>
#include <agascripthelp.h>
#include <agapack.h>
#include <agautil.h>
#include <agaerr.h>

#include <agan/agan.h>

/*
 * Defines a few misc. functions Python wants and a few scriptglue helpers
 * Declared in `agapyinc'.
 */

#ifdef _DEBUG
/* The extra debug info this enables is a bit too verbose. */
int debugging = 0;
#endif

void py_fatal(const char* msg) {
	aga_log(__FILE__, "Python Fatal Error: %s", msg);
	abort();
}

FILE* pyopen_r(const char* path) {
	void* fp;
	aga_size_t i;

	for(i = 0; i < aga_global_pack->len; ++i) {
		struct aga_res* res = &aga_global_pack->db[i];
		if(aga_streql(path, res->conf->name)) {
			enum aga_result result;

			result = aga_resseek(res, &fp);
			if(result) {
				aga_soft(__FILE__, "aga_resseek", result);
				return 0;
			}

			return fp;
		}
	}

	if(!(fp = fopen(path, "rb"))) aga_errno_path(__FILE__, "fopen", path);
	return fp;
}

void pyclose(FILE* fp) {
	if(!fp) return;
	if(fp == aga_global_pack->fp) return;

	if(fclose(fp) == EOF) aga_errno(__FILE__, "fclose");
}

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4305) /* Type cast truncates. */
# pragma warning(disable: 4826) /* Sign extending cast. */
#endif

void* aga_script_mkptr(void* p) {
	return py_int_new((py_value_t) p);
}

void* aga_script_getptr(void* op) {
	return (void*) py_int_get(op);
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

aga_bool_t aga_arg_list(
		const struct py_object* args, enum py_type type) {

	return args && args->type == type;
}

aga_bool_t aga_arg(
		struct py_object** v, struct py_object* args, aga_size_t n,
		enum py_type type) {

	return (*v = py_tuple_get(args, n)) && (*v)->type == type;
}

void* aga_arg_error(const char* proc, const char* types) {
	aga_fixed_buf_t buf;

	strcat(buf, proc);
	strcat(buf, "() arguments must be ");
	strcat(buf, types);
	py_error_set_string(py_type_error, buf);

	return 0;
}

aga_bool_t aga_script_float(struct py_object* o, double* f) {
	*f = py_float_get(o);
	return !!py_error_occurred();
}

aga_bool_t aga_script_int(struct py_object* o, py_value_t * i) {
	*i = py_int_get(o);
	return !!py_error_occurred();
}

aga_bool_t aga_script_string(struct py_object* o, const char** s) {
	*s = py_string_get(o);
	return !!py_error_occurred();
}

aga_bool_t aga_script_bool(struct py_object* o, aga_bool_t* b) {
	*b = !!py_int_get(o);
	return !!py_error_occurred();
}

aga_bool_t aga_list_set(
		struct py_object* list, aga_size_t n, struct py_object* v) {

	return py_list_set(list, (int) n, v) == -1;
}

aga_bool_t aga_list_get(
		struct py_object* list, aga_size_t n, struct py_object** v) {

	return !(*v = py_list_get(list, (int) n));
}
