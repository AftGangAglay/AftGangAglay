/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_H
#define AGAN_H

#include <asys/result.h>

#include <aga/python.h>

#define AGA_GET_USERDATA(env) ((struct aga_script_userdata*) env->py->user)

struct py_env;

struct aga_config_node;

extern struct py_object* agan_dict;
extern const char* agan_trans_components[3];
extern const char* agan_conf_components[3];
extern const char* agan_xyz[3];
extern const char* agan_rgb[3];

enum asys_result aga_insertstr(const char*, const char*);

enum asys_result aga_insertfloat(const char*, double);

enum asys_result aga_insertint(const char*, py_value_t);

enum asys_result aga_mkmod(struct py_env*, void**);

asys_bool_t aga_script_err(const char*, const char*, enum asys_result);

asys_bool_t aga_script_gl_err(const char*, const char*);

asys_bool_t agan_settransmat(struct py_object*, asys_bool_t);

struct py_object* agan_scriptconf(
		struct aga_config_node*, asys_bool_t, struct py_object*);

asys_bool_t aga_arg_list(const struct py_object*, enum py_type);
asys_bool_t aga_vararg_list(const struct py_object*, enum py_type, asys_size_t);
asys_bool_t aga_vararg_list_typed(
		const struct py_object*, enum py_type, asys_size_t, enum py_type);

/*
 * NOTE: This assumes you have already verified the argument list is a valid
 * 		 Tuple object with `aga_arg_list'.
 */
asys_bool_t aga_arg(
		struct py_object**, struct py_object*, asys_size_t, enum py_type);

asys_bool_t aga_vararg(
		struct py_object**, struct py_object*, asys_size_t, enum py_type,
		asys_size_t);

asys_bool_t aga_vararg_typed(
		struct py_object**, struct py_object*, asys_size_t, enum py_type,
		asys_size_t, enum py_type);

/* Just returns 0. */
void* aga_arg_error(const char*, const char*);

#endif
