/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */
#ifndef AGA_WIN32_WINDOWDATA_H
#define AGA_WIN32_WINDOWDATA_H

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

#endif
