/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agaimg.h>
#include <agasnd.h>
#include <agalog.h>
#include <agascript.h>

int main(int argc, char** argv) {
	struct aga_ctx ctx;

	struct aga_scriptclass* class;
	struct aga_scriptinst inst;

	const char* logfiles[] = { "/dev/stdout", "aga.log" };
	aga_af_chk("aga_mklog", aga_mklog(logfiles, AF_ARRLEN(logfiles)));

	aga_log(__FILE__, "Breathing in the chemicals...");

	aga_af_chk("aga_init", aga_init(&ctx, argc, argv));

	aga_af_chk("aga_setcam", aga_setcam(&ctx));

	aga_af_chk(
		"aga_findclass", aga_findclass(&ctx.scripteng, &class, "game"));
	aga_af_chk("aga_mkscriptinst", aga_mkscriptinst(class, &inst));

	glEnable(GL_LIGHTING);
	aga_af_chk("glEnable", af_gl_chk());

	{
		af_size_t i;
		float pos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float (poss[])[3] = {
			{ 0.0f, 0.0f, 0.0f },
			{ 45.0f, 0.0f, 0.0f }
		};
		float col[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		for(i = 0; i < AF_ARRLEN(poss); ++i) {
			glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				glTranslatef(poss[i][0], poss[i][1], poss[i][2]);

			glEnable(GL_LIGHT0 + i);
			aga_af_chk("glEnable", af_gl_chk());

			glLightfv(GL_LIGHT0 + i, GL_POSITION, pos);
			aga_af_chk("glLightfv", af_gl_chk());

			glLightfv(GL_LIGHT0 + i, GL_AMBIENT, col);
			aga_af_chk("glLightfv", af_gl_chk());

			glLighti(GL_LIGHT0 + i, GL_SPOT_EXPONENT, 128);
			aga_af_chk("glLighti", af_gl_chk());

			glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 0.0f);
			aga_af_chk("glLightf", af_gl_chk());
			glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 0.0f);
			aga_af_chk("glLightf", af_gl_chk());
			glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.01f);
			aga_af_chk("glLightf", af_gl_chk());
		}
	}

	ctx.die = AF_FALSE;
	while(!ctx.die) {
		aga_af_chk("aga_poll", aga_poll(&ctx));

		{
			float clear[] = { 1.0f, 0.0f, 1.0f, 1.0f };
			aga_af_chk("af_clear", af_clear(&ctx.af_ctx, clear));
		}

		aga_af_chk("aga_instcall", aga_instcall(&inst, "update"));

		aga_af_chk("af_flush", af_flush(&ctx.af_ctx));
		aga_af_chk("aga_swapbuf", aga_swapbuf(&ctx, &ctx.win));
	}

	aga_log(__FILE__, "Tearing down...");

	aga_af_chk("aga_instcall", aga_instcall(&inst, "close"));

	aga_af_chk("aga_killscriptinst", aga_killscriptinst(&inst));

	aga_af_chk("aga_kill", aga_kill(&ctx));

	aga_log(__FILE__, "Bye-bye!");
	aga_af_chk("aga_killlog", aga_killlog());

	return EXIT_SUCCESS;
}
