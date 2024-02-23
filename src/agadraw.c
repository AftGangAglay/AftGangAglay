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

/* TODO: Do error checking on all matrix ops - we're missing a lot of it. */

enum aga_result aga_setdrawparam(void) {
	glEnable(GL_CULL_FACE);
	AGA_GL_CHK("glEnable");

	glEnable(GL_BLEND);
    AGA_GL_CHK("glEnable");

	glEnable(GL_FOG);
    AGA_GL_CHK("glEnable");

	glEnable(GL_TEXTURE_2D);
	AGA_GL_CHK("glEnable");

	/*
	glEnable(GL_LIGHTING);
	AGA_GL_CHK("glEnable");
	 */

	glEnable(GL_DEPTH_TEST);
	AGA_GL_CHK("glEnable");

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    AGA_GL_CHK("glBlendFunc");

    return AGA_RESULT_OK;
}

/* TODO: Make more distinct "pipeline modes" rather than random en/disable? */
enum aga_result aga_pushrawdraw(void) {
	glDisable(GL_TEXTURE_2D);
    AGA_GL_CHK("glDisable");

	glDisable(GL_LIGHTING);
    AGA_GL_CHK("glDisable");

	glDisable(GL_DEPTH_TEST);
    AGA_GL_CHK("glDisable");

	glMatrixMode(GL_MODELVIEW);
    AGA_GL_CHK("glMatrixMode");
		glPushMatrix();
        AGA_GL_CHK("glPushMatrix");
		glLoadIdentity();
        AGA_GL_CHK("glLoadIdentity");
		glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
        AGA_GL_CHK("glOrtho");

	glMatrixMode(GL_PROJECTION);
    AGA_GL_CHK("glMatrixMode");
		glPushMatrix();
        AGA_GL_CHK("glPushMatrix");
		glLoadIdentity();
        AGA_GL_CHK("glLoadIdentity");

	return AGA_RESULT_OK;
}

enum aga_result aga_poprawdraw(void) {
	glMatrixMode(GL_MODELVIEW);
    AGA_GL_CHK("glMatrixMode");
		glPopMatrix();
        AGA_GL_CHK("glPopMatrix");

	glMatrixMode(GL_PROJECTION);
    AGA_GL_CHK("glMatrixMode");
		glPopMatrix();
        AGA_GL_CHK("glPopMatrix");

	glEnable(GL_TEXTURE_2D);
    AGA_GL_CHK("glEnable");

	glEnable(GL_LIGHTING);
    AGA_GL_CHK("glEnable");

	glEnable(GL_DEPTH_TEST);
    AGA_GL_CHK("glEnable");

	return AGA_RESULT_OK;
}

enum aga_result aga_puttext(float x, float y, const char* text) {
	AGA_CHK(aga_pushrawdraw());

    /* TODO: Proper text parameters. */
	glColor3f(1.0f, 1.0f, 1.0f);

	glRasterPos2f(x, y);
    AGA_GL_CHK("glRasterPos2f");

	glListBase(AGA_FONT_LIST_BASE);
    AGA_GL_CHK("glListBase");

	glCallLists((int) strlen(text), GL_UNSIGNED_BYTE, text);
    AGA_GL_CHK("glCallLists");

	AGA_CHK(aga_poprawdraw());

	return AGA_RESULT_OK;
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
    AGA_PARAM_CHK(col);

    glClearColor(col[0], col[1], col[2], col[3]);
    AGA_GL_CHK("glClearColor");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    AGA_GL_CHK("glClear");

    return AGA_RESULT_OK;
}

enum aga_result aga_flush(void) {
    glFlush();
    AGA_GL_CHK("glFlush");

    return AGA_RESULT_OK;
}

enum aga_result aga_glerr(const char* loc, const char* proc) {
    enum aga_result err = AGA_RESULT_OK;

    unsigned res;

    while((res = glGetError())) {
        err = AGA_RESULT_ERROR; /* TODO: Translate GL error. */

        if(loc) { /* Null `loc' acts to clear the GL error state. */
			const char* str = (const char*) gluErrorString(res);
			aga_log(loc, "err: %s: %s", proc, str);
		}
    }

    return err;
}
