/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_H
#define AGAN_H

#include <agascripthelp.h>

struct aga_conf_node;

struct agan_nativeptr {
	OB_HEAD
	void* ptr;
};

extern const typeobject agan_nativeptr_type;

#define is_nativeptrobject(op) ((op)->ob_type == &agan_nativeptr_type)

aga_pyobject_t agan_mknativeptr(void* ptr);

#define AGAN_SCRIPTPROC(name) \
	aga_pyobject_t agan_##name(AGA_UNUSED aga_pyobject_t self, \
           AGA_UNUSED aga_pyobject_t arg)

extern aga_pyobject_t agan_dict;
extern const char* agan_trans_components[3];
extern const char* agan_conf_components[3];
extern const char* agan_xyz[3];

aga_bool_t agan_settransmat(aga_pyobject_t trans, aga_bool_t inv);
aga_pyobject_t agan_scriptconf(
		struct aga_conf_node* node, aga_bool_t root, aga_pyobject_t list);

#endif
