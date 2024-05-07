/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WIN_H
#define AGA_WIN_H

#define AGA_FONT_LIST_BASE (1000)

#include <agaenv.h>
#include <agaresult.h>

#ifdef _WIN32
struct aga_win {
	void* hwnd;
	void* dc;
	aga_size_t width, height;

	int client_off_x, client_off_y;
};

struct aga_keymap {
	aga_bool_t* keystates;
};

struct aga_winenv {
	void* module;
	void* wgl;
	void* cursor;
	int class;
	aga_bool_t visible, captured;
};
#else
struct aga_win {
	aga_size_t width, height;
	aga_ulong_t xwin;
	aga_ulong_t blank_cursor, arrow_cursor;
};

struct aga_keymap {
	aga_ulong_t* keymap;
	aga_bool_t* keystates;
	int keysyms_per_keycode, keycode_len, keycode_min;
};

struct aga_winenv {
	void* dpy;
	void* glx;
	aga_ulong_t wm_delete;
	int dpy_fd;
	int screen;
	aga_bool_t double_buffered;
};
#endif

enum aga_button_state {
	AGA_BUTTON_UP,
	AGA_BUTTON_DOWN,
	AGA_BUTTON_CLICK /* Became held this frame. */
};

enum aga_button {
	AGA_BUTTON_LEFT,
	AGA_BUTTON_RIGHT,
	AGA_BUTTON_MIDDLE,

	AGA_BUTTON_MAX
};

struct aga_buttons {
	enum aga_button_state states[AGA_BUTTON_MAX];
};

struct aga_pointer {
	int dx, dy;
	int x, y;
};

/*
 * NOTE: Glyphs are generated as display lists corresponding to the ASCII value
 * 		 Of each printable character (i.e. `glCallList('a')')
 */
enum aga_result aga_mkwinenv(struct aga_winenv*, const char*);

enum aga_result aga_killwinenv(struct aga_winenv*);

enum aga_result aga_mkkeymap(struct aga_keymap*, struct aga_winenv*);

enum aga_result aga_killkeymap(struct aga_keymap*);

enum aga_result aga_mkwin(
		aga_size_t, aga_size_t, struct aga_winenv*, struct aga_win*, int,
		char**);

enum aga_result aga_killwin(struct aga_winenv*, struct aga_win*);

enum aga_result aga_keylook(struct aga_keymap*, aga_uint8_t, aga_bool_t*);

/*
 * NOTE: Cursor capture is a somewhat importable concept. As it stands, we
 * 		 Consider the cursor to be captured if mouse deltas continue to be
 * 		 Recorded as the mouse moves indefinitely in one direction and the
 * 		 Cursor does not move outside the Window bounds - i.e. sliding the
 * 		 Mouse to the left across your entire desk would continue to report
 * 		 (-1, 0) and would not hover any other Windows to the left of `win'.
 */
enum aga_result aga_setcursor(
		struct aga_winenv*, struct aga_win*, aga_bool_t, aga_bool_t);

enum aga_result aga_glctx(struct aga_winenv*, struct aga_win*);

enum aga_result aga_swapbuf(struct aga_winenv*, struct aga_win*);

enum aga_result aga_poll(
		struct aga_winenv*, struct aga_keymap*, struct aga_win*,
		struct aga_pointer*, aga_bool_t*, struct aga_buttons*);

enum aga_result aga_diag(const char*, const char*, aga_bool_t*, aga_bool_t);

enum aga_result aga_filediag(char**);

enum aga_result aga_shellopen(const char*);

#endif
