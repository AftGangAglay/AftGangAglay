/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WINDOW_H
#define AGA_WINDOW_H

/*
 * TODO: Apple System 8.1 appears to be the earliest macOS to support OpenGL.
 */

#define AGA_FONT_LIST_BASE (1000)

#include <asys/base.h>
#include <asys/result.h>

#ifdef ASYS_WIN32
struct aga_window {
	void* hwnd;
    void* wgl;
	asys_size_t width, height;
};

struct aga_window_device {
	void* cursor;
	asys_bool_t visible, captured;

	struct aga_window* capture;

	int caption_height;
	int border_sizeable_x, border_sizeable_y;
	int border_x, border_y;
};
#else
typedef unsigned long aga_xid_t;

struct aga_window {
	asys_size_t width, height;

	aga_xid_t window;
	void* glx;

	asys_bool_t double_buffered;

	aga_xid_t blank_cursor, arrow_cursor;
};

struct aga_window_device {
	int screen;
	void* display;

	aga_xid_t wm_protocols, wm_delete;

	struct aga_window* capture;
};
#endif

struct asys_main_data;

struct aga_keymap {
    asys_bool_t* states;
};

/* TODO: Use this for keystrokes and button presses. */
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
 * 		 Of each printable character (i.e. `glCallList('a')') with the list
 * 		 Base `AGA_FONT_LIST_BASE'.
 */
enum asys_result aga_window_device_new(struct aga_window_device*, const char*);
enum asys_result aga_window_device_delete(struct aga_window_device*);

enum asys_result aga_window_device_poll(
		struct aga_window_device*, struct aga_keymap*, struct aga_window*,
		struct aga_pointer*, asys_bool_t*, struct aga_buttons*);

enum asys_result aga_keymap_new(struct aga_keymap*, struct aga_window_device*);
enum asys_result aga_keymap_delete(struct aga_keymap*);

enum asys_result aga_keymap_lookup(struct aga_keymap*, unsigned, asys_bool_t*);

enum asys_result aga_window_new(
		asys_size_t, asys_size_t, const char*,
		struct aga_window_device*, struct aga_window*,
        asys_bool_t, struct asys_main_data*);

enum asys_result aga_window_delete(
		struct aga_window_device*, struct aga_window*);

enum asys_result aga_window_select(
		struct aga_window_device*, struct aga_window*);

/*
 * NOTE: Cursor capture is a somewhat importable concept. As it stands, we
 * 		 Consider the cursor to be captured if mouse deltas continue to be
 * 		 Recorded as the mouse moves indefinitely in one direction and the
 * 		 Cursor does not move outside the window bounds - i.e. sliding the
 * 		 Mouse to the left across your entire desk would continue to report
 * 		 (-1, 0) and would not hover any other Windows to the left of `win'.
 */
enum asys_result aga_window_set_cursor(
		struct aga_window_device*, struct aga_window*, asys_bool_t,
		asys_bool_t);

enum asys_result aga_window_swap(
		struct aga_window_device*, struct aga_window*);

enum asys_result aga_dialog(
		const char*, const char*, asys_bool_t*, asys_bool_t);

enum asys_result aga_dialog_file(char**);
enum asys_result aga_shell_open(const char*);

#endif
