/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agactx.h>
#include <agalog.h>
#include <agadraw.h>
#include <agaerr.h>

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

enum af_err aga_init(
		struct aga_ctx* ctx, struct aga_opts* opts, int argc, char** argv) {

	enum af_err result;

	AF_PARAM_CHK(ctx);
	AF_PARAM_CHK(opts);

	aga_log(__FILE__, "Starting context init...");

	ctx->opts = opts;

	/* TODO: Move out windowing environment to separate conglomerated init. */
	if((result = aga_mkctxdpy(ctx, opts->display))) {
		aga_af_soft(__FILE__, "aga_mkctxdpy", result);
		return result;
	}
	if((result = aga_mkwin(opts, &ctx->winenv, &ctx->win, argc, argv))) {
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

	if(opts->audio_enabled) {
		result = aga_mksnddev(opts->audio_dev, &ctx->snddev);
		if(result) {
			aga_af_soft(__FILE__, "aga_mksnddev", result);
			opts->audio_dev = AF_FALSE;
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

	if(ctx->opts->audio_enabled) AF_CHK(aga_killsnddev(&ctx->snddev));

	return AF_ERR_NONE;
}
