/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>

const static struct af_vert_element vert_elements[] = {
        { AF_MEMBSIZE(struct aga_vertex, pos ), AF_VERT_POS  },
        { AF_MEMBSIZE(struct aga_vertex, col ), AF_VERT_COL  },
        { AF_MEMBSIZE(struct aga_vertex, uv  ), AF_VERT_UV   },
        { AF_MEMBSIZE(struct aga_vertex, norm), AF_VERT_NORM },
};

enum af_err aga_init(struct aga_ctx* ctx, int* argcp, char** argvp) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(argcp);
	AF_PARAM_CHK(argvp);

	glutInitDisplayMode(GLUT_DOUBLE);
	glutInitWindowSize((int) ctx->settings.width, (int) ctx->settings.height);
	glutInit(argcp, argvp);
	ctx->win = glutCreateWindow("Aft Gang Aglay");

	AF_CHK(af_mkctx(&ctx->af_ctx, AF_FIDELITY_FAST));
    AF_CHK(af_mkvert(
        &ctx->af_ctx, &ctx->vert, vert_elements, AF_ARRLEN(vert_elements)));

	af_memset(&ctx->cam, 0, sizeof(struct aga_cam));
	AF_CHK(aga_setcam(ctx));

	return AF_ERR_NONE;
}

enum af_err aga_kill(struct aga_ctx* ctx) {
	AF_CTX_CHK(ctx);

	glutDestroyWindow(ctx->win);

	AF_CHK(af_killvert(&ctx->af_ctx, &ctx->vert));
	AF_CHK(af_killctx(&ctx->af_ctx));

	return AF_ERR_NONE;
}

enum af_err aga_setcam(struct aga_ctx* ctx) {
	AF_CTX_CHK(ctx);

	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(
			ctx->settings.fov,
			(double) ctx->settings.width / (double) ctx->settings.height,
			0.1, 100.0);

		glTranslatef(0.0f, 0.0f, -ctx->cam.dist);
		glRotatef(ctx->cam.pitch, 1.0f, 0.0f, 0.0f);
		glRotatef(ctx->cam.yaw, 0.0f, 1.0f, 0.0f);
		glTranslatef(
			ctx->cam.pos.decomp.x,
			ctx->cam.pos.decomp.y,
			ctx->cam.pos.decomp.z);

	return AF_ERR_NONE;
}

void aga_af_chk(const char* proc, enum af_err e) {
    const char* n;
    switch(e) {
        default:;
            AF_FALLTHROUGH;
            /* FALLTHRU */
        case AF_ERR_NONE: return;

        case AF_ERR_UNKNOWN: n = "unknown"; break;
        case AF_ERR_BAD_PARAM: n = "bad parameter"; break;
        case AF_ERR_BAD_CTX: n = "bad context"; break;
        case AF_ERR_BAD_OP: n = "bad operation"; break;
        case AF_ERR_NO_GL: n = "no opengl"; break;
        case AF_ERR_MEM: n = "out of memory"; break;
    }

    fprintf(stderr, "%s: %s\n", proc, n);
    abort();
}
