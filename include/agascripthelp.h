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
#define AGA_INCREF(v) (py_object_incref(v), v)

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

aga_bool_t aga_script_float(struct py_object*, float*);

aga_bool_t aga_script_int(struct py_object*, int*);

aga_bool_t aga_script_string(struct py_object*, const char**);

aga_bool_t aga_script_bool(struct py_object*, aga_bool_t*);

aga_bool_t aga_list_set(struct py_object*, aga_size_t, struct py_object*);

aga_bool_t aga_list_get(struct py_object*, aga_size_t, struct py_object**);

#endif
