/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_AUDIO_H
#define AGAN_AUDIO_H

#include <agan/agan.h>

enum asys_result agan_audio_register(struct py_env*);

struct py_object* agan_playsnd(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_clrsnd(
		struct py_env*, struct py_object*, struct py_object*);

#endif
