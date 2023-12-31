/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WIN_H
#define AGA_WIN_H

#define AGA_FONT_LIST_BASE (1000)

#include <afeirsa/afeirsa.h>

struct aga_win {
	union {
		af_ulong_t xwin;
		void* hwnd;
	} win;
	void* dc;
	af_size_t width;
	af_size_t height;
	af_ulong_t blank_cursor;
	af_ulong_t arrow_cursor;
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
    int x, y;
};

union aga_winenv {
	struct {
		void* dpy;
		int dpy_fd;
		int screen;
		void* glx;
		af_bool_t double_buffered;
		af_ulong_t wm_delete;
	} x;
	struct {
		void* module;
		int class;
		void* wgl;
		void* cursor;
		af_bool_t visible;
		af_bool_t captured;
	} win32;
};

/*
 * NOTE: Glyphs are generated as display lists corresponding to the ASCII value
 * 		 Of each printable character (i.e. `glCallList('a')')
 */
enum af_err aga_mkwinenv(union aga_winenv* env, const char* display);
enum af_err aga_killwinenv(union aga_winenv* env);

enum af_err aga_mkkeymap(struct aga_keymap* keymap, union aga_winenv* env);
enum af_err aga_killkeymap(struct aga_keymap* keymap);

enum af_err aga_mkwin(
		af_size_t width, af_size_t height, union aga_winenv* env,
		struct aga_win* win, int argc, char** argv);
enum af_err aga_killwin(union aga_winenv* env, struct aga_win* win);

/*
 * NOTE: Cursor capture is a somewhat importable concept. As it stands, we
 * 		 Consider the cursor to be captured if mouse deltas continue to be
 * 		 Recorded as the mouse moves indefinitely in one direction and the
 * 		 Cursor does not move outside the Window bounds - i.e. sliding the
 * 		 Mouse to the left across your entire desk would continue to report
 * 		 (-1, 0) and would not hover any other Windows to the left of `win'.
 */
enum af_err aga_setcursor(
		union aga_winenv* env, struct aga_win* win, af_bool_t visible,
		af_bool_t captured);

enum af_err aga_glctx(union aga_winenv* env, struct aga_win* win);
enum af_err aga_swapbuf(union aga_winenv* env, struct aga_win* win);

enum af_err aga_poll(
		union aga_winenv* env, struct aga_keymap* keymap, struct aga_win* win,
		struct aga_pointer* pointer, af_bool_t* die);

enum af_err aga_diag(
		const char* message, const char* title, af_bool_t* response,
		af_bool_t is_error);

enum af_err aga_shellopen(const char* uri);

#endif
