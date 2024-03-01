/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPTHELP_H
#define AGA_SCRIPTHELP_H

#include <agapyinc.h>

/*
 * NOTE: I'm okay with allowing a bit more macro magic in here to reduce the
 * 		 Overall verbosity of the Python glue code.
 */

/* TODO: This can just be a function. */
#define AGA_INCREF(v) (PY_INCREF(v), v)

/* TODO: `py_is_*' should be functions. */
/* TODO: These can just be functions. */
#define AGA_ARGLIST(type) (arg && py_is_##type(arg))
#define AGA_ARG(var, n, type) \
    (((var) = py_tuple_get(arg, (n))) && py_is_##type((var)))
#define AGA_ARGERR(func, types) \
    do { \
        py_error_set_string(py_type_error, func "() arguments must be " types); \
        return 0; \
    } while(0)

aga_bool_t aga_script_float(struct py_object* o, float* f);

aga_bool_t aga_script_int(struct py_object* o, int* i);

aga_bool_t aga_script_string(struct py_object* o, char** s);

aga_bool_t aga_script_bool(struct py_object* o, aga_bool_t* b);

aga_bool_t
aga_list_set(struct py_object* list, aga_size_t n, struct py_object* v);

aga_bool_t
aga_list_get(struct py_object* list, aga_size_t n, struct py_object** v);

#endif
