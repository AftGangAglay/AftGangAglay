/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agadraw.h>
#include <agawin.h>
#include <agalog.h>
#include <agastd.h>
#include <agaerr.h>
#include <agagl.h>

static enum aga_drawflags aga_global_drawflags = 0;

enum aga_result aga_setdraw(enum aga_drawflags flags) {
	static const char* name[] = { "glDisable", "glEnable" };
#ifdef _WIN32
	static void (APIENTRY *func[2])(GLenum);
#else
	static void (*func[2])(GLenum);
#endif
	static struct {
		enum aga_drawflags flag;
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
	if((result = aga_gl_error(__FILE__, "glShadeModel"))) return result;

	flags &= ~AGA_DRAW_FLAT;

	for(i = 0; i < AGA_LEN(flag); ++i) {
		aga_bool_t x = !!(flags & flag[i].flag);
		func[x](flag[i].cap);
		if((result = aga_gl_error(__FILE__, name[x]))) return result;
	}

	aga_global_drawflags = flags;

	return AGA_RESULT_OK;
}

enum aga_drawflags aga_getdraw(void) {
	return aga_global_drawflags;
}

enum aga_result aga_pushall_ident(void) {
	enum aga_result result;

	glMatrixMode(GL_MODELVIEW);
	if((result = aga_gl_error(__FILE__, "glMatrixMode"))) return result;

	glPushMatrix();
	if((result = aga_gl_error(__FILE__, "glPushMatrix"))) return result;

	glLoadIdentity();
	if((result = aga_gl_error(__FILE__, "glLoadIdentity"))) return result;

	/* TODO: This shouldn't be here but it makes text show up as expected? */
	glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
	if((result = aga_gl_error(__FILE__, "glOrtho"))) return result;

	glMatrixMode(GL_PROJECTION);
	if((result = aga_gl_error(__FILE__, "glMatrixMode"))) return result;

	glPushMatrix();
	if((result = aga_gl_error(__FILE__, "glPushMatrix"))) return result;

	glLoadIdentity();
	if((result = aga_gl_error(__FILE__, "glLoadIdentity"))) return result;

	return AGA_RESULT_OK;
}

enum aga_result aga_popall(void) {
	enum aga_result result;

	glMatrixMode(GL_MODELVIEW);
	if((result = aga_gl_error(__FILE__, "glMatrixMode"))) return result;

	glPopMatrix();
	if((result = aga_gl_error(__FILE__, "glPushMatrix"))) return result;

	glMatrixMode(GL_PROJECTION);
	if((result = aga_gl_error(__FILE__, "glMatrixMode"))) return result;

	glPopMatrix();
	if((result = aga_gl_error(__FILE__, "glPushMatrix"))) return result;

	return AGA_RESULT_OK;
}

enum aga_result aga_puttext(float x, float y, const char* text) {
	enum aga_result result;
	enum aga_drawflags fl = aga_getdraw();

	if((result = aga_pushall_ident())) return result;
	if((result = aga_setdraw(AGA_DRAW_NONE))) return result;

	/* TODO: Proper text parameters. */
	glColor3f(1.0f, 1.0f, 1.0f);

	glRasterPos2f(x, y);
	if((result = aga_gl_error(__FILE__, "glRasterPos2f"))) return result;

	glListBase(AGA_FONT_LIST_BASE);
	if((result = aga_gl_error(__FILE__, "glListBase"))) return result;

	glCallLists((int) strlen(text), GL_UNSIGNED_BYTE, text);
	if((result = aga_gl_error(__FILE__, "glCallLists"))) return result;

	if((result = aga_popall())) return result;

	return aga_setdraw(fl);
}

enum aga_result aga_puttextfmt(float x, float y, const char* fmt, ...) {
	aga_fixed_buf_t buf = { 0 };
	va_list ap;

	va_start(ap, fmt);
	if(vsprintf(buf, fmt, ap) < 0) {
		va_end(ap);
		return aga_errno(__FILE__, "vsprintf");
	}
	va_end(ap);

	return aga_puttext(x, y, buf);
}

enum aga_result aga_clear(const float* col) {
	enum aga_result result;

	if(!col) return AGA_RESULT_BAD_PARAM;

	glClearColor(col[0], col[1], col[2], col[3]);
	result = aga_gl_error(__FILE__, "glClearColor");
	if(result) return result;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	result = aga_gl_error(__FILE__, "glClear");
	if(result) return result;

	return AGA_RESULT_OK;
}

enum aga_result aga_flush(void) {
	glFlush();
	return aga_gl_error(__FILE__, "glFlush");
}

static enum aga_result aga_gl_result(aga_uint_t err) {
	switch(err) {
		default: return AGA_RESULT_ERROR;
		case GL_INVALID_ENUM: return AGA_RESULT_BAD_TYPE;
		case GL_INVALID_VALUE: return AGA_RESULT_BAD_PARAM;
		case GL_INVALID_OPERATION: return AGA_RESULT_BAD_OP;
		case GL_OUT_OF_MEMORY: return AGA_RESULT_OOM;
		case GL_STACK_UNDERFLOW: return AGA_RESULT_OOM;
		case GL_STACK_OVERFLOW: return AGA_RESULT_OOM;
	}
}

enum aga_result aga_gl_error(const char* loc, const char* proc) {
	enum aga_result err = AGA_RESULT_OK;

	unsigned res;

	while((res = glGetError())) {
		err = aga_gl_result(res);
		if(loc) { /* Null `loc' acts to clear the GL error state. */
			const char* str = (const char*) gluErrorString(res);
			if(!str) str = "unknown error";
			aga_log(loc, "err: %s: %s", proc, str);
		}
	}

	return err;
}
