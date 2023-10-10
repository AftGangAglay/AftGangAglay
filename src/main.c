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

	enum af_err result;

	const char* logfiles[] = { "/dev/stdout", "aga.log" };

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
		float col[] = {1.0f, 1.0f, 1.0f};
		glFogfv(GL_FOG_COLOR, col);
		aga_af_chk(__FILE__, "glFogfv", af_gl_chk());
	}

	glFogf(GL_FOG_DENSITY, 0.1f);
	aga_af_chk(__FILE__, "glFogf", af_gl_chk());
	glFogf(GL_FOG_START, 0.0f);
	aga_af_chk(__FILE__, "glFogf", af_gl_chk());
	glFogf(GL_FOG_END, 10.0f);
	aga_af_chk(__FILE__, "glFogf", af_gl_chk());

	ctx.die = AF_FALSE;
	while(!ctx.die) {
		result = aga_poll(&ctx);
		if(result) aga_af_soft(__FILE__, "aga_poll", result);

		{
			float clear[] = { 1.0f, 0.0f, 1.0f, 1.0f };
			aga_af_chk(__FILE__, "af_clear", af_clear(&ctx.af_ctx, clear));
		}
		result = aga_instcall(&inst, "update");
		if(result) aga_af_soft(__FILE__, "aga_instcall", result);

		{
			aga_fixed_buf_t msg = { 0 };
			const char* c;

			int sprintf(char * str, const char* format, ...);
			long time(long* tloc);

			sprintf(msg, "%li", time(0));

			glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();

			glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();

			glRasterPos4f(0.0f, 0.0f, 0.0f, 1.0f);
			aga_af_chk(__FILE__, "glRasterPos4f", af_gl_chk());

			glMatrixMode(GL_MODELVIEW);
				glPopMatrix();

			glMatrixMode(GL_PROJECTION);
				glPopMatrix();

			for(c = msg; *c; ++c) {
				glCallList(ctx.font_base + (*c - (' ')));
				aga_af_chk(__FILE__, "glCallList", af_gl_chk());
			}
		}

		result = af_flush(&ctx.af_ctx);
		if(result) aga_af_soft(__FILE__, "af_flush", result);
		result = aga_swapbuf(&ctx, &ctx.win);
		if(result) aga_af_soft(__FILE__, "aga_swapbuf", result);
	}

	aga_log(__FILE__, "Tearing down...");

	aga_af_chk(__FILE__, "aga_instcall", aga_instcall(&inst, "close"));

	aga_af_chk(__FILE__, "aga_killscriptinst", aga_killscriptinst(&inst));

	aga_af_chk(__FILE__, "aga_kill", aga_kill(&ctx));

	aga_log(__FILE__, "Bye-bye!");
	aga_af_chk(__FILE__, "aga_killlog", aga_killlog());

	return EXIT_SUCCESS;
}
