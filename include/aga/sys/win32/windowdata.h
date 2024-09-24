/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */
#ifndef AGA_WIN32_WINDOWDATA_H
#define AGA_WIN32_WINDOWDATA_H

struct aga_window {
	void* hwnd;
    void* wgl;
	aga_size_t width, height;

	int client_off_x, client_off_y;
};

/* TODO: Move event state output to window device? */
struct aga_window_device {
	void* module;
	void* cursor;
	int class;
	aga_bool_t visible, captured;
};

#endif
