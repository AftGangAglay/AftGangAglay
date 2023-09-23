/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WIN_H
#define AGA_WIN_H

#include <afeirsa/aftypes.h>
#include <afeirsa/aferr.h>

#include <X11/keysymdef.h>

struct aga_ctx;

struct aga_win {
	af_ulong_t xwin;
};

enum af_err aga_mkctxdpy(struct aga_ctx* ctx, const char* display);
enum af_err aga_killctxdpy(struct aga_ctx* ctx);

enum af_err aga_mkwin(struct aga_ctx* ctx, struct aga_win* win);
enum af_err aga_killwin(struct aga_ctx* ctx, struct aga_win* win);

enum af_err aga_glctx(struct aga_ctx* ctx, struct aga_win* win);
enum af_err aga_swapbuf(struct aga_ctx* ctx, struct aga_win* win);

enum af_err aga_poll(struct aga_ctx* ctx);

#endif
