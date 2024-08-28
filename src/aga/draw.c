/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/draw.h>
#include <aga/window.h>
#include <aga/log.h>
#include <aga/std.h>
#include <aga/error.h>
#include <aga/gl.h>

static enum aga_draw_flags aga_global_drawflags = 0;

enum aga_result aga_draw_set(enum aga_draw_flags flags) {
	static const char* name[] = { "glDisable", "glEnable" };
#ifdef _WIN32
	static void (APIENTRY *func[2])(GLenum);
#else
	static void (*func[2])(GLenum);
#endif
	static struct {
		enum aga_draw_flags flag;
		GLenum cap;
	} flag[] = {
			{ AGA_DRAW_BACKFACE, GL_CULL_FACE },
			{ AGA_DRAW_BLEND, GL_BLEND },
			{ AGA_DRAW_FOG, GL_FOG },
			{ AGA_DRAW_TEXTURE, GL_TEXTURE_2D },
			{ AGA_DRAW_LIGHTING, GL_LIGHTING },
			{ AGA_DRAW_DEPTH, GL_DEPTH_TEST }
	};

	enum aga_result result;
	aga_size_t i;

	func[0] = glDisable;
	func[1] = glEnable;

	glShadeModel((flags & AGA_DRAW_FLAT) ? GL_FLAT : GL_SMOOTH);
	if((result = aga_error_gl(__FILE__, "glShadeModel"))) return result;

	flags &= ~AGA_DRAW_FLAT;

	for(i = 0; i < AGA_LEN(flag); ++i) {
		aga_bool_t x = !!(flags & flag[i].flag);
		func[x](flag[i].cap);
		if((result = aga_error_gl(__FILE__, name[x]))) return result;
	}

	aga_global_drawflags = flags;

	return AGA_RESULT_OK;
}

enum aga_draw_flags aga_draw_get(void) {
	return aga_global_drawflags;
}

enum aga_result aga_draw_push(void) {
	enum aga_result result;

	glMatrixMode(GL_MODELVIEW);
	if((result = aga_error_gl(__FILE__, "glMatrixMode"))) return result;

	glPushMatrix();
	if((result = aga_error_gl(__FILE__, "glPushMatrix"))) return result;

	glLoadIdentity();
	if((result = aga_error_gl(__FILE__, "glLoadIdentity"))) return result;

	/* TODO: This shouldn't be here but it makes text show up as expected? */
	glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
	if((result = aga_error_gl(__FILE__, "glOrtho"))) return result;

	glMatrixMode(GL_PROJECTION);
	if((result = aga_error_gl(__FILE__, "glMatrixMode"))) return result;

	glPushMatrix();
	if((result = aga_error_gl(__FILE__, "glPushMatrix"))) return result;

	glLoadIdentity();
	if((result = aga_error_gl(__FILE__, "glLoadIdentity"))) return result;

	return AGA_RESULT_OK;
}

enum aga_result aga_draw_pop(void) {
	enum aga_result result;

	glMatrixMode(GL_MODELVIEW);
	if((result = aga_error_gl(__FILE__, "glMatrixMode"))) return result;

	glPopMatrix();
	if((result = aga_error_gl(__FILE__, "glPushMatrix"))) return result;

	glMatrixMode(GL_PROJECTION);
	if((result = aga_error_gl(__FILE__, "glMatrixMode"))) return result;

	glPopMatrix();
	if((result = aga_error_gl(__FILE__, "glPushMatrix"))) return result;

	return AGA_RESULT_OK;
}
