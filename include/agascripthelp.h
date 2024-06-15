/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

/* TODO: This should probably be in agan. */

#ifndef AGA_SCRIPTHELP_H
#define AGA_SCRIPTHELP_H

#include <agapyinc.h>

void* aga_script_mkptr(void*);
void* aga_script_getptr(void*);

aga_bool_t aga_arg_list(const struct py_object*, enum py_type);
aga_bool_t aga_vararg_list(const struct py_object*, enum py_type, aga_size_t);
aga_bool_t aga_vararg_list_typed(
		const struct py_object*, enum py_type, aga_size_t, enum py_type);

/*
 * NOTE: This assumes you have already verified the argument list is a valid
 * 		 Tuple object with `aga_arg_list'.
 */
aga_bool_t aga_arg(
		struct py_object**, struct py_object*, aga_size_t, enum py_type);

aga_bool_t aga_vararg(
		struct py_object**, struct py_object*, aga_size_t, enum py_type,
		aga_size_t);

aga_bool_t aga_vararg_typed(
		struct py_object**, struct py_object*, aga_size_t, enum py_type,
		aga_size_t, enum py_type);

/* Just returns 0. */
void* aga_arg_error(const char*, const char*);

#endif
