/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_MATH_H
#define AGAN_MATH_H

#include <agan/agan.h>

enum aga_result agan_math_register(void);

struct py_object* agan_bitand(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_bitshl(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_randnorm(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_bitor(
		struct py_env* env, struct py_object*, struct py_object*);

#endif
