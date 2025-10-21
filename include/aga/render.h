/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_RENDER_H
#define AGA_RENDER_H

#include <asys/result.h>
#include <asys/base.h>

#define AGA_FONT_LIST_BASE (1000)

enum asys_result aga_render_text(
		float, float, const float*, const char*);

enum asys_result aga_render_text_format(
		float, float, const float*, const char*, ...);

enum asys_result aga_render_line_graph(
		const float*, asys_size_t, float, const float*);

enum asys_result aga_render_area(int, int, unsigned, unsigned);
enum asys_result aga_render_clear(const float*);
enum asys_result aga_render_flush(void);

#endif
