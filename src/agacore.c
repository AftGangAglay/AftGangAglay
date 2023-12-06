/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agalog.h>
#include <agadraw.h>

#define AGA_WANT_UNIX
#include <agastd.h>
#undef AGA_WANT_UNIX

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

enum af_err aga_init(struct aga_ctx* ctx, int argc, char** argv) {
	enum af_err result;

	const char* confpath = "aga.sgml";
	const char* display = getenv("DISPLAY");
	const char* chcwd = ".";
	const char* audio_dev = 0;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(argv);

	ctx->audio_enabled = AF_FALSE;

#ifdef _POSIX_SOURCE
	{
		const char* helpstr =
			"warn: usage: %s [-f config] [-A dsp] [-D display] [-C dir]";
		int o;
		while((o = 	getopt(argc, argv, "f:s:A:D:C:")) != -1) {
			switch(o) {
				default: {
					aga_log(__FILE__, helpstr, argv[0]);
					goto break2;
				}
				case 'f': confpath = optarg; break;
				case 'A': audio_dev = optarg; break;
				case 'D': display = optarg; break;
				case 'C': chcwd = optarg; break;
			}
		}
		break2:;
	}
#endif

	if(chdir(chcwd) == -1) return aga_af_errno(__FILE__, "chdir");

	af_memset(&ctx->conf, 0, sizeof(ctx->conf));

	result = aga_mkconf(confpath, &ctx->conf);
	if(result) aga_af_soft(__FILE__, "aga_mkconf", result);

	AF_CHK(aga_mkctxdpy(ctx, display));
	AF_CHK(aga_mkwin(ctx, &ctx->win, argc, argv));
	AF_CHK(aga_glctx(ctx, &ctx->win));
	AF_CHK(af_mkctx(&ctx->af_ctx, AF_FIDELITY_FAST));
	AF_CHK(af_mkvert(
		&ctx->af_ctx, &ctx->vert, vert_elements, AF_ARRLEN(vert_elements)));

	AF_CHK(aga_setdrawparam());

	{
		int v;
		const char* enabled[] = { "Audio", "Enabled" };
		const char* device[] = { "Audio", "Device" };

		result = aga_conftree(
			&ctx->conf, enabled, AF_ARRLEN(enabled), &v, AGA_INTEGER);
		if(result) aga_af_soft(__FILE__, "aga_conftree", result);

		if(!audio_dev) {
			result = aga_conftree(
				&ctx->conf, device, AF_ARRLEN(device), &audio_dev, AGA_STRING);
			if(result) {
				aga_af_soft(__FILE__, "aga_conftree", result);
				audio_dev = "/dev/dsp1";
			}
		}

		if(ctx->audio_enabled) {
			result = aga_mksnddev(audio_dev, &ctx->snddev);
			if(result) {
				aga_af_soft(__FILE__, "aga_mksnddev", result);
				ctx->audio_enabled = AF_FALSE;
			}
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

	if(ctx->audio_enabled) AF_CHK(aga_killsnddev(&ctx->snddev));

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
