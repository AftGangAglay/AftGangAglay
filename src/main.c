/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agaimg.h>
#include <agasnd.h>
#include <agalog.h>
#include <agascript.h>

#include <afeirsa/afgl.h>

int main(int argc, char** argv) {
	struct aga_ctx ctx;

	struct aga_scriptclass* class;
	struct aga_scriptinst inst;

	const char* logfiles[] = { "/dev/stdout", "aga.log" };
	void aga_af_chk(const char* loc, const char* proc, enum af_err e);
	AGA_NORETURN void aga_errno_chk(const char* loc, const char* proc);

	aga_af_chk(
		__FILE__, "aga_mklog", aga_mklog(logfiles, AF_ARRLEN(logfiles)));

	aga_log(__FILE__, "Breathing in the chemicals...");

	aga_af_chk(__FILE__, "aga_init", aga_init(&ctx, argc, argv));

	aga_af_chk(__FILE__, 
		"aga_findclass", aga_findclass(&ctx.scripteng, &class, "game"));
	aga_af_chk(__FILE__, "aga_mkscriptinst", aga_mkscriptinst(class, &inst));

	ctx.die = AF_FALSE;
	while(!ctx.die) {
		aga_af_chk(__FILE__, "aga_poll", aga_poll(&ctx));

		{
			float clear[] = { 1.0f, 0.0f, 1.0f, 1.0f };
			aga_af_chk(__FILE__, "af_clear", af_clear(&ctx.af_ctx, clear));
		}

		aga_af_chk(__FILE__, "aga_instcall", aga_instcall(&inst, "update"));

		aga_af_chk(__FILE__, "af_flush", af_flush(&ctx.af_ctx));
		aga_af_chk(__FILE__, "aga_swapbuf", aga_swapbuf(&ctx, &ctx.win));
	}

	aga_log(__FILE__, "Tearing down...");

	aga_af_chk(__FILE__, "aga_instcall", aga_instcall(&inst, "close"));

	aga_af_chk(__FILE__, "aga_killscriptinst", aga_killscriptinst(&inst));

	aga_af_chk(__FILE__, "aga_kill", aga_kill(&ctx));

	aga_log(__FILE__, "Bye-bye!");
	aga_af_chk(__FILE__, "aga_killlog", aga_killlog());

	return EXIT_SUCCESS;
}
