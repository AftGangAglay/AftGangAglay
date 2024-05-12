/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_DRAW_H
#define AGAN_DRAW_H

#include <agan/agan.h>

enum agan_surface {
	AGAN_SURFACE_FRONT,
	AGAN_SURFACE_BACK,
	AGAN_SURFACE_STENCIL,
	AGAN_SURFACE_DEPTH
};

enum aga_result agan_draw_register(struct py_env*);

struct py_object* agan_setcam(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_text(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_fogparam(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_fogcol(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_clear(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_mktrans(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_shadeflat(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_getpix(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_setflag(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_getflag(
		struct py_env* env, struct py_object*, struct py_object*);

struct py_object* agan_line3d(
		struct py_env* env, struct py_object*, struct py_object*);

#endif
