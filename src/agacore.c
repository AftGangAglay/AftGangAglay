/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agaconf.h>

static const struct af_vert_element vert_elements[] = {
	{ AF_MEMBSIZE(struct aga_vertex, pos ), AF_VERT_POS  },
	{ AF_MEMBSIZE(struct aga_vertex, col ), AF_VERT_COL  },
	{ AF_MEMBSIZE(struct aga_vertex, uv  ), AF_VERT_UV   },
	{ AF_MEMBSIZE(struct aga_vertex, norm), AF_VERT_NORM },
};

enum af_err aga_init(struct aga_ctx* ctx, int* argcp, char** argvp) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(argcp);
	AF_PARAM_CHK(argvp);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize((int) ctx->settings.width, (int) ctx->settings.height);
	glutInit(argcp, argvp);
	if(!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) return AF_ERR_BAD_OP;
	ctx->win = glutCreateWindow("Aft Gang Aglay");

	AF_CHK(af_mkctx(&ctx->af_ctx, AF_FIDELITY_FAST));
	AF_CHK(af_mkvert(
		&ctx->af_ctx, &ctx->vert, vert_elements, AF_ARRLEN(vert_elements)));

	af_memset(&ctx->cam, 0, sizeof(struct aga_cam));
	AF_CHK(aga_setcam(ctx));

	if(ctx->settings.audio_enabled) {
		AF_CHK(aga_mksnddev(ctx->settings.audio_dev, &ctx->snddev));
	}

	/* TODO: Python path resolution in packaged builds. */
	AF_CHK(aga_mkscripteng(
		&ctx->scripteng, ctx->settings.startup_script,
		ctx->settings.python_path));

	return AF_ERR_NONE;
}

enum af_err aga_kill(struct aga_ctx* ctx) {
	AF_CTX_CHK(ctx);

	glutDestroyWindow(ctx->win);

	AF_CHK(af_killvert(&ctx->af_ctx, &ctx->vert));
	AF_CHK(af_killctx(&ctx->af_ctx));

	if(ctx->settings.audio_enabled) AF_CHK(aga_killsnddev(&ctx->snddev));

	AF_CHK(aga_killscripteng(&ctx->scripteng));

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

void aga_errno_chk(const char* proc) {
	perror(proc);
	abort();
}

void aga_fatal(const char* fmt, ...) {
	va_list l;
	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
	putc('\n', stderr);
	abort();
}

void aga_boundf(float* f, float min, float max) {
	if(*f < min) *f = min;
	if(*f > max) *f = max;
}
