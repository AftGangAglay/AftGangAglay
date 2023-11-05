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

int main(int argc, char** argv) {
	struct aga_ctx ctx;

	struct aga_scripteng scripteng;
	struct aga_scriptclass* class = 0;
	struct aga_scriptinst inst;

	struct aga_timestamp ts;
	af_size_t frame_us = 0;

	enum af_err result;

	const char* logfiles[] = { AGA_SYSOUT, "aga.log" };

	aga_af_chk(
		__FILE__, "aga_mklog", aga_mklog(logfiles, AF_ARRLEN(logfiles)));

	aga_log(__FILE__, "Breathing in the chemicals...");

	aga_af_chk(__FILE__, "aga_init", aga_init(&ctx, argc, argv));

	result = aga_mkscripteng(
		&ctx, &scripteng, ctx.settings.startup_script,
		ctx.settings.python_path, argc, argv);
	if(result) aga_af_soft(__FILE__, "aga_mkscripteng", result);
	else {
		aga_af_chk(__FILE__, "aga_findclass", aga_findclass(
			&scripteng, &class, "game"));
		aga_af_chk(__FILE__, "aga_mkscriptinst", aga_mkscriptinst(
			class, &inst));

		result = aga_instcall(&inst, "create");
		if(result) aga_af_soft(__FILE__, "aga_instcall", result);
	}

	glEnable(GL_CULL_FACE);
	aga_af_chk(__FILE__, "glEnable", af_gl_chk());

	ctx.die = AF_FALSE;
	while(!ctx.die) {
		aga_af_chk(__FILE__, "aga_startstamp", aga_startstamp(&ts));

		result = aga_poll(&ctx);
		if(result) aga_af_soft(__FILE__, "aga_poll", result);

		if(class) {
			result = aga_instcall(&inst, "update");
			if(result) aga_af_soft(__FILE__, "aga_instcall", result);
		}

		aga_af_chk(__FILE__, "aga_puttextfmt", aga_puttextfmt(
			&ctx, -0.8f, 0.7f, "frametime: %zu", frame_us));
		aga_af_chk(__FILE__, "aga_puttextfmt", aga_puttextfmt(
			&ctx, -0.8f, 0.6f, "fps: %lf",
			(1.0 / (double) frame_us) * 1e6));

		result = af_flush(&ctx.af_ctx);
		if(result) aga_af_soft(__FILE__, "af_flush", result);

		aga_af_chk(__FILE__, "aga_endstamp", aga_endstamp(&ts, &frame_us));

		if(!ctx.die) {
			result = aga_swapbuf(&ctx, &ctx.win);
			if(result) aga_af_soft(__FILE__, "aga_swapbuf", result);
		}
	}

	aga_log(__FILE__, "Tearing down...");

	if(class) {
		aga_af_chk(__FILE__, "aga_instcall", aga_instcall(&inst, "close"));
		aga_af_chk(__FILE__, "aga_killscriptinst", aga_killscriptinst(&inst));
	}

	aga_af_chk(__FILE__, "aga_killscripteng", aga_killscripteng(&scripteng));
	aga_af_chk(__FILE__, "aga_kill", aga_kill(&ctx));

	aga_log(__FILE__, "Bye-bye!");
	aga_af_chk(__FILE__, "aga_killlog", aga_killlog());

	return 0;
}
