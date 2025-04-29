/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_AUDIO_H
#define AGAN_AUDIO_H

#include <agan/agan.h>

enum asys_result agan_audio_register(struct py_env*);

struct py_object* agan_playsnd(
		struct py_env*, struct py_object*, struct py_object*);

#endif
