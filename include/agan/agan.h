/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_H
#define AGAN_H

#include <agascripthelp.h>

struct aga_conf_node;

struct agan_nativeptr {
	struct py_object ob;

	void* ptr;
};

extern const struct py_type agan_nativeptr_type;

#define py_is_nativeptr(op) ((op)->type == &agan_nativeptr_type)

struct py_object* agan_mknativeptr(void* ptr);

extern struct py_object* agan_dict;
extern const char* agan_trans_components[3];
extern const char* agan_conf_components[3];
extern const char* agan_xyz[3];

aga_bool_t agan_settransmat(struct py_object* trans, aga_bool_t inv);

struct py_object* agan_scriptconf(
		struct aga_conf_node* node, aga_bool_t root, struct py_object* list);

#endif
