/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_IO_H
#define AGAN_IO_H

#include <agan/agan.h>

enum aga_result agan_io_register(struct py_env*);

struct py_object* agan_getkey(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_getmotion(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_setcursor(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_getbuttons(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_getpos(
		struct py_env* env, struct py_object*, struct py_object*);

#endif
