/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agaimg.h>
#include <agasnd.h>
#include <agaio.h>
#include <agascript.h>

int main(int argc, char** argv) {
	struct aga_ctx ctx;

	struct aga_clip nggyu = { 0 };

	struct aga_scriptclass* class;
	struct aga_scriptinst inst;

	aga_af_chk("aga_init", aga_init(&ctx, argc, argv));

	aga_setcam(&ctx);

	if(ctx.settings.audio_enabled) {
		aga_af_chk(
			"AGA_MK_LARGE_FILE_STRATEGY",
			AGA_MK_LARGE_FILE_STRATEGY(
				"res/nggyu-u8pcm-8k.raw", &nggyu.pcm, &nggyu.len));
	}

	aga_af_chk(
		"aga_findclass", aga_findclass(&ctx.scripteng, &class, "camera"));
	aga_af_chk("aga_mkscriptinst", aga_mkscriptinst(class, &inst));

	ctx.die = AF_FALSE;
	while(!ctx.die) {
		if(ctx.settings.audio_enabled) aga_putclip(&ctx.snddev, &nggyu);

		aga_af_chk("aga_poll", aga_poll(&ctx));

		{
			float clear[] = { 1.0f, 0.0f, 1.0f, 1.0f };
			aga_af_chk("af_clear", af_clear(&ctx.af_ctx, clear));
		}

		aga_af_chk("aga_instcall", aga_instcall(&inst, "update"));

		aga_af_chk("af_flush", af_flush(&ctx.af_ctx));
		aga_af_chk("aga_swapbuf", aga_swapbuf(&ctx, &ctx.win));
	}

	if(ctx.settings.audio_enabled) {
		aga_af_chk(
			"AGA_KILL_LARGE_FILE_STRATEGY",
			AGA_KILL_LARGE_FILE_STRATEGY(nggyu.pcm, nggyu.len));
	}

	aga_af_chk("aga_killscriptinst", aga_killscriptinst(&inst));

	aga_af_chk("aga_kill", aga_kill(&ctx));

	return EXIT_SUCCESS;
}
