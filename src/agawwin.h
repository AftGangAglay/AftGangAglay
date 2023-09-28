/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_W_WIN_H
#define AGA_W_WIN_H

enum af_err aga_mkctxdpy(struct aga_ctx* ctx, const char* display) {
	(void) ctx;
	(void) display;
	return AF_ERR_NONE;
}

enum af_err aga_killctxdpy(struct aga_ctx* ctx) {
	(void) ctx;
	return AF_ERR_NONE;
}

enum af_err aga_mkwin(struct aga_ctx* ctx, struct aga_win* win) {
	(void) ctx;
	(void) win;
	return AF_ERR_NONE;
}

enum af_err aga_killwin(struct aga_ctx* ctx, struct aga_win* win) {
	(void) ctx;
	(void) win;
	return AF_ERR_NONE;
}

enum af_err aga_glctx(struct aga_ctx* ctx, struct aga_win* win) {
	(void) ctx;
	(void) win;
	return AF_ERR_NONE;
}

enum af_err aga_swapbuf(struct aga_ctx* ctx, struct aga_win* win) {
	(void) ctx;
	(void) win;
	return AF_ERR_NONE;
}

enum af_err aga_poll(struct aga_ctx* ctx) {
	(void) ctx;
	return AF_ERR_NONE;
}

#endif
