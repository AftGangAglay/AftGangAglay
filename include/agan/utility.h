/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_MISC_H
#define AGAN_MISC_H

#include <agan/agan.h>

enum asys_result agan_misc_register(struct py_env*);

struct py_object* agan_getconf(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_packlist(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_log(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_die(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_dt(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_strsplit(
		struct py_env*, struct py_object*, struct py_object*);

#endif
