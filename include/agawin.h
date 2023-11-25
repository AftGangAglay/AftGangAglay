/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WIN_H
#define AGA_WIN_H

#include <afeirsa/aftypes.h>
#include <afeirsa/aferr.h>

#define AGA_FONT_LIST_BASE (1000)

struct aga_ctx;

struct aga_win {
	af_ulong_t xwin;
	void* storage;
};

struct aga_keymap {
	int keysyms_per_keycode;
	int keycode_len;
	int keycode_min;
	af_ulong_t* keymap;
};

struct aga_winenv {
	/* TODO: Document what fields `agawwin' yoinks. */
	void* dpy;
	int dpy_fd;
	int screen;
	void* glx;
	af_bool_t double_buffered;
	af_ulong_t wm_delete;
};

/*
 * NOTE: Glyphs are generated as display lists corresponding to the ASCII value
 * 		 Of each printable character (i.e. `glCallList('a')')
 */
enum af_err aga_mkctxdpy(struct aga_ctx* ctx, const char* display);
enum af_err aga_killctxdpy(struct aga_ctx* ctx);

enum af_err aga_mkwin(
		struct aga_ctx* ctx, struct aga_win* win, int argc, char** argv);
enum af_err aga_killwin(struct aga_ctx* ctx, struct aga_win* win);

enum af_err aga_glctx(struct aga_ctx* ctx, struct aga_win* win);
enum af_err aga_swapbuf(struct aga_ctx* ctx, struct aga_win* win);

enum af_err aga_poll(struct aga_ctx* ctx);

#endif
