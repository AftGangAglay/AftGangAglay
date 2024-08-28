/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/render.h>
#include <aga/draw.h>
#include <aga/window.h>
#include <aga/gl.h>
#include <aga/utility.h>
#include <aga/std.h>
#include <aga/error.h>

enum aga_result aga_render_text(float x, float y, const char* text) {
	enum aga_result result;
	enum aga_draw_flags fl = aga_draw_get();

	if((result = aga_draw_push())) return result;
	if((result = aga_draw_set(AGA_DRAW_NONE))) return result;

	/* TODO: Proper text parameters. */
	glColor3f(1.0f, 1.0f, 1.0f);

	glRasterPos2f(x, y);
	if((result = aga_error_gl(__FILE__, "glRasterPos2f"))) return result;

	glListBase(AGA_FONT_LIST_BASE);
	if((result = aga_error_gl(__FILE__, "glListBase"))) return result;

	glCallLists((int) aga_strlen(text), GL_UNSIGNED_BYTE, text);
	if((result = aga_error_gl(__FILE__, "glCallLists"))) return result;

	if((result = aga_draw_pop())) return result;

	return aga_draw_set(fl);
}

enum aga_result aga_render_text_format(
		float x, float y, const char* fmt, ...) {

	aga_fixed_buf_t buf = { 0 };
	va_list ap;

	va_start(ap, fmt);
	if(vsprintf(buf, fmt, ap) < 0) {
		va_end(ap);
		return aga_error_system(__FILE__, "vsprintf");
	}
	va_end(ap);

	return aga_render_text(x, y, buf);
}

enum aga_result aga_render_clear(const float* col) {
	enum aga_result result;

	if(!col) return AGA_RESULT_BAD_PARAM;

	glClearColor(col[0], col[1], col[2], col[3]);
	result = aga_error_gl(__FILE__, "glClearColor");
	if(result) return result;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	result = aga_error_gl(__FILE__, "glClear");
	if(result) return result;

	return AGA_RESULT_OK;
}

enum aga_result aga_render_flush(void) {
	glFlush();
	return aga_error_gl(__FILE__, "glFlush");
}
