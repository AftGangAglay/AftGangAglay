/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_DRAW_H
#define AGAN_DRAW_H

#include <agan/agan.h>

struct py_object* agan_setcam(struct py_object*, struct py_object*);

struct py_object* agan_text(struct py_object*, struct py_object*);

struct py_object* agan_fogparam(struct py_object*, struct py_object*);

struct py_object* agan_fogcol(struct py_object*, struct py_object*);

struct py_object* agan_clear(struct py_object*, struct py_object*);

struct py_object* agan_mktrans(struct py_object*, struct py_object*);

struct py_object* agan_shadeflat(struct py_object*, struct py_object*);

struct py_object* agan_getpix(struct py_object*, struct py_object*);

#endif
