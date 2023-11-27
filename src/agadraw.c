/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agadraw.h>
#include <agastd.h>

#include <afeirsa/afgl.h>

enum af_err aga_setdrawparam(void) {
	glEnable(GL_CULL_FACE);
	AF_GL_CHK;

	glEnable(GL_BLEND);
	AF_GL_CHK;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	AF_GL_CHK;

	return AF_ERR_NONE;
}

enum af_err aga_puttext(float x, float y, const char* text) {
	glDisable(GL_TEXTURE_2D);
	AF_GL_CHK;

	glDisable(GL_LIGHTING);
	AF_GL_CHK;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glRasterPos2f(x, y);
	AF_GL_CHK;

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glColor3f(1.0f, 1.0f, 1.0f);
	AF_GL_CHK;

	glListBase(AGA_FONT_LIST_BASE);
	AF_GL_CHK;

	glCallLists((int) af_strlen(text), GL_UNSIGNED_BYTE, text);
	AF_GL_CHK;

	glEnable(GL_TEXTURE_2D);
	AF_GL_CHK;

	glEnable(GL_LIGHTING);
	AF_GL_CHK;

	return AF_ERR_NONE;
}

enum af_err aga_puttextfmt(float x, float y, const char* fmt, ...) {
	aga_fixed_buf_t buf = { 0 };
	va_list l;

	va_start(l, fmt);
	if(vsprintf(buf, fmt, l) < 0) return aga_af_errno(__FILE__, "vsprintf");
	va_end(l);

	return aga_puttext(x, y, buf);
}
