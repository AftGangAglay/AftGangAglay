/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agaimg.h>
#include <agasnd.h>
#include <agalog.h>
#include <agascript.h>
#include <agadraw.h>

#include <afeirsa/afgl.h>

#ifdef AGA_HAVE_UNIX
# include <sys/time.h>
#endif

int main(int argc, char** argv) {
	struct aga_ctx ctx;

	struct aga_scriptclass* class;
	struct aga_scriptinst inst;

	enum af_err result;

	const char* logfiles[] = { AGA_SYSOUT, "aga.log" };

	aga_af_chk(
		__FILE__, "aga_mklog", aga_mklog(logfiles, AF_ARRLEN(logfiles)));

	aga_log(__FILE__, "Breathing in the chemicals...");

	aga_af_chk(__FILE__, "aga_init", aga_init(&ctx, argc, argv));

	aga_af_chk(__FILE__,
		"aga_findclass", aga_findclass(&ctx.scripteng, &class, "game"));
	aga_af_chk(__FILE__, "aga_mkscriptinst", aga_mkscriptinst(class, &inst));

	result = aga_instcall(&inst, "create");
	if(result) aga_af_soft(__FILE__, "aga_instcall", result);

	glEnable(GL_FOG);
	aga_af_chk(__FILE__, "glEnable", af_gl_chk());

	glFogi(GL_FOG_MODE, GL_EXP);
	aga_af_chk(__FILE__, "glFogi", af_gl_chk());

	{
		float col[] = {0.03f, 0.0f, 0.01f};
		glFogfv(GL_FOG_COLOR, col);
		aga_af_chk(__FILE__, "glFogfv", af_gl_chk());
	}

	glEnable(GL_CULL_FACE);
	aga_af_chk(__FILE__, "glEnable", af_gl_chk());
	glDepthFunc(GL_LEQUAL);
	aga_af_chk(__FILE__, "glDepthFunc", af_gl_chk());

	glFogf(GL_FOG_DENSITY, 0.05f);
	aga_af_chk(__FILE__, "glFogf", af_gl_chk());
	glFogf(GL_FOG_START, 1.0f);
	aga_af_chk(__FILE__, "glFogf", af_gl_chk());
	glFogf(GL_FOG_END, 10.0f);
	aga_af_chk(__FILE__, "glFogf", af_gl_chk());

	ctx.die = AF_FALSE;
	while(!ctx.die) {
		af_size_t verts = ctx.frame_verts;
#ifdef AGA_HAVE_UNIX
		struct timeval tv0;
		struct timeval tv1;
		if(gettimeofday(&tv0, 0) == -1) {
			aga_af_chk(__FILE__, "gettimeofday",
			   aga_af_errno(__FILE__, "gettimeofday"));
		}
#endif
		ctx.frame_verts = 0;

		result = aga_poll(&ctx);
		if(result) aga_af_soft(__FILE__, "aga_poll", result);

		{
			float clear[] = { 1.0f, 0.0f, 1.0f, 1.0f };
			aga_af_chk(__FILE__, "af_clear", af_clear(&ctx.af_ctx, clear));
		}
		result = aga_instcall(&inst, "update");
		if(result) aga_af_soft(__FILE__, "aga_instcall", result);

		aga_af_chk(__FILE__, "aga_puttextfmt", aga_puttextfmt(
			&ctx, -0.8f, 0.8f, "verts: %zu", verts));

		aga_af_chk(__FILE__, "aga_puttextfmt", aga_puttextfmt(
			&ctx, -0.8f, 0.7f, "frametime: %zu", ctx.frame_us));
		aga_af_chk(__FILE__, "aga_puttextfmt", aga_puttextfmt(
			&ctx, -0.8f, 0.6f, "fps: %lf",
			(1.0 / (double) ctx.frame_us) * 1e6));

		result = af_flush(&ctx.af_ctx);
		if(result) aga_af_soft(__FILE__, "af_flush", result);
		result = aga_swapbuf(&ctx, &ctx.win);
		if(result) aga_af_soft(__FILE__, "aga_swapbuf", result);

#ifdef AGA_HAVE_UNIX
		if(gettimeofday(&tv1, 0) == -1) {
			aga_af_chk(__FILE__, "gettimeofday",
			   aga_af_errno(__FILE__, "gettimeofday"));
		}

		if(tv0.tv_sec != tv1.tv_sec) continue; /* World is ending. */
		ctx.frame_us = tv1.tv_usec - tv0.tv_usec;
#endif
	}

	aga_log(__FILE__, "Tearing down...");

	aga_af_chk(__FILE__, "aga_instcall", aga_instcall(&inst, "close"));

	aga_af_chk(__FILE__, "aga_killscriptinst", aga_killscriptinst(&inst));

	aga_af_chk(__FILE__, "aga_kill", aga_kill(&ctx));

	aga_log(__FILE__, "Bye-bye!");
	aga_af_chk(__FILE__, "aga_killlog", aga_killlog());

	return EXIT_SUCCESS;
}
