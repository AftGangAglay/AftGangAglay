/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_MATH_H
#define AGAN_MATH_H

#include <agan/agan.h>

enum asys_result agan_math_register(struct py_env*);

struct py_object* agan_bitand(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_bitshl(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_randnorm(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_bitor(
		struct py_env*, struct py_object*, struct py_object*);

#endif
