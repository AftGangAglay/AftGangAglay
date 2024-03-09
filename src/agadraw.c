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

enum aga_result aga_setdrawparam(void) {
	enum aga_result result;

	glEnable(GL_CULL_FACE);
	result = aga_gl_error(__FILE__, "glEnable");
	if(result) return result;

	glEnable(GL_BLEND);
	result = aga_gl_error(__FILE__, "glEnable");
	if(result) return result;

	glEnable(GL_FOG);
	result = aga_gl_error(__FILE__, "glEnable");
	if(result) return result;

	glEnable(GL_TEXTURE_2D);
	result = aga_gl_error(__FILE__, "glEnable");
	if(result) return result;

	/*
	glEnable(GL_LIGHTING);
	AGA_GL_CHK("glEnable");
	 */

	glEnable(GL_DEPTH_TEST);
	result = aga_gl_error(__FILE__, "glEnable");
	if(result) return result;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return aga_gl_error(__FILE__, "glBlendFunc");
}

enum aga_result aga_pushrawdraw(void) {
	enum aga_result result;

	glDisable(GL_TEXTURE_2D);
	result = aga_gl_error(__FILE__, "glDisable");
	if(result) return result;

	/*
	glDisable(GL_LIGHTING);
	AGA_GL_CHK("glDisable");
	 */

	glDisable(GL_DEPTH_TEST);
	result = aga_gl_error(__FILE__, "glDisable");
	if(result) return result;

	glMatrixMode(GL_MODELVIEW);
	result = aga_gl_error(__FILE__, "glMatrixMode");
	if(result) return result;

	glPushMatrix();
	result = aga_gl_error(__FILE__, "glPushMatrix");
	if(result) return result;

	glLoadIdentity();
	result = aga_gl_error(__FILE__, "glLoadIdentity");
	if(result) return result;

	glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
	result = aga_gl_error(__FILE__, "glOrtho");
	if(result) return result;

	glMatrixMode(GL_PROJECTION);
	result = aga_gl_error(__FILE__, "glMatrixMode");
	if(result) return result;

	glPushMatrix();
	result = aga_gl_error(__FILE__, "glPushMatrix");
	if(result) return result;

	glLoadIdentity();
	result = aga_gl_error(__FILE__, "glLoadIdentity");
	if(result) return result;

	return AGA_RESULT_OK;
}

enum aga_result aga_poprawdraw(void) {
	enum aga_result result;

	glMatrixMode(GL_MODELVIEW);
	result = aga_gl_error(__FILE__, "glMatrixMode");
	if(result) return result;

	glPopMatrix();
	result = aga_gl_error(__FILE__, "glPopMatrix");
	if(result) return result;

	glMatrixMode(GL_PROJECTION);
	result = aga_gl_error(__FILE__, "glMatrixMode");
	if(result) return result;

	glPopMatrix();
	result = aga_gl_error(__FILE__, "glPopMatrix");
	if(result) return result;

	glEnable(GL_TEXTURE_2D);
	result = aga_gl_error(__FILE__, "glEnable");
	if(result) return result;

	/*
	glEnable(GL_LIGHTING);
	AGA_GL_CHK("glEnable");
	 */

	glEnable(GL_DEPTH_TEST);
	result = aga_gl_error(__FILE__, "glEnable");
	if(result) return result;

	return AGA_RESULT_OK;
}

enum aga_result aga_puttext(float x, float y, const char* text) {
	enum aga_result result;

	result = aga_pushrawdraw();
	if(result) return result;

	/* TODO: Proper text parameters. */
	glColor3f(1.0f, 1.0f, 1.0f);

	glRasterPos2f(x, y);
	result = aga_gl_error(__FILE__, "glRasterPos2f");
	if(result) return result;

	glListBase(AGA_FONT_LIST_BASE);
	result = aga_gl_error(__FILE__, "glListBase");
	if(result) return result;

	glCallLists((int) strlen(text), GL_UNSIGNED_BYTE, text);
	result = aga_gl_error(__FILE__, "glCallLists");
	if(result) return result;

	return aga_poprawdraw();
}

enum aga_result aga_puttextfmt(float x, float y, const char* fmt, ...) {
	aga_fixed_buf_t buf = { 0 };
	va_list l;

	va_start(l, fmt);
	if(vsprintf(buf, fmt, l) < 0) return aga_errno(__FILE__, "vsprintf");
	va_end(l);

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

enum aga_result aga_gl_error(const char* loc, const char* proc) {
	enum aga_result err = AGA_RESULT_OK;

	unsigned res;

	while((res = glGetError())) {
		switch(res) {
			default: err = AGA_RESULT_ERROR;
				break;
			case GL_INVALID_ENUM: err = AGA_RESULT_BAD_PARAM;
				break;
			case GL_INVALID_VALUE: err = AGA_RESULT_BAD_PARAM;
				break;
			case GL_INVALID_OPERATION: err = AGA_RESULT_BAD_OP;
				break;
			case GL_OUT_OF_MEMORY: err = AGA_RESULT_OOM;
				break;
			case GL_STACK_UNDERFLOW: err = AGA_RESULT_OOM;
				break;
			case GL_STACK_OVERFLOW: err = AGA_RESULT_OOM;
				break;
		}

		if(loc) { /* Null `loc' acts to clear the GL error state. */
			const char* str = (const char*) gluErrorString(res);
			aga_log(loc, "err: %s: %s", proc, str);
		}
	}

	return err;
}
