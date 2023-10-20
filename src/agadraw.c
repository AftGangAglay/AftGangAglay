/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agadraw.h>
#include <agastd.h>

#include <afeirsa/afgl.h>

enum af_err aga_puttext(
		struct aga_ctx* ctx, float x, float y, const char* text) {

	/* GL Text-less backends or no font or bad font shouldn't mean a crash. */
	if(!ctx->font_base) return AF_ERR_NONE;

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

	for(; *text; ++text) {
		glCallList(ctx->font_base + (*text - (' ')));
		AF_GL_CHK;
	}

	glEnable(GL_TEXTURE_2D);
	AF_GL_CHK;

	glEnable(GL_LIGHTING);
	AF_GL_CHK;

	return AF_ERR_NONE;
}

enum af_err aga_puttextfmt(
		struct aga_ctx* ctx, float x, float y, const char* fmt, ...) {

	aga_fixed_buf_t buf = { 0 };
	va_list l;

	va_start(l, fmt);
	if(vsprintf(buf, fmt, l) < 0) return aga_af_errno(__FILE__, "vsprintf");
	va_end(l);

	return aga_puttext(ctx, x, y, buf);
}
