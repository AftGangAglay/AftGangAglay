/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agaimg.h>
#include <agasnd.h>
#include <agaio.h>
#include <agalog.h>
#include <agastd.h>
#include <agascript.h>
#include <agadraw.h>

int main(int argc, char** argv) {
	struct aga_ctx ctx;
	enum af_err result;

	struct aga_scripteng scripteng;
	struct aga_scriptclass* class = 0;
	struct aga_scriptinst inst;

	const char* logfiles[] = { AGA_SYSOUT, "aga.log" };
	aga_mklog(logfiles, AF_ARRLEN(logfiles));

	aga_log(__FILE__, "Breathing in the chemicals...");

	aga_af_chk(__FILE__, "aga_init", aga_init(&ctx, argc, argv));

#ifdef _DEBUG
	do {
		const char* hook[] = { "Development", "PreHook" };
		char* program = getenv("SHELL");
		char* args[] = { 0 /* shell */, 0 /* -c */, 0 /* exec */, 0 };
		char* pdir = strrchr(ctx.conf_path, '/');

# ifdef _WINDOWS
		if(!program) program = "cmd";
		args[1] = "/c";
# else
		if(!program) program = "sh";
		args[1] = "-c";
# endif

		args[0] = program;

		result = aga_conftree(
				&ctx.conf, hook, AF_ARRLEN(hook), &args[2], AGA_STRING);
		if(result) break;

		aga_log(__FILE__, "Executing project pre-run hook `%s'", args[2]);

		if(pdir) {
			af_size_t pdir_len = (af_size_t) (pdir - ctx.conf_path) + 1;
			pdir = calloc(pdir_len + 1, sizeof(char));
			if(!pdir) {
				(void) aga_af_errno(__FILE__, "calloc");
				break;
			}
			strncpy(pdir, ctx.conf_path, pdir_len);
		}
		result = aga_spawn_sync(program, args, pdir);
		if(result) aga_af_soft(__FILE__, "aga_spawn_sync", result);
		free(pdir);
	} while(0);
#endif

	{
		const char* startup_script = "script/main.py";
		const char* python_path = "vendor/python/lib:script:script/aga";

		const char* startup[] = { "Script", "Startup" };
		const char* path[] = { "Script", "Path" };

		result = aga_conftree(
			&ctx.conf, startup, AF_ARRLEN(startup),
			&startup_script, AGA_STRING);
		if(result) aga_af_soft(__FILE__, "aga_conftree", result);

		result = aga_conftree(
				&ctx.conf, path, AF_ARRLEN(path),
				&python_path, AGA_STRING);
		if(result) aga_af_soft(__FILE__, "aga_conftree", result);

		result = aga_mkscripteng(
			&ctx, &scripteng, startup_script, python_path, argc, argv);
	}
	if(result) aga_af_soft(__FILE__, "aga_mkscripteng", result);
	else {
		aga_af_chk(__FILE__, "aga_findclass", aga_findclass(
			&scripteng, &class, "game"));
		aga_af_chk(__FILE__, "aga_mkscriptinst", aga_mkscriptinst(
			class, &inst));

		result = aga_instcall(&inst, "create");
		if(result) aga_af_soft(__FILE__, "aga_instcall", result);
	}

	ctx.die = AF_FALSE;
	while(!ctx.die) {
		result = aga_poll(&ctx);
		if(result) aga_af_soft(__FILE__, "aga_poll", result);

		if(class) {
			result = aga_instcall(&inst, "update");
			if(result) aga_af_soft(__FILE__, "aga_instcall", result);
		}
		else {
			float col[4] = { 0.6f, 0.3f, 0.8f, 1.0f };
			aga_af_chk(__FILE__, "af_clear", af_clear(&ctx.af_ctx, col));
			aga_af_chk(__FILE__, "aga_puttextfmt", aga_puttextfmt(
				-0.8f, 0.0f, "No project loaded or no script files provided"));
			aga_af_chk(__FILE__, "aga_puttextfmt", aga_puttextfmt(
				-0.8f, -0.1f, "Did you forget `-f' or `-C'?"));
		}

		result = af_flush(&ctx.af_ctx);
		if(result) aga_af_soft(__FILE__, "af_flush", result);

		if(!ctx.die) { /* Window is already dead/dying if `die' is set. */
			result = aga_swapbuf(&ctx, &ctx.win);
			if(result) aga_af_soft(__FILE__, "aga_swapbuf", result);
		}
	}

	aga_log(__FILE__, "Tearing down...");

	/* Need to flush before shutdown to avoid NSGL dying */
	result = af_flush(&ctx.af_ctx);
	if(result) aga_af_soft(__FILE__, "af_flush", result);

	if(class) {
		aga_af_chk(__FILE__, "aga_instcall", aga_instcall(&inst, "close"));
		aga_af_chk(__FILE__, "aga_killscriptinst", aga_killscriptinst(&inst));
	}

	aga_af_chk(__FILE__, "aga_killscripteng", aga_killscripteng(&scripteng));
	aga_af_chk(__FILE__, "aga_kill", aga_kill(&ctx));

	aga_log(__FILE__, "Bye-bye!");

	return 0;
}
