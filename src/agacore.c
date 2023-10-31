/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agalog.h>

#define AGA_WANT_UNIX
#include <agastd.h>
#undef AGA_WANT_UNIX

static const struct af_vert_element vert_elements[] = {
	{ AF_MEMBSIZE(struct aga_vertex, col ), AF_VERT_COL  },
	{ AF_MEMBSIZE(struct aga_vertex, uv  ), AF_VERT_UV   },
	{ AF_MEMBSIZE(struct aga_vertex, norm), AF_VERT_NORM },
	{ AF_MEMBSIZE(struct aga_vertex, pos ), AF_VERT_POS  }
};

static enum af_err aga_parseconf(struct aga_ctx* ctx, const char* path) {
	struct aga_conf_node* item;
	struct aga_conf_node* v;
	struct aga_settings* sets = &ctx->settings;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(path);

	/* Set defaults */
	{
		sets->sensitivity = 0.25f;
		sets->move_speed = 0.1f;

		sets->width = 640;
		sets->height = 480;
		sets->fov = 60.0f;

		sets->audio_enabled = AF_FALSE;
		if(!sets->audio_dev) sets->audio_dev = "/dev/dsp";

		if(!sets->startup_script) sets->startup_script = "script/main.py";
		sets->python_path = "vendor/python/lib:script:script/aga";
	}

	af_memset(&ctx->conf, 0, sizeof(ctx->conf));

	AF_CHK(aga_mkconf(path, &ctx->conf));

	for(item = ctx->conf.children->children;
		item < ctx->conf.children->children + ctx->conf.children->len;
		++item) {

		if(af_streql(item->name, "Input")) {
			for(v = item->children; v < item->children + item->len; ++v) {
				aga_confvar("Sensitivity", v, AGA_FLOAT, &sets->sensitivity);
				aga_confvar("MoveSpeed", v, AGA_FLOAT, &sets->move_speed);
			}
		}
		else if(af_streql(item->name, "Display")) {
			for(v = item->children; v < item->children + item->len; ++v) {
				aga_confvar("Width", v, AGA_INTEGER, &sets->width);
				aga_confvar("Height", v, AGA_INTEGER, &sets->height);
				aga_confvar("FOV", v, AGA_FLOAT, &sets->fov);
			}
		}
		else if(af_streql(item->name, "Audio")) {
			for(v = item->children; v < item->children + item->len; ++v) {
				long enabled;
				if(aga_confvar("Enabled", v, AGA_INTEGER, &enabled))
					sets->audio_enabled = !!enabled;
				aga_confvar("Device", v, AGA_STRING, &sets->audio_dev);
			}
		}
		else if(af_streql(item->name, "Script")) {
			for(v = item->children; v < item->children + item->len; ++v) {
				aga_confvar("Startup", v, AGA_STRING, &sets->startup_script);
				aga_confvar("Path", v, AGA_STRING, &sets->python_path);
			}
		}
	}

	return AF_ERR_NONE;
}

enum af_err aga_init(struct aga_ctx* ctx, int argc, char** argv) {
	const char* confpath = "aga.sgml";
	enum af_err result;
	const char* display = 0;
	const char* chcwd = 0;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(argc);
	AF_PARAM_CHK(argv);

	ctx->settings.audio_dev = 0;
	ctx->settings.startup_script = 0;

#ifdef _POSIX_SOURCE
	{
		const char* helpstr =
			"warn: usage: %s [-f config] [-s script] [-A dsp] [-D display] "
			"[-C dir]";
		int o;
		while((o = 	getopt(argc, argv, "f:s:A:D:C:")) != -1) {
			switch(o) {
				default: {
					aga_log(__FILE__, helpstr, argv[0]);
					goto break2;
				}
				case 'f': confpath = optarg; break;
				case 's': ctx->settings.startup_script = optarg; break;
				case 'A': ctx->settings.audio_dev = optarg; break;
				case 'D': display = optarg; break;
				case 'C': chcwd = optarg; break;
			}
		}
		break2:;
	}
#endif

	if(!display) display = getenv("DISPLAY");
	if(chcwd && chdir(chcwd) == -1) return aga_af_errno(__FILE__, "chdir");

	result = aga_parseconf(ctx, confpath);
	if(result) aga_af_soft(__FILE__, "aga_parseconf", result);

	ctx->argc = argc;
	ctx->argv = argv;

	AF_CHK(aga_mkctxdpy(ctx, display));
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

	return AF_ERR_NONE;
}

enum af_err aga_kill(struct aga_ctx* ctx) {
	AF_CTX_CHK(ctx);

	AF_CHK(af_killvert(&ctx->af_ctx, &ctx->vert));
	AF_CHK(af_killctx(&ctx->af_ctx));

	AF_CHK(aga_killwin(ctx, &ctx->win));
	AF_CHK(aga_killctxdpy(ctx));

	if(ctx->settings.audio_enabled) AF_CHK(aga_killsnddev(&ctx->snddev));

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

#ifdef AGA_HAVE_UNIX
# include <sys/time.h>

enum af_err aga_startstamp(struct aga_timestamp* ts) {
	struct timeval tv;

	AF_PARAM_CHK(ts);

	if(gettimeofday(&tv, 0) == -1) {
		return aga_af_errno(__FILE__, "gettimeofday");
	}

	ts->sec = tv.tv_sec;
	ts->usec = tv.tv_usec;

	return AF_ERR_NONE;
}

enum af_err aga_endstamp(const struct aga_timestamp* ts, af_size_t* us) {
	struct timeval tv;

	AF_PARAM_CHK(ts);
	AF_PARAM_CHK(us);

	if(gettimeofday(&tv, 0) == -1) {
		return aga_af_errno(__FILE__, "gettimeofday");
	}

	*us = tv.tv_usec - ts->usec;
	*us += (tv.tv_sec - ts->sec) * 1000000;

	return AF_ERR_NONE;
}

#endif
