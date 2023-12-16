/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WIN_H
#define AGA_WIN_H

#define AGA_FONT_LIST_BASE (1000)

#include <afeirsa/afeirsa.h>

struct aga_win {
	af_ulong_t xwin;
	void* storage;
	af_size_t width;
	af_size_t height;
};

struct aga_keymap {
	int keysyms_per_keycode;
	int keycode_len;
	int keycode_min;
	af_ulong_t* keymap;
	af_bool_t* keystates;
};

struct aga_pointer {
	int dx, dy;
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
enum af_err aga_mkwinenv(struct aga_winenv* env, const char* display);
enum af_err aga_killctxdpy(struct aga_winenv* env);

enum af_err aga_mkkeymap(struct aga_keymap* keymap, struct aga_winenv* env);
enum af_err aga_killkeymap(struct aga_keymap* keymap);

enum af_err aga_mkwin(
		af_size_t width, af_size_t height, struct aga_winenv* env,
		struct aga_win* win, int argc, char** argv);
enum af_err aga_killwin(struct aga_winenv* env, struct aga_win* win);

enum af_err aga_glctx(struct aga_winenv* env, struct aga_win* win);
enum af_err aga_swapbuf(struct aga_winenv* env, struct aga_win* win);

enum af_err aga_poll(
		struct aga_winenv* env, struct aga_keymap* keymap, struct aga_win* win,
		struct aga_pointer* pointer, af_bool_t* die);

#endif
