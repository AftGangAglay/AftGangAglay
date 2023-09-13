/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>

static const struct af_vert_element vert_elements[] = {
	{ AF_MEMBSIZE(struct aga_vertex, pos ), AF_VERT_POS  },
	{ AF_MEMBSIZE(struct aga_vertex, col ), AF_VERT_COL  },
	{ AF_MEMBSIZE(struct aga_vertex, uv  ), AF_VERT_UV   },
	{ AF_MEMBSIZE(struct aga_vertex, norm), AF_VERT_NORM },
};

static enum af_err aga_parseconf(struct aga_ctx* ctx, const char* path) {
	struct aga_conf_node* item;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(path);

	/* Set defaults */
	{
		ctx->settings.sensitivity = 0.25f;
		ctx->settings.zoom_speed = 0.1f;
		ctx->settings.min_zoom = 2.0f;
		ctx->settings.max_zoom = 50.0f;

		ctx->settings.width = 640;
		ctx->settings.height = 480;
		ctx->settings.fov = 60.0f;

		ctx->settings.audio_enabled = AF_FALSE;
		ctx->settings.audio_dev = "/dev/dsp";

		ctx->settings.startup_script = "res/main.py";
		ctx->settings.python_path = "vendor/python/lib:res";
	}

	AF_CHK(aga_mkconf(path, &ctx->conf));

	for(item = ctx->conf.children->children;
		item < ctx->conf.children->children + ctx->conf.children->len;
		++item) {

		if(af_streql(item->name, "Input")) {
			struct aga_conf_node* v;
			for(v = item->children; v < item->children + item->len; ++v) {
				if(af_streql(v->name, "Sensitivity")) {
					AF_VERIFY(v->type == AGA_FLOAT, AF_ERR_BAD_PARAM);
					ctx->settings.sensitivity = (float) v->data.flt;
				}
				else if(af_streql(v->name, "ZoomSpeed")) {
					AF_VERIFY(v->type == AGA_FLOAT, AF_ERR_BAD_PARAM);
					ctx->settings.zoom_speed = (float) v->data.flt;
				}
				else if(af_streql(v->name, "MinZoom")) {
					AF_VERIFY(v->type == AGA_FLOAT, AF_ERR_BAD_PARAM);
					ctx->settings.min_zoom = (float) v->data.flt;
				}
				else if(af_streql(v->name, "MaxZoom")) {
					AF_VERIFY(v->type == AGA_FLOAT, AF_ERR_BAD_PARAM);
					ctx->settings.max_zoom = (float) v->data.flt;
				}
				else {
					fprintf(
						stderr, "warn: unknown input setting `%s'\n", v->name);
				}
			}
		}
		else if(af_streql(item->name, "Display")) {
			struct aga_conf_node* v;
			for(v = item->children; v < item->children + item->len; ++v) {
				if(af_streql(v->name, "Width")) {
					AF_VERIFY(v->type == AGA_INTEGER, AF_ERR_BAD_PARAM);
					ctx->settings.width = v->data.integer;
				}
				else if(af_streql(v->name, "Height")) {
					AF_VERIFY(v->type == AGA_INTEGER, AF_ERR_BAD_PARAM);
					ctx->settings.height = v->data.integer;
				}
				else if(af_streql(v->name, "FOV")) {
					AF_VERIFY(v->type == AGA_FLOAT, AF_ERR_BAD_PARAM);
					ctx->settings.fov = (float) v->data.flt;
				}
				else {
					fprintf(
						stderr,
						"warn: unknown display setting `%s'\n", v->name);
				}
			}
		}
		else if(af_streql(item->name, "Audio")) {
			struct aga_conf_node* v;
			for(v = item->children; v < item->children + item->len; ++v) {
				if(af_streql(v->name, "Enabled")) {
					AF_VERIFY(v->type == AGA_INTEGER, AF_ERR_BAD_PARAM);
					ctx->settings.audio_enabled = !!v->data.integer;
				}
				else if(af_streql(v->name, "Device")) {
					AF_VERIFY(v->type == AGA_STRING, AF_ERR_BAD_PARAM);
					ctx->settings.audio_dev = v->data.string;
				}
				else {
					fprintf(
						stderr, "warn: unknown audio setting `%s'\n", v->name);
				}
			}
		}
		else if(af_streql(item->name, "Script")) {
			struct aga_conf_node* v;
			for(v = item->children; v < item->children + item->len; ++v) {
				if(af_streql(v->name, "Startup")) {
					AF_VERIFY(v->type == AGA_STRING, AF_ERR_BAD_PARAM);
					ctx->settings.startup_script = v->data.string;
				}
				else if(af_streql(v->name, "Path")) {
					AF_VERIFY(v->type == AGA_STRING, AF_ERR_BAD_PARAM);
					ctx->settings.python_path = v->data.string;
				}
				else {
					fprintf(
						stderr,
						"warn: unknown script setting `%s'\n", v->name);
				}
			}
		}
	}

	return AF_ERR_NONE;
}

enum af_err aga_init(struct aga_ctx* ctx, int* argcp, char** argvp) {
	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(argcp);
	AF_PARAM_CHK(argvp);

	AF_CHK(aga_parseconf(ctx, "res/aga.sgml"));

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

	AF_CHK(aga_killconf(&ctx->conf));

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
