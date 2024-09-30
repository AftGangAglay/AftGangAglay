/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_RENDER_H
#define AGA_RENDER_H

#include <aga/result.h>
#include <aga/environment.h>

enum aga_result aga_render_text(
		float, float, const float*, const char*);

enum aga_result aga_render_text_format(
		float, float, const float*, const char*, ...);

enum aga_result aga_render_line_graph(
		const float*, aga_size_t, float, const float*);

enum aga_result aga_render_clear(const float*);
enum aga_result aga_render_flush(void);

#endif
