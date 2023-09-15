/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agaimg.h>
#include <agaconf.h>
#include <agasnd.h>
#include <agaio.h>
#include <agascript.h>

int main(int argc, char** argv) {
	struct aga_ctx ctx;

	GLUquadric* sphere;

	struct af_buf loadbuf;

	struct aga_img img1;
	struct af_buf tex1;
	struct aga_img img2;
	struct af_buf tex2;

	struct aga_clip nggyu = { 0 };

	struct aga_scriptclass* class;
	struct aga_scriptinst inst;

	af_uchar_t* loaded;
	af_size_t loaded_len;

	aga_af_chk("aga_init", aga_init(&ctx, argc, argv));

	sphere = gluNewQuadric();
	gluQuadricTexture(sphere, GL_TRUE);

	aga_af_chk(
		"AGA_MK_LARGE_FILE_STRATEGY",
		AGA_MK_LARGE_FILE_STRATEGY(
			"res/thing.raw", (af_uchar_t**) &loaded, &loaded_len));
	aga_af_chk("af_mkbuf", af_mkbuf(&ctx.af_ctx, &loadbuf, AF_BUF_VERT));
	aga_af_chk(
		"af_upload", af_upload(&ctx.af_ctx, &loadbuf, loaded, loaded_len));

	aga_af_chk("aga_mkimg", aga_mkimg(&img1, "res/arse.tiff"));
	aga_af_chk("aga_mkteximg", aga_mkteximg(&ctx.af_ctx, &img1, &tex1));

	aga_af_chk("aga_mkimg", aga_mkimg(&img2, "res/test.tiff"));
	aga_af_chk("aga_mkteximg", aga_mkteximg(&ctx.af_ctx, &img2, &tex2));

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

		aga_af_chk("aga_instcall", aga_instcall(&inst, "update"));

		{
			float clear[] = { 1.0f, 0.0f, 1.0f, 1.0f };
			aga_af_chk("af_clear", af_clear(&ctx.af_ctx, clear));
		}

		aga_af_chk("af_settex", af_settex(&ctx.af_ctx, &tex1));
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glScalef(1.0f, 1.0f, 1.0f);
			glTranslatef(-5.0f, -2.0f, 0.0f);

			aga_af_chk(
				"af_draw",
				af_drawbuf(&ctx.af_ctx, &loadbuf, &ctx.vert, AF_TRIANGLES));
		}

		aga_af_chk("af_settex", af_settex(&ctx.af_ctx, &tex2));
		{
			static double r = 0.0;
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotated((r += 0.5), 1.0, 1.0, 1.0);

			glColor3d(sin(r / 50.0), cos(r / 50.0), 0.1);
			gluSphere(sphere, 50.0, 20, 20);
		}

		aga_af_chk("af_flush", af_flush(&ctx.af_ctx));
		aga_af_chk("aga_swapbuf", aga_swapbuf(&ctx, &ctx.win));
	}

	gluDeleteQuadric(sphere);

	aga_af_chk("af_killbuf", af_killbuf(&ctx.af_ctx, &tex1));
	aga_af_chk("aga_killimg", aga_killimg(&img1));
	aga_af_chk("af_killbuf", af_killbuf(&ctx.af_ctx, &tex2));
	aga_af_chk("aga_killimg", aga_killimg(&img2));

	aga_af_chk("af_killbuf", af_killbuf(&ctx.af_ctx, &loadbuf));

	if(ctx.settings.audio_enabled) {
		aga_af_chk(
			"AGA_KILL_LARGE_FILE_STRATEGY",
			AGA_KILL_LARGE_FILE_STRATEGY(nggyu.pcm, nggyu.len));
	}

	aga_af_chk("aga_killscriptinst", aga_killscriptinst(&inst));

	aga_af_chk(
		"AGA_KILL_LARGE_FILE_STRATEGY",
		AGA_KILL_LARGE_FILE_STRATEGY(loaded, loaded_len));

	aga_af_chk("aga_kill", aga_kill(&ctx));

	return EXIT_SUCCESS;
}
