/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_MISC_H
#define AGAN_MISC_H

#include <agan/agan.h>

enum aga_result agan_misc_register(void);

struct py_object* agan_getconf(struct py_object*, struct py_object*);

struct py_object* agan_log(struct py_object*, struct py_object*);

struct py_object* agan_die(struct py_object*, struct py_object*);

struct py_object* agan_killpack(struct py_object*, struct py_object*);
struct py_object* agan_mkpack(struct py_object*, struct py_object*);

#endif
