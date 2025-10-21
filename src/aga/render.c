/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/render.h>
#include <aga/draw.h>

#include <asys/varargs.h>
#include <asys/string.h>

#include <mil/system.h>

enum asys_result aga_render_text(
		float x, float y, const float* color, const char* text) {

	enum asys_result result;
	enum aga_draw_flags fl = aga_draw_get();

	if((result = aga_draw_push())) return result;
	if((result = aga_draw_set(AGA_DRAW_NONE))) return result;

	glColor4fv(color);

	glRasterPos2f(x, y);
	if((result = mil_gl_result(__FILE__, "glRasterPos2f"))) return result;

	glListBase(AGA_FONT_LIST_BASE);
	if((result = mil_gl_result(__FILE__, "glListBase"))) return result;

	glCallLists((int) asys_string_length(text), GL_UNSIGNED_BYTE, text);
	if((result = mil_gl_result(__FILE__, "glCallLists"))) return result;

	if((result = aga_draw_pop())) return result;
	return aga_draw_set(fl);
}

enum asys_result aga_render_text_format(
		float x, float y, const float* color, const char* format, ...) {

	static asys_fixed_buffer_t buffer = { 0 };

	enum asys_result result;

	va_list list;

	va_start(list, format);

	/*
	 * TODO: This can pass along the written len to avoid re-iterating the
	 * 		 String.
	 */
	result = asys_string_format_variadic(&buffer, 0, format, list);

	va_end(list);

	if(result) return result;

	return aga_render_text(x, y, color, buffer);
}

enum asys_result aga_render_line_graph(
		const float* heights, asys_size_t count, float width,
		const float* color) {

	enum asys_result result;

	asys_size_t i;
	enum aga_draw_flags fl = aga_draw_get();

	float dx = 1.0f / (float) (count - 1);

	if((result = aga_draw_push())) return result;
	if((result = aga_draw_set(AGA_DRAW_NONE))) return result;

	glLineWidth(width);
	if((result = mil_gl_result(__FILE__, "glLineWidth"))) return result;

	glBegin(GL_LINE_STRIP);
		glColor4fv(color);
		for(i = 0; i < count; ++i) glVertex2f(i * dx, 1.0f - heights[i]);
	glEnd();
	if((result = mil_gl_result(__FILE__, "glEnd"))) return result;

	if((result = aga_draw_pop())) return result;
	return aga_draw_set(fl);
}

enum asys_result aga_render_area(int x, int y, unsigned width, unsigned height) {
	glViewport(x, y, (int) width, (int) height);

	return mil_gl_result(__FILE__, "glViewport");
}

enum asys_result aga_render_clear(const float* color) {
	enum asys_result result;

	if(!color) return ASYS_RESULT_BAD_PARAM;

	glClearColor(color[0], color[1], color[2], color[3]);
	if((result = mil_gl_result(__FILE__, "glClearColor"))) return result;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if((result = mil_gl_result(__FILE__, "glClear"))) return result;

	return ASYS_RESULT_OK;
}

enum asys_result aga_render_flush(void) {
	glFlush();
	return mil_gl_result(__FILE__, "glFlush");
}
