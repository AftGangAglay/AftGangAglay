/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_DRAW_H
#define AGA_DRAW_H

#include <afeirsa/afeirsa.h>

enum af_err aga_setdrawparam(struct af_ctx* af, struct af_vert* vert);

enum af_err aga_pushrawdraw(void);
enum af_err aga_poprawdraw(void);

enum af_err aga_puttext(float x, float y, const char* text);
enum af_err aga_puttextfmt(float x, float y, const char* fmt, ...);

#endif
