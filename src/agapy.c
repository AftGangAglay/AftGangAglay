/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agastd.h>
#include <agascript.h>
#include <agalog.h>
#include <agascripthelp.h>

#include <agan/agan.h>

/*
 * Defines our nativeptr type and a few misc. functions Python wants.
 * Also contains a few scriptglue helpers declared in `agapyinc'.
 */

#ifdef _DEBUG
/* The extra debug info this enables is a bit too verbose. */
int debugging = 0;
#endif

static void agan_killnativeptr(struct py_object* obj) { free(obj); }

const struct py_type agan_nativeptr_type = {
		{ 1, 0, &py_type_type }, "nativeptr",
		sizeof(struct agan_nativeptr), agan_killnativeptr, 0, 0, 0, 0 };

struct py_object* agan_mknativeptr(void* ptr) {
	struct py_object* o = py_object_new((void*) &agan_nativeptr_type);
	((struct agan_nativeptr*) o)->ptr = ptr;
	return o;
}

void py_fatal(const char* msg) {
	aga_log(__FILE__, "Python Fatal Error: %s", msg);
	abort();
}

FILE* pyopen_r(const char* path) {
	return aga_open_r(path);
}

void pyclose(FILE* fp) {
	aga_close(fp);
}

aga_bool_t aga_script_float(struct py_object* o, float* f) {
	*f = (float) py_float_get(o);
	return py_error_occurred();
}

aga_bool_t aga_script_int(struct py_object* o, int* i) {
	*i = py_int_get(o);
	return py_error_occurred();
}

aga_bool_t aga_script_string(struct py_object* o, const char** s) {
	*s = py_string_get(o);
	return py_error_occurred();
}

aga_bool_t aga_script_bool(struct py_object* o, aga_bool_t* b) {
	*b = !!py_int_get(o);
	return py_error_occurred();
}

aga_bool_t
aga_list_set(struct py_object* list, aga_size_t n, struct py_object* v) {
	return py_list_set(list, (int) n, v) == -1;
}

aga_bool_t
aga_list_get(struct py_object* list, aga_size_t n, struct py_object** v) {
	return !(*v = py_list_get(list, (int) n));
}
