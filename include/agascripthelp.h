/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPTHELP_H
#define AGA_SCRIPTHELP_H

#include <agapyinc.h>

typedef object* aga_pyobject_t;

/*
 * NOTE: I'm okay with allowing a bit more macro magic in here to reduce the
 * 		 Overall verbosity of the Python glue code.
 */

/* TODO: This can just be a function. */
#define AGA_INCREF(v) (INCREF(v), v)

/* TODO: These can just be functions. */
#define AGA_ARGLIST(type) (arg && is_##type##object(arg))
#define AGA_ARG(var, n, type) \
    (((var) = gettupleitem(arg, (n))) && is_##type##object((var)))
#define AGA_ARGERR(func, types) \
    do { \
        err_setstr(TypeError, func "() arguments must be " types); \
        return 0; \
    } while(0)

aga_bool_t aga_script_float(aga_pyobject_t o, float* f);

aga_bool_t aga_script_int(aga_pyobject_t o, int* i);

aga_bool_t aga_script_string(aga_pyobject_t o, char** s);

aga_bool_t aga_script_bool(aga_pyobject_t o, aga_bool_t* b);

aga_bool_t aga_list_set(aga_pyobject_t list, aga_size_t n, aga_pyobject_t v);

aga_bool_t aga_list_get(aga_pyobject_t list, aga_size_t n, aga_pyobject_t* v);

#endif
