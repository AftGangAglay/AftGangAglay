/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agawin.h>
#include <agasnd.h>
#include <agalog.h>
#include <agaerr.h>
#include <agadraw.h>
#include <agastartup.h>
#include <agascript.h>

int main(int argc, char** argv) {
	enum af_err result;

	struct aga_opts opts;

    struct af_ctx af;
    struct af_vert vert;

    struct aga_snddev snd;

	union aga_winenv env;
	struct aga_win win;
	struct aga_keymap keymap;
	struct aga_pointer pointer;

	struct aga_scripteng scripteng;
	struct aga_scriptclass class = { 0 };
	struct aga_scriptinst inst;

    af_bool_t die = AF_FALSE;

	const char* logfiles[] = { 0 /* auto stdout */, "aga.log" };
	aga_mklog(logfiles, AF_ARRLEN(logfiles));

	aga_log(__FILE__, "Breathing in the chemicals...");

    result = aga_setopts(&opts, argc, argv);
	if(result) aga_af_soft(__FILE__, "aga_setopts", result);

    aga_log(__FILE__, "Initializing systems...");

    aga_af_chk(__FILE__, "aga_mkwinenv", aga_mkwinenv(&env, opts.display));
	aga_af_chk(__FILE__, "aga_mkkeymap", aga_mkkeymap(&keymap, &env));
	aga_af_chk(__FILE__, "aga_mkwin",
	   aga_mkwin(opts.width, opts.height, &env, &win, argc, argv));
	aga_af_chk(__FILE__, "aga_glctx", aga_glctx(&env, &win));
    aga_af_chk(__FILE__, "af_mkctx", af_mkctx(&af, AF_FIDELITY_FAST));

    aga_log(__FILE__, "Acquired GL context");

    aga_af_chk(__FILE__, "aga_setdrawparam", aga_setdrawparam(&af, &vert));

#ifdef _DEBUG
# ifdef AGA_HAVE_SPAWN
	result = aga_prerun_hook(&opts);
	if(result) aga_af_soft(__FILE__, "aga_setopts", result);
# endif
#endif

    if(opts.audio_enabled) {
        result = aga_mksnddev(opts.audio_dev, &snd);
        if(result) {
            aga_af_soft(__FILE__, "aga_mksnddev", result);
            opts.audio_enabled = AF_FALSE;
        }
    }

    result = aga_mkscripteng(
        &scripteng, opts.startup_script, opts.python_path, argc, argv);
	if(result) aga_af_soft(__FILE__, "aga_mkscripteng", result);
	else {
		result = aga_setscriptptr(&scripteng, AGA_SCRIPT_KEYMAP, &keymap);
		if(result) aga_af_soft(__FILE__, "aga_setscriptptr", result);
		result = aga_setscriptptr(&scripteng, AGA_SCRIPT_POINTER, &pointer);
		if(result) aga_af_soft(__FILE__, "aga_setscriptptr", result);
		result = aga_setscriptptr(&scripteng, AGA_SCRIPT_OPTS, &opts);
		if(result) aga_af_soft(__FILE__, "aga_setscriptptr", result);
		result = aga_setscriptptr(&scripteng, AGA_SCRIPT_AFCTX, &af);
		if(result) aga_af_soft(__FILE__, "aga_setscriptptr", result);
		result = aga_setscriptptr(&scripteng, AGA_SCRIPT_AFVERT, &vert);
		if(result) aga_af_soft(__FILE__, "aga_setscriptptr", result);
		result = aga_setscriptptr(&scripteng, AGA_SCRIPT_SNDDEV, &snd);
		if(result) aga_af_soft(__FILE__, "aga_setscriptptr", result);
		result = aga_setscriptptr(&scripteng, AGA_SCRIPT_DIE, &die);
		if(result) aga_af_soft(__FILE__, "aga_setscriptptr", result);

		aga_af_chk(__FILE__, "aga_findclass", aga_findclass(
			&scripteng, &class, "game"));
		aga_af_chk(__FILE__, "aga_mkscriptinst", aga_mkscriptinst(
			&class, &inst));

		result = aga_instcall(&inst, "create");
		if(result) aga_af_soft(__FILE__, "aga_instcall", result);
	}
    aga_log(__FILE__, "Hello, script engine!");

    aga_log(__FILE__, "Done!");

	while(!die) {
		result = aga_poll(&env, &keymap, &win, &pointer, &die);
		if(result) aga_af_soft(__FILE__, "aga_poll", result);

		if(class.class) {
			result = aga_instcall(&inst, "update");
			if(result) aga_af_soft(__FILE__, "aga_instcall", result);
		}
		else {
			float col[4] = { 0.6f, 0.3f, 0.8f, 1.0f };
			aga_af_chk(__FILE__, "af_clear", af_clear(&af, col));
			aga_af_chk(__FILE__, "aga_puttextfmt", aga_puttextfmt(
				-0.8f, 0.0f, "No project loaded or no script files provided"));
			aga_af_chk(__FILE__, "aga_puttextfmt", aga_puttextfmt(
				-0.8f, -0.1f, "Did you forget `-f' or `-C'?"));
		}

		result = af_flush(&af);
		if(result) aga_af_soft(__FILE__, "af_flush", result);

		aga_af_chk(__FILE__, "malloc", AF_ERR_MEM);

		if(!die) { /* Window is already dead/dying if `die' is set. */
			result = aga_swapbuf(&env, &win);
			if(result) aga_af_soft(__FILE__, "aga_swapbuf", result);
		}
	}

	aga_log(__FILE__, "Tearing down...");

	/* Need to flush before shutdown to avoid NSGL dying */
	result = af_flush(&af);
	if(result) aga_af_soft(__FILE__, "af_flush", result);

	if(class.class) {
		aga_af_chk(__FILE__, "aga_instcall", aga_instcall(&inst, "close"));
		aga_af_chk(__FILE__, "aga_killscriptinst", aga_killscriptinst(&inst));
	}

	aga_af_chk(__FILE__, "aga_killscripteng", aga_killscripteng(&scripteng));
    aga_af_chk(__FILE__, "af_killvert", af_killvert(&af, &vert));
    aga_af_chk(__FILE__, "af_killctx", af_killctx(&af));

    if(opts.audio_enabled) {
        aga_af_chk(__FILE__, "aga_killsnddev", aga_killsnddev(&snd));
    }
	aga_af_chk(__FILE__, "aga_killconf", aga_killconf(&opts.config));

	aga_af_chk(__FILE__, "aga_killwin", aga_killwin(&env, &win));
	aga_af_chk(__FILE__, "aga_killkeymap", aga_killkeymap(&keymap));
	aga_af_chk(__FILE__, "aga_killwinenv", aga_killwinenv(&env));

	aga_log(__FILE__, "Bye-bye!");

	return 0;
}
