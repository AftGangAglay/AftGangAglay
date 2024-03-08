/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

/* TODO: Rename to `aftgangaglay.c' */

/* TODO: Fuzz headless. */

#include <agawin.h>
#include <agasnd.h>
#include <agautil.h>
#include <agalog.h>
#include <agaerr.h>
#include <agapack.h>
#include <agaio.h>
#include <agadraw.h>
#include <agastartup.h>
#include <agascript.h>

#include <apro.h>

/*
 * Quick and dirty convenience macros to keep main clean. Ideally in the future
 * We can find a less janky way to achieve this but for now `main' has the
 * Highest density of `agaerr' calls so these have a dramatic impact on
 * Readability.
 */
#define CHK(proc, param) aga_chk(__FILE__, #proc, proc param)
#define SOFT(proc, param) \
    do { \
        enum aga_result soft_chk_result = proc param; \
        if(soft_chk_result) aga_soft(__FILE__, #proc, soft_chk_result); \
    } while(0)

static enum aga_result aga_putnil(void) {
	/*
	 * TODO: We can definitely work on making this more useful. Maybe
	 * 		 An interactive way to load a project once we have some
	 * 		 Semblance of UI?
	 */

	static const char str1[] = "No project loaded or no script files provided";
	static const char str2[] = "Did you forget `-f' or `-C'?";
	static const float col[] = { 0.6f, 0.3f, 0.8f, 1.0f };

	AGA_CHK(aga_clear(col));
	AGA_CHK(aga_puttextfmt(0.05f, 0.1f, str1));
	AGA_CHK(aga_puttextfmt(0.05f, 0.2f, str2));

	return AGA_RESULT_OK;
}

int main(int argc, char** argv) {
	enum aga_result result;

	struct aga_opts opts;

	struct aga_respack pack;

	struct aga_snddev snd;

	struct aga_winenv env;
	struct aga_win win;
	struct aga_keymap keymap;
	struct aga_pointer pointer;

	struct aga_scripteng scripteng;
	struct aga_scriptclass class = { 0 };
	struct aga_scriptinst inst;

	aga_bool_t die = AGA_FALSE;

	const char* logfiles[] = { 0 /* auto stdout */, "aga.log" };
	aga_mklog(logfiles, AGA_LEN(logfiles));

	aga_log(__FILE__, "Breathing in the chemicals...");

	SOFT(aga_setopts, (&opts, argc, argv));
	SOFT(aga_mkrespack, (opts.respack, &pack));
	SOFT(aga_setconf, (&opts, &pack));

	aga_log(__FILE__, "Initializing systems...");

	CHK(aga_mkwinenv, (&env, opts.display));
	CHK(aga_mkkeymap, (&keymap, &env));
	CHK(aga_mkwin, (opts.width, opts.height, &env, &win, argc, argv));

	CHK(aga_glctx, (&env, &win));

	aga_log(__FILE__, "Acquired GL context");

	CHK(aga_setdrawparam, ());

#ifdef _DEBUG
# ifdef AGA_HAVE_SPAWN
	SOFT(aga_prerun_hook, (&opts));
# endif
#endif

	if(opts.audio_enabled) {
		result = aga_mksnddev(opts.audio_dev, &snd);
		if(result) {
			aga_soft(__FILE__, "aga_mksnddev", result);
			opts.audio_enabled = AGA_FALSE;
		}
	}

	if(!aga_streql(opts.version, AGA_VERSION)) {
		static const char err[] = "err: Project version `%s' does not match "
								  "engine version `" AGA_VERSION "'";
		aga_log(__FILE__, err, opts.version);
	}

	aga_log(__FILE__, "Starting up the script engine...");

	result = aga_mkscripteng(
			&scripteng, opts.startup_script, &pack, opts.python_path);
	if(result) aga_soft(__FILE__, "aga_mkscripteng", result);
	else {
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_KEYMAP, &keymap));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_POINTER, &pointer));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_OPTS, &opts));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_SNDDEV, &snd));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_DIE, &die));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_WINENV, &env));
		SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_WIN, &win));
		/* Pack gets set during script engine init. */
		/* SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_PACK, &pack)); */

		CHK(aga_findclass, (&scripteng, &class, "game"));
		CHK(aga_mkscriptinst, (&class, &inst));

		SOFT(aga_instcall, (&inst, "create"));
		aga_log(__FILE__, "Hello, script engine!");
	}

	aga_log(__FILE__, "Done!");

	while(!die) {
		apro_stamp_start(APRO_PRESWAP);

		apro_stamp_start(APRO_POLL);
		pointer.dx = 0;
		pointer.dy = 0;
		SOFT(aga_poll, (&env, &keymap, &win, &pointer, &die));
		apro_stamp_end(APRO_POLL);

		apro_stamp_start(APRO_SCRIPT_UPDATE);
		if(class.class) SOFT(aga_instcall, (&inst, "update"));
		else SOFT(aga_putnil, ());
		apro_stamp_end(APRO_SCRIPT_UPDATE);

		apro_stamp_start(APRO_RES_SWEEP);
		SOFT(aga_sweeprespack, (&pack));
		apro_stamp_end(APRO_RES_SWEEP);

		apro_stamp_end(APRO_PRESWAP);

#ifndef NDEBUG
# define PROF(i, j, sec) \
		SOFT(aga_puttextfmt, ( \
				0.05f + 0.05f * (j), 0.1f + 0.05f * (i), #sec ": %lluus", \
				apro_stamp_us(APRO_##sec)))

		{
			unsigned d = 0;
			PROF(d++, 0, PRESWAP);
				PROF(d++, 1, POLL);
				PROF(d++, 1, SCRIPT_UPDATE);
					PROF(d++, 2, SCRIPT_INSTCALL_RISING);
					PROF(d++, 2, SCRIPT_INSTCALL_EXEC);
						PROF(d++, 3, CEVAL_CALL_RISING);
						PROF(d++, 3, CEVAL_CALL_EVAL);
				PROF(d++, 1, RES_SWEEP);
		}
# undef PROF
#endif
		apro_clear();

		/* Window is already dead/dying if `die' is set. */
		if(!die) SOFT(aga_swapbuf, (&env, &win));
	}

	aga_log(__FILE__, "Tearing down...");

	/* Need to flush before shutdown to avoid NSGL dying */
	SOFT(aga_flush, ());

	if(class.class) {
		SOFT(aga_instcall, (&inst, "close"));
		SOFT(aga_killscriptinst, (&inst));
	}

	SOFT(aga_killscripteng, (&scripteng));

	if(opts.audio_enabled) SOFT(aga_killsnddev, (&snd));
	SOFT(aga_killconf, (&opts.config));

	SOFT(aga_killwin, (&env, &win));
	SOFT(aga_killkeymap, (&keymap));
	SOFT(aga_killwinenv, (&env));
	SOFT(aga_killrespack, (&pack));

	aga_log(__FILE__, "Bye-bye!");

	return 0;
}
