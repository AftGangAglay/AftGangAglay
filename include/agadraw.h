/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_DRAW_H
#define AGA_DRAW_H

#include <agaresult.h>

enum aga_result aga_setdrawparam(void);

enum aga_result aga_pushrawdraw(void);

enum aga_result aga_poprawdraw(void);

enum aga_result aga_puttext(float, float, const char*);

enum aga_result aga_puttextfmt(float, float, const char*, ...);

enum aga_result aga_clear(const float*);

enum aga_result aga_flush(void);

#endif
