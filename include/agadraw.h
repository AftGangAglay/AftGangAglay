/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_DRAW_H
#define AGA_DRAW_H

#include <agaresult.h>

enum aga_drawflags {
	AGA_DRAW_NONE = 0,
	AGA_DRAW_BACKFACE = 1,
	AGA_DRAW_BLEND = 1 << 1,
	AGA_DRAW_FOG = 1 << 2,
	AGA_DRAW_TEXTURE = 1 << 3,
	AGA_DRAW_LIGHTING = 1 << 4,
	AGA_DRAW_DEPTH = 1 << 5
};

enum aga_result aga_setdraw(enum aga_drawflags);
enum aga_drawflags aga_getdraw(void);

enum aga_result aga_pushall_ident(void);
enum aga_result aga_popall(void);

enum aga_result aga_puttext(float, float, const char*);

enum aga_result aga_puttextfmt(float, float, const char*, ...);

enum aga_result aga_clear(const float*);

enum aga_result aga_flush(void);

enum aga_result aga_gl_error(const char*, const char*);

#endif
