/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agalog.h>

#include <agastd.h>

static const struct af_vert_element vert_elements[] = {
	{ AF_MEMBSIZE(struct aga_vertex, col ), AF_VERT_COL  },
	{ AF_MEMBSIZE(struct aga_vertex, uv  ), AF_VERT_UV   },
	{ AF_MEMBSIZE(struct aga_vertex, norm), AF_VERT_NORM },
	{ AF_MEMBSIZE(struct aga_vertex, pos ), AF_VERT_POS  }
};

static enum af_err aga_parseconf(struct aga_ctx* ctx, const char* path) {
	struct aga_conf_node* item;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(path);

	/* Set defaults */
	{
		ctx->settings.sensitivity = 0.25f;
		ctx->settings.move_speed = 0.1f;

		ctx->settings.width = 640;
		ctx->settings.height = 480;
		ctx->settings.fov = 60.0f;

		ctx->settings.audio_enabled = AF_FALSE;
		ctx->settings.audio_dev = "/dev/dsp";

		ctx->settings.startup_script = "res/script/main.py";
		ctx->settings.python_path =
			"vendor/python/lib:res/script:res/script/aga";
	}

	af_memset(&ctx->conf, 0, sizeof(ctx->conf));

	AF_CHK(aga_mkconf(path, &ctx->conf));

	/* TODO: We can almost certainly clean this up. */

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
				else if(af_streql(v->name, "MoveSpeed")) {
					AF_VERIFY(v->type == AGA_FLOAT, AF_ERR_BAD_PARAM);
					ctx->settings.move_speed = (float) v->data.flt;
				}
				else {
					aga_log(
						__FILE__,
						"warn: unknown input setting `%s'", v->name);
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
					aga_log(
						__FILE__,
						"warn: unknown display setting `%s'", v->name);
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
					aga_log(
						__FILE__,
						"warn: unknown audio setting `%s'", v->name);
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
					aga_log(
						__FILE__,
						"warn: unknown script setting `%s'", v->name);
				}
			}
		}
	}

	return AF_ERR_NONE;
}

enum af_err aga_init(struct aga_ctx* ctx, int argc, char** argv) {
	/* TODO: argv */
	const char confpath[] = "res/aga.sgml";

	enum af_err result;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(argc);
	AF_PARAM_CHK(argv);

	result = aga_parseconf(ctx, confpath);
	if(result) aga_af_soft(__FILE__, "aga_parseconf", result);

	ctx->argc = argc;
	ctx->argv = argv;

	/* TODO: `DISPLAY' from argv */
	AF_CHK(aga_mkctxdpy(ctx));
	AF_CHK(aga_mkwin(ctx, &ctx->win));
	AF_CHK(aga_glctx(ctx, &ctx->win));
	AF_CHK(af_mkctx(&ctx->af_ctx, AF_FIDELITY_FAST));

	AF_CHK(af_mkvert(
		&ctx->af_ctx, &ctx->vert, vert_elements, AF_ARRLEN(vert_elements)));

	if(ctx->settings.audio_enabled) {
		result = aga_mksnddev(ctx->settings.audio_dev, &ctx->snddev);
		if(result) {
			aga_af_soft(__FILE__, "aga_mksnddev", result);
			ctx->settings.audio_enabled = AF_FALSE;
		}
	}

	/* TODO: Python path resolution in packaged builds. */
	AF_CHK(aga_mkscripteng(
		ctx, ctx->settings.startup_script, ctx->settings.python_path));

	return AF_ERR_NONE;
}

enum af_err aga_kill(struct aga_ctx* ctx) {
	AF_CTX_CHK(ctx);

	AF_CHK(af_killvert(&ctx->af_ctx, &ctx->vert));
	AF_CHK(af_killctx(&ctx->af_ctx));

	AF_CHK(aga_killwin(ctx, &ctx->win));
	AF_CHK(aga_killctxdpy(ctx));

	if(ctx->settings.audio_enabled) AF_CHK(aga_killsnddev(&ctx->snddev));

	AF_CHK(aga_killscripteng(&ctx->scripteng));

	AF_CHK(aga_killconf(&ctx->conf));

	return AF_ERR_NONE;
}

const char* aga_af_errname(enum af_err e) {
	switch(e) {
		default:;
			AF_FALLTHROUGH;
			/* FALLTHRU */
		case AF_ERR_NONE: return "none";

		case AF_ERR_UNKNOWN: return "unknown";
		case AF_ERR_BAD_PARAM: return "bad parameter";
		case AF_ERR_BAD_CTX: return "bad context";
		case AF_ERR_BAD_OP: return "bad operation";
		case AF_ERR_NO_GL: return "no opengl";
		case AF_ERR_MEM: return "out of memory";
	}

	return "none";
}

void aga_af_chk(const char* loc, const char* proc, enum af_err e) {
	if(!e) return;
	aga_af_soft(loc, proc, e);
	abort();
}

void aga_af_soft(const char* loc, const char* proc, enum af_err e) {
	aga_log(loc, "err: %s: %s", proc, aga_af_errname(e));
}

void aga_errno_chk(const char* loc, const char* proc) {
	aga_log(loc, "err: %s: %s", proc, strerror(errno));
	abort();
}

enum af_err aga_af_errno(const char* loc, const char* proc) {
	return aga_af_patherrno(loc, proc, 0);
}

enum af_err aga_af_patherrno(
		const char* loc, const char* proc, const char* path) {

	if(loc) {
		if(path) aga_log(loc, "err: %s: %s `%s'", proc, strerror(errno), path);
		else aga_log(loc, "err: %s: %s", proc, strerror(errno));
	}
	switch(errno) {
		default: return AF_ERR_UNKNOWN;
		case 0: return AF_ERR_NONE;

		case EBADF: return AF_ERR_BAD_PARAM;
		case ENOMEM: return AF_ERR_MEM;
		case EACCES:
			AF_FALLTHROUGH;
			/* FALLTHRU */
		case EOPNOTSUPP: return AF_ERR_BAD_OP;
	}
}

void aga_boundf(float* f, float min, float max) {
	if(*f < min) *f = min;
	if(*f > max) *f = max;
}
