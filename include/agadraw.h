/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_DRAW_H
#define AGA_DRAW_H

#include <agacore.h>
#include <afeirsa/aferr.h>

enum af_err aga_puttext(
		struct aga_ctx* ctx, float x, float y, const char* text);

enum af_err aga_puttextfmt(
		struct aga_ctx* ctx, float x, float y, const char* fmt, ...);

#endif
