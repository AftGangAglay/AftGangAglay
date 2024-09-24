/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */
#ifndef AGA_X_WINDOWDATA_H
#define AGA_X_WINDOWDATA_H

struct aga_window {
	aga_size_t width, height;

	aga_ulong_t window;
	void* glx;

	aga_bool_t double_buffered;

	aga_ulong_t blank_cursor, arrow_cursor;
};

struct aga_window_device {
	int screen;
	void* display;
	int display_fd;

	aga_ulong_t wm_protocols, wm_delete;

	struct aga_window* capture;
};

#endif
