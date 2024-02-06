/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WIN_H
#define AGA_WIN_H

#define AGA_FONT_LIST_BASE (1000)

#include <agaresult.h>

#ifdef AF_GLXABI
struct aga_win {
	af_size_t width, height;
	af_ulong_t xwin;
	af_ulong_t blank_cursor, arrow_cursor;
};

struct aga_keymap {
	af_ulong_t* keymap;
	af_bool_t* keystates;
	int keysyms_per_keycode, keycode_len, keycode_min;
};

struct aga_winenv {
	void* dpy;
	void* glx;
	af_ulong_t wm_delete;
	int dpy_fd;
	int screen;
	af_bool_t double_buffered;
};
#elif defined(AF_WGL)
struct aga_win {
	void* hwnd;
	void* dc;
	af_size_t width, height;
};

/*
 * TODO: Make a key lookup function to remove redundant `keysyms_per_keycode'
 * 		 and `keycode_len' members here.
 */
struct aga_keymap {
	af_ulong_t* keymap;
	af_bool_t* keystates;
	int keysyms_per_keycode, keycode_len;
};

struct aga_winenv {
	void* module;
	void* wgl;
	void* cursor;
	int class;
	af_bool_t visible, captured;
};
#endif

struct aga_pointer {
	int dx, dy;
    int x, y;
};

/*
 * NOTE: Glyphs are generated as display lists corresponding to the ASCII value
 * 		 Of each printable character (i.e. `glCallList('a')')
 */
enum af_err aga_mkwinenv(struct aga_winenv* env, const char* display);
enum af_err aga_killwinenv(struct aga_winenv* env);

enum af_err aga_mkkeymap(struct aga_keymap* keymap, struct aga_winenv* env);
enum af_err aga_killkeymap(struct aga_keymap* keymap);

enum af_err aga_mkwin(
		af_size_t width, af_size_t height, struct aga_winenv* env,
		struct aga_win* win, int argc, char** argv);
enum af_err aga_killwin(struct aga_winenv* env, struct aga_win* win);

/*
 * NOTE: Cursor capture is a somewhat importable concept. As it stands, we
 * 		 Consider the cursor to be captured if mouse deltas continue to be
 * 		 Recorded as the mouse moves indefinitely in one direction and the
 * 		 Cursor does not move outside the Window bounds - i.e. sliding the
 * 		 Mouse to the left across your entire desk would continue to report
 * 		 (-1, 0) and would not hover any other Windows to the left of `win'.
 */
enum af_err aga_setcursor(
		struct aga_winenv* env, struct aga_win* win, af_bool_t visible,
		af_bool_t captured);

enum af_err aga_glctx(struct aga_winenv* env, struct aga_win* win);
enum af_err aga_swapbuf(struct aga_winenv* env, struct aga_win* win);

enum af_err aga_poll(
		struct aga_winenv* env, struct aga_keymap* keymap, struct aga_win* win,
		struct aga_pointer* pointer, af_bool_t* die);

enum af_err aga_diag(
		const char* message, const char* title, af_bool_t* response,
		af_bool_t is_error);

enum af_err aga_shellopen(const char* uri);

#endif
