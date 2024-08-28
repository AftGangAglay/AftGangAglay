/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_DRAW_H
#define AGA_DRAW_H

#include <aga/result.h>

enum aga_draw_flags {
	AGA_DRAW_NONE = 0,
	AGA_DRAW_BACKFACE = 1,
	AGA_DRAW_BLEND = 1 << 1,
	AGA_DRAW_FOG = 1 << 2,
	AGA_DRAW_TEXTURE = 1 << 3,
	AGA_DRAW_LIGHTING = 1 << 4,
	AGA_DRAW_DEPTH = 1 << 5,
	AGA_DRAW_FLAT = 1 << 6
};

enum aga_result aga_draw_set(enum aga_draw_flags);
enum aga_draw_flags aga_draw_get(void);

enum aga_result aga_draw_push(void);
enum aga_result aga_draw_pop(void);

#endif
