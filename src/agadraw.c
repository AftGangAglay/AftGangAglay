/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agadraw.h>
#include <agawin.h>
#include <agalog.h>
#include <agastd.h>
#include <agaerr.h>

#include <afeirsa/afgl.h>

/*
 * NOTE: This isn't actually used anywhere nowadays, but it's still polite to
 * 		 Base our vertex definitions off of a struct.
 */
struct aga_vertex {
    float col[4];
    float uv[2];
    float norm[3];
    float pos[3];
};

static const struct af_vert_element vert_elements[] = {
    { AF_MEMBSIZE(struct aga_vertex, col ), AF_VERT_COL  },
    { AF_MEMBSIZE(struct aga_vertex, uv  ), AF_VERT_UV   },
    { AF_MEMBSIZE(struct aga_vertex, norm), AF_VERT_NORM },
    { AF_MEMBSIZE(struct aga_vertex, pos ), AF_VERT_POS  }
};


enum af_err aga_setdrawparam(struct af_ctx* af, struct af_vert* vert) {
	glEnable(GL_CULL_FACE);
	AF_GL_CHK;

	glEnable(GL_BLEND);
	AF_GL_CHK;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	AF_GL_CHK;

    AF_CHK(af_mkvert(af, vert, vert_elements, AF_ARRLEN(vert_elements)));

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
