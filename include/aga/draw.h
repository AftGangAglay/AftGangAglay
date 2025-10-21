/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_DRAW_H
#define AGA_DRAW_H

#include <asys/result.h>
#include <asys/base.h>

enum aga_draw_flags {
	AGA_DRAW_NONE = 0,
	AGA_DRAW_BACKFACE = 1,
	AGA_DRAW_BLEND = 1 << 1,
	AGA_DRAW_FOG = 1 << 2,
	AGA_DRAW_TEXTURE = 1 << 3,
	AGA_DRAW_LIGHTING = 1 << 4,
	AGA_DRAW_DEPTH = 1 << 5,
	AGA_DRAW_FLAT = 1 << 6,
	AGA_DRAW_FIDELITY = 1 << 7
};

enum asys_result aga_draw_set(enum aga_draw_flags);
enum aga_draw_flags aga_draw_get(void);

enum asys_result aga_draw_push(void);
enum asys_result aga_draw_pop(void);

enum asys_result aga_draw_fidelity(asys_bool_t);

/* NOTE: Outputs pointer to static string storage. */
enum asys_result aga_renderer_string(const char**);

#endif
