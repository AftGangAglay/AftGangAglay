/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_H
#define AGAN_H

#include <agaresult.h>
#include <agapyinc.h>

struct py_env;

struct aga_conf_node;

extern struct py_object* agan_dict;
extern const char* agan_trans_components[3];
extern const char* agan_conf_components[3];
extern const char* agan_xyz[3];
extern const char* agan_rgb[3];

enum aga_result aga_insertstr(const char*, const char*);

enum aga_result aga_insertfloat(const char*, double);

enum aga_result aga_insertint(const char*, py_value_t);

enum aga_result aga_mkmod(void**);

aga_bool_t aga_script_err(const char*, enum aga_result);

aga_bool_t aga_script_gl_err(const char*);

void* aga_getscriptptr(const char*);

aga_bool_t agan_settransmat(struct py_object*, aga_bool_t);

struct py_object* agan_scriptconf(
		struct aga_conf_node*, aga_bool_t, struct py_object*);

#endif
