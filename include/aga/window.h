/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WINDOW_H
#define AGA_WINDOW_H

#define AGA_FONT_LIST_BASE (1000)

#include <aga/environment.h>
#include <aga/result.h>

#ifdef _WIN32
# include <aga/sys/win32/windowdata.h>
#else
# include <aga/sys/x/windowdata.h>
#endif

struct aga_keymap {
    aga_bool_t* states;
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
enum aga_result aga_window_device_new(struct aga_window_device*, const char*);
enum aga_result aga_window_device_delete(struct aga_window_device*);

enum aga_result aga_window_device_poll(
		struct aga_window_device*, struct aga_keymap*, struct aga_window*,
		struct aga_pointer*, aga_bool_t*, struct aga_buttons*);

enum aga_result aga_keymap_new(struct aga_keymap*, struct aga_window_device*);
enum aga_result aga_keymap_delete(struct aga_keymap*);

enum aga_result aga_keymap_lookup(struct aga_keymap*, unsigned, aga_bool_t*);

enum aga_result aga_window_new(
		aga_size_t, aga_size_t, struct aga_window_device*, struct aga_window*,
        aga_bool_t, int, char**);

enum aga_result aga_window_delete(
		struct aga_window_device*, struct aga_window*);

/*
 * NOTE: Cursor capture is a somewhat importable concept. As it stands, we
 * 		 Consider the cursor to be captured if mouse deltas continue to be
 * 		 Recorded as the mouse moves indefinitely in one direction and the
 * 		 Cursor does not move outside the window bounds - i.e. sliding the
 * 		 Mouse to the left across your entire desk would continue to report
 * 		 (-1, 0) and would not hover any other Windows to the left of `win'.
 */
enum aga_result aga_window_set_cursor(
		struct aga_window_device*, struct aga_window*, aga_bool_t, aga_bool_t);

enum aga_result aga_window_swap(struct aga_window_device*, struct aga_window*);

enum aga_result aga_dialog(const char*, const char*, aga_bool_t*, aga_bool_t);
enum aga_result aga_dialog_file(char**);
enum aga_result aga_shell_open(const char*);

#endif
