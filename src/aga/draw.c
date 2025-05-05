/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

/*
 * There are some interesting techniques here and in parents:
 * https://www.opengl.org/archives/resources/code/samples/advanced/advanced97/notes/node60.html.
 */

#include <aga/draw.h>

#include <asys/log.h>
#include <asys/string.h>

#include <mil/system.h>

static enum aga_draw_flags aga_global_draw_flags = 0;

enum asys_result aga_draw_set(enum aga_draw_flags flags) {
	static const char* name[] = { "glDisable", "glEnable" };
#ifdef ASYS_WIN32
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

	enum asys_result result;
	asys_size_t i;

	func[0] = glDisable;
	func[1] = glEnable;

	glShadeModel((flags & AGA_DRAW_FLAT) ? GL_FLAT : GL_SMOOTH);
	if((result = mil_gl_result(__FILE__, "glShadeModel"))) return result;

	result = aga_draw_fidelity(!!(flags & AGA_DRAW_FIDELITY));
	if(result) return result;

	for(i = 0; i < ASYS_LENGTH(flag); ++i) {
		asys_bool_t x = !!(flags & flag[i].flag);
		func[x](flag[i].cap);
		if((result = mil_gl_result(__FILE__, name[x]))) return result;
	}

	aga_global_draw_flags = flags;

	return ASYS_RESULT_OK;
}

enum aga_draw_flags aga_draw_get(void) {
	return aga_global_draw_flags;
}

enum asys_result aga_draw_push(void) {
	enum asys_result result;

	glMatrixMode(GL_MODELVIEW);
	if((result = mil_gl_result(__FILE__, "glMatrixMode"))) return result;

	glPushMatrix();
	if((result = mil_gl_result(__FILE__, "glPushMatrix"))) return result;

	glLoadIdentity();
	if((result = mil_gl_result(__FILE__, "glLoadIdentity"))) return result;

	/* TODO: This shouldn't be here but it makes text show up as expected? */
	glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
	if((result = mil_gl_result(__FILE__, "glOrtho"))) return result;

	glMatrixMode(GL_PROJECTION);
	if((result = mil_gl_result(__FILE__, "glMatrixMode"))) return result;

	glPushMatrix();
	if((result = mil_gl_result(__FILE__, "glPushMatrix"))) return result;

	glLoadIdentity();
	if((result = mil_gl_result(__FILE__, "glLoadIdentity"))) return result;

	return ASYS_RESULT_OK;
}

enum asys_result aga_draw_pop(void) {
	enum asys_result result;

	glMatrixMode(GL_MODELVIEW);
	if((result = mil_gl_result(__FILE__, "glMatrixMode"))) return result;

	glPopMatrix();
	if((result = mil_gl_result(__FILE__, "glPushMatrix"))) return result;

	glMatrixMode(GL_PROJECTION);
	if((result = mil_gl_result(__FILE__, "glMatrixMode"))) return result;

	glPopMatrix();
	if((result = mil_gl_result(__FILE__, "glPushMatrix"))) return result;

	return ASYS_RESULT_OK;
}

enum asys_result aga_draw_fidelity(asys_bool_t hq) {
	static const unsigned targets[] = {
			GL_PERSPECTIVE_CORRECTION_HINT,
			GL_POINT_SMOOTH_HINT,
			GL_LINE_SMOOTH_HINT,
			GL_POLYGON_SMOOTH_HINT,
			GL_FOG_HINT
	};

	enum asys_result result;
	asys_size_t i;

	for(i = 0; i < ASYS_LENGTH(targets); ++i) {
		glHint(targets[i], hq ? GL_NICEST : GL_FASTEST);
		if((result = mil_gl_result(__FILE__, "glHint"))) return result;
	}

	return ASYS_RESULT_OK;
}

enum asys_result aga_renderer_string(const char** out) {
	static asys_fixed_buffer_t buffer = { 0 };

	enum asys_result result;

	const char* version;
	const char* vendor;
	const char* renderer;

	version = (const char*) glGetString(GL_VERSION);
	if(!version) return mil_gl_result(__FILE__, "glGetString");

	vendor = (const char*) glGetString(GL_VENDOR);
	if(!vendor) return mil_gl_result(__FILE__, "glGetString");

	renderer = (const char*) glGetString(GL_RENDERER);
	if(!renderer) return mil_gl_result(__FILE__, "glGetString");

	result = asys_string_format(
			&buffer, 0, "%s %s %s", version, vendor, renderer);

	if(result) return result;

	*out = buffer;

	return ASYS_RESULT_OK;
}
