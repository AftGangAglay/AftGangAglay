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

#ifdef AGA_HAVE_UNIX
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

	if(chdir(chcwd) == -1) (void) aga_af_patherrno(__FILE__, "chdir", chcwd);
#endif

	aga_log(__FILE__, "Starting context init...");

	af_memset(&ctx->conf, 0, sizeof(ctx->conf));

	ctx->conf_path = confpath;
	result = aga_mkconf(confpath, &ctx->conf);
	if(result) aga_af_soft(__FILE__, "aga_mkconf", result);

	aga_log(__FILE__, "Config loaded from `%s'", confpath);

	if((result = aga_mkctxdpy(ctx, display))) {
		aga_af_soft(__FILE__, "aga_mkctxdpy", result);
		return result;
	}
	if((result = aga_mkwin(ctx, &ctx->win, argc, argv))) {
		aga_af_soft(__FILE__, "aga_mkwin", result);
		return result;
	}
	if((result = aga_glctx(ctx, &ctx->win))) {
		aga_af_soft(__FILE__, "aga_glctx", result);
		return result;
	}
	if((result = af_mkctx(&ctx->af_ctx, AF_FIDELITY_FAST))) {
		aga_af_soft(__FILE__, "af_mkctx", result);
		return result;
	}

	aga_log(__FILE__, "Acquired GL context");

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

	aga_log(__FILE__, "Done!");

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
