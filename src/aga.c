/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

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
#include <agaresgen.h>

#include <apro.h>

static enum aga_result aga_putnil(void) {
	/*
	 * TODO: We can definitely work on making this more useful. Maybe
	 * 		 An interactive way to load a project once we have some
	 * 		 Semblance of UI?
	 */

	enum aga_result result;

	static const char str1[] = "No project loaded or no script files provided";
	static const char str2[] = "Did you forget `-f' or `-C'?";
	static const float col[] = { 0.6f, 0.3f, 0.8f, 1.0f };

	result = aga_clear(col);
	if(result) return result;

	result = aga_puttextfmt(0.05f, 0.1f, str1);
	if(result) return result;

	return aga_puttextfmt(0.05f, 0.2f, str2);
}

#ifndef NDEBUG

static void aga_prof_put(unsigned y, unsigned x, enum apro_section s) {
	aga_ulong_t us = apro_stamp_us(s);
	float tx = 0.01f + (0.035f * (float) x);
	float ty = 0.05f + (0.035f * (float) y);
	enum aga_result result = aga_puttextfmt(
			tx, ty, "%s: %lluus", apro_section_name(s), us);
	aga_soft(__FILE__, "aga_puttextfmt", result);
}

#endif

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

	if(argc > 1 && aga_streql(argv[1], "mkres")) {
		unsigned i;
		static const struct {
			const char* type;
			enum aga_result (*proc)(const char*);
		} res[] = {
				{ "aga_resgen_img", aga_resgen_img },
				{ "aga_resgen_model", aga_resgen_model },
				{ "aga_resgen_pack", aga_resgen_pack },
				{ "aga_resgen_snd", aga_resgen_snd },
		};

		if(argc < 4) {
			aga_log(__FILE__, "err: usage: %s mkres {img|model|pack|snd} RES");
			aga_abort();
		}

		for(i = 0; i < AGA_LEN(res); ++i) {
			if(aga_streql(argv[2], res[i].type + sizeof("aga_resgen"))) {
				aga_check(__FILE__, res[i].type, res[i].proc(argv[3]));
				return 0;
			}
		}

		aga_log(__FILE__, "err: Unknown resource type `%s'", argv[2]);
		aga_abort();
	}

	aga_soft(__FILE__, "aga_setopts", aga_setopts(&opts, argc, argv));
	aga_soft(__FILE__, "aga_mkrespack", aga_mkrespack(opts.respack, &pack));
	aga_soft(__FILE__, "aga_setconf", aga_setconf(&opts, &pack));

	aga_log(__FILE__, "Initializing systems...");

	aga_check(
			__FILE__, "aga_mkwinenv", aga_mkwinenv(
					&env, opts.display));
	aga_check(
			__FILE__, "aga_mkkeymap", aga_mkkeymap(
					&keymap, &env));
	aga_check(
			__FILE__, "aga_mkwin", aga_mkwin(
					opts.width, opts.height, &env, &win, argc, argv));

	aga_check(__FILE__, "aga_glctx", aga_glctx(&env, &win));

	aga_log(__FILE__, "Acquired GL context");

	aga_check(__FILE__, "aga_setdrawparam", aga_setdrawparam());

#ifdef _DEBUG
# ifdef AGA_HAVE_SPAWN
	aga_soft(__FILE__, "aga_prerun_hook", aga_prerun_hook(&opts));
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
	aga_soft(__FILE__, "aga_mkscripteng", result);
	if(!result) {
		aga_soft(
				__FILE__, "aga_setscriptptr", aga_setscriptptr(
						&scripteng, AGA_SCRIPT_KEYMAP, &keymap));
		aga_soft(
				__FILE__, "aga_setscriptptr", aga_setscriptptr(
						&scripteng, AGA_SCRIPT_POINTER, &pointer));
		aga_soft(
				__FILE__, "aga_setscriptptr", aga_setscriptptr(
						&scripteng, AGA_SCRIPT_OPTS, &opts));
		aga_soft(
				__FILE__, "aga_setscriptptr", aga_setscriptptr(
						&scripteng, AGA_SCRIPT_SNDDEV, &snd));
		aga_soft(
				__FILE__, "aga_setscriptptr", aga_setscriptptr(
						&scripteng, AGA_SCRIPT_DIE, &die));
		aga_soft(
				__FILE__, "aga_setscriptptr", aga_setscriptptr(
						&scripteng, AGA_SCRIPT_WINENV, &env));
		aga_soft(
				__FILE__, "aga_setscriptptr", aga_setscriptptr(
						&scripteng, AGA_SCRIPT_WIN, &win));
		/* Pack gets set during script engine init. */
		/* SOFT(aga_setscriptptr, (&scripteng, AGA_SCRIPT_PACK, &pack)); */

		aga_check(
				__FILE__, "aga_findclass", aga_findclass(
						&scripteng, &class, "game"));
		aga_check(
				__FILE__, "aga_mkscriptinst", aga_mkscriptinst(
						&class, &inst));

		aga_soft(__FILE__, "aga_instcall", aga_instcall(&inst, "create"));
		aga_log(__FILE__, "Hello, script engine!");
	}

	aga_log(__FILE__, "Done!");

	while(!die) {
		apro_stamp_start(APRO_PRESWAP);

		apro_stamp_start(APRO_POLL);
		pointer.dx = 0;
		pointer.dy = 0;
		aga_soft(
				__FILE__, "aga_poll", aga_poll(
						&env, &keymap, &win, &pointer, &die));
		apro_stamp_end(APRO_POLL);

		apro_stamp_start(APRO_SCRIPT_UPDATE);
		if(class.class) {
			aga_soft(__FILE__, "aga_instcall", aga_instcall(&inst, "update"));
		}
		else { aga_soft(__FILE__, "aga_putnil", aga_putnil()); }
		apro_stamp_end(APRO_SCRIPT_UPDATE);

		apro_stamp_start(APRO_RES_SWEEP);
		aga_soft(__FILE__, "aga_sweeprespack", aga_sweeprespack(&pack));
		apro_stamp_end(APRO_RES_SWEEP);

		apro_stamp_end(APRO_PRESWAP);

#ifndef NDEBUG
		/* @formatter:off */
		{
			unsigned d = 0;
			unsigned x = 0;
			unsigned n = 0;

			aga_prof_put(d++, 0, APRO_PRESWAP);
				aga_prof_put(d++, 1, APRO_POLL);
				aga_prof_put(d++, 1, APRO_SCRIPT_UPDATE);
					aga_prof_put(d++, 2, APRO_SCRIPT_INSTCALL_RISING);
					aga_prof_put(d++, 2, APRO_SCRIPT_INSTCALL_EXEC);
						aga_prof_put(d++, 3, APRO_CEVAL_CALL_RISING);
						aga_prof_put(d++, 3, APRO_CEVAL_CALL_EVAL);
							aga_prof_put(d++, 4, APRO_CEVAL_CODE_EVAL_RISING);
							aga_prof_put(d++, 4, APRO_CEVAL_CODE_EVAL);
							aga_prof_put(d++, 4, APRO_CEVAL_CODE_EVAL_FALLING);
				aga_prof_put(d++, 1, APRO_RES_SWEEP);

			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_GETKEY);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_GETMOTION);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_SETCURSOR);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_SETCAM);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_TEXT);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_FOGPARAM);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_FOGCOL);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_CLEAR);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_MKTRANS);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_GETCONF);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_LOG);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_DIE);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_MKOBJ);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_INOBJ);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_PUTOBJ);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_KILLOBJ);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_OBJTRANS);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_OBJCONF);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_BITAND);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_BITSHL);
			aga_prof_put(x++, 12, APRO_SCRIPTGLUE_RANDNORM);

			aga_prof_put(n++, 20, APRO_PUTOBJ_RISING);
			aga_prof_put(n++, 20, APRO_PUTOBJ_LIGHT);
			aga_prof_put(n++, 20, APRO_PUTOBJ_CALL);
			aga_prof_put(n++, 20, APRO_PUTOBJ_FALLING);
		}
		/* @formatter:on */
#endif
		apro_clear();

		/* Window is already dead/dying if `die' is set. */
		if(!die) aga_soft(__FILE__, "aga_swapbuf", aga_swapbuf(&env, &win));
	}

	aga_log(__FILE__, "Tearing down...");

#ifdef __APPLE__
	/* Need to flush before shutdown to avoid NSGL dying */
	aga_soft(__FILE__, "aga_flush", aga_flush());
#endif

	if(class.class) {
		aga_soft(__FILE__, "aga_instcall", aga_instcall(&inst, "close"));
		aga_soft(__FILE__, "aga_killscriptinst", aga_killscriptinst(&inst));
	}

	aga_soft(__FILE__, "aga_killscripteng", aga_killscripteng(&scripteng));

	if(opts.audio_enabled) {
		aga_soft(__FILE__, "aga_killsnddev", aga_killsnddev(&snd));
	}

	aga_soft(__FILE__, "aga_killconf", aga_killconf(&opts.config));

	aga_soft(__FILE__, "aga_killwin", aga_killwin(&env, &win));
	aga_soft(__FILE__, "aga_killkeymap", aga_killkeymap(&keymap));
	aga_soft(__FILE__, "aga_killwinenv", aga_killwinenv(&env));
	aga_soft(__FILE__, "aga_killrespack", aga_killrespack(&pack));

	aga_log(__FILE__, "Bye-bye!");

	return 0;
}
