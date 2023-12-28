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

/*
 * Quick and dirty convenience macros to keep main clean.
 */
#define CHK(proc, param) aga_af_chk(__FILE__, #proc, proc param)
#define SOFT(proc, param) \
	do { \
        enum af_err soft_chk_result = proc param; \
		if(soft_chk_result) aga_af_soft(__FILE__, #proc, soft_chk_result); \
	} while(0)

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

	CHK(aga_setopts, (&opts, argc, argv));

    aga_log(__FILE__, "Initializing systems...");

    CHK(aga_mkwinenv, (&env, opts.display));
	CHK(aga_mkkeymap, (&keymap, &env));
	CHK(aga_mkwin, (opts.width, opts.height, &env, &win, argc, argv));

	CHK(aga_glctx, (&env, &win));
    CHK(af_mkctx, (&af, AF_FIDELITY_FAST));

    aga_log(__FILE__, "Acquired GL context");

    CHK(aga_setdrawparam, (&af, &vert));

#ifdef _DEBUG
# ifdef AGA_HAVE_SPAWN
	SOFT(aga_prerun_hook, (&opts));
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
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_KEYMAP, &keymap));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_POINTER, &pointer));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_OPTS, &opts));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_AFCTX, &af));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_AFVERT, &vert));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_SNDDEV, &snd));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_DIE, &die));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_WINENV, &env));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_WIN, &win));

		CHK(aga_findclass, (&scripteng, &class, "game"));
		CHK(aga_mkscriptinst, (&class, &inst));

		SOFT(aga_instcall, (&inst, "create"));
	    aga_log(__FILE__, "Hello, script engine!");
	}

    aga_log(__FILE__, "Done!");

	while(!die) {
		pointer.dx = 0;
		pointer.dy = 0;

		SOFT(aga_poll, (&env, &keymap, &win, &pointer, &die));

		if(class.class) SOFT(aga_instcall, (&inst, "update"));
		else {
			static const char str1[] =
				"No project loaded or no script files provided";
			static const char str2[] = "Did you forget `-f' or `-C'?";
			static const float col[4] = { 0.6f, 0.3f, 0.8f, 1.0f };

			CHK(af_clear, (&af, col));
			CHK(aga_puttextfmt, (-0.8f, 0.0f, str1));
			CHK(aga_puttextfmt, (-0.8f, -0.1f, str2));
		}

		SOFT(af_flush, (&af));

		/* Window is already dead/dying if `die' is set. */
		if(!die) SOFT(aga_swapbuf, (&env, &win));
	}

	aga_log(__FILE__, "Tearing down...");

	/* Need to flush before shutdown to avoid NSGL dying */
	SOFT(af_flush, (&af));

	if(class.class) {
		SOFT(aga_instcall, (&inst, "close"));
		SOFT(aga_killscriptinst, (&inst));
	}

	SOFT(aga_killscripteng, (&scripteng));
    SOFT(af_killvert, (&af, &vert));
    SOFT(af_killctx, (&af));

    if(opts.audio_enabled) SOFT(aga_killsnddev, (&snd));
	SOFT(aga_killconf, (&opts.config));

	SOFT(aga_killwin, (&env, &win));
	SOFT(aga_killkeymap, (&keymap));
	SOFT(aga_killwinenv, (&env));

	aga_log(__FILE__, "Bye-bye!");

	return 0;
}
