/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_INPUT_H
#define AGA_INPUT_H

/*
 * TODO: Legacy input API is deprecated and will be removed in the next major
 * 		 Release in favour of direct script-dispatched input events.
 * 		 This header was added after the MIL migration to keep compat.
 */

#include <asys/base.h>

#include <mil/base.h>

#ifdef ASYS_WIN32
# define AGA_KEY_MAX (0xFF)
#else
# define AGA_KEY_MAX (0xFFFF)
#endif

struct aga_keymap {
	asys_bool_t states[AGA_KEY_MAX + 1];
};

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
	asys_bool_t dirty;
	asys_bool_t captured;

	asys_bool_t warping;
	asys_bool_t warp_gt;
	asys_bool_t warp_x;
	int warp_coord;

	int dx, dy;
	int x, y;
};

struct aga_input_pack {
	struct aga_buttons buttons;
	struct aga_pointer pointer;
	struct aga_keymap keymap;
};

struct mil_input_data;

void aga_wrap_pointer(
		struct mil_ctx*, mil_widget_t, struct aga_pointer*, int, int);

void aga_translate_mil_input(struct mil_input_data*, struct aga_input_pack*);

void aga_main_window_input(mil_widget_t, struct mil_ctx*, void*);

#endif
