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

static GLUquadric* sphere;

struct aga_ctx ctx;

struct af_buf buf;
struct af_buf loadbuf;

struct aga_img img1;
struct af_buf tex1;

struct aga_img img2;
struct af_buf tex2;

af_uchar_t* pcm;
af_size_t pcm_len;
af_size_t pcm_pos = 0;

af_uchar_t* loaded;
af_size_t loaded_len;

static af_bool_t did_click = AF_FALSE;
static int last_button = -1;
static int click_pos[2] = { -1, -1 };

static void key(unsigned char k, int x, int y) {
	(void) x, (void) y;
	if(k == '\033') {
		gluDeleteQuadric(sphere);

		aga_af_chk("af_killbuf", af_killbuf(&ctx.af_ctx, &tex1));
		aga_af_chk("aga_killimg", aga_killimg(&img1));
		aga_af_chk("af_killbuf", af_killbuf(&ctx.af_ctx, &tex2));
		aga_af_chk("aga_killimg", aga_killimg(&img2));

		aga_af_chk("af_killbuf", af_killbuf(&ctx.af_ctx, &buf));
		aga_af_chk("af_killbuf", af_killbuf(&ctx.af_ctx, &loadbuf));

		if(ctx.settings.audio_enabled) {
			aga_af_chk(
				"AGA_KILL_LARGE_FILE_STRATEGY",
				AGA_KILL_LARGE_FILE_STRATEGY(pcm, pcm_len));
		}

		aga_af_chk(
			"AGA_KILL_LARGE_FILE_STRATEGY",
			AGA_KILL_LARGE_FILE_STRATEGY(loaded, loaded_len));

		aga_af_chk("aga_kill", aga_kill(&ctx));

		exit(EXIT_SUCCESS);
	}
}

static void click(int button, int state, int x, int y) {
	last_button = button;
	if(state == GLUT_UP) return;

	did_click = AF_TRUE;
	click_pos[0] = x;
	click_pos[1] = y;
}

static void motion(int x, int y) {
	static int old_x = -1, old_y = -1;

	if(did_click) {
		did_click = AF_FALSE;
		old_x = click_pos[0];
		old_y = click_pos[1];
	}

	{
		float dx = (float) (x - old_x);
		float dy = (float) (y - old_y);
		switch(last_button) {
			default: return;
			case GLUT_RIGHT_BUTTON: {
				ctx.cam.yaw += ctx.settings.sensitivity * dx;
				ctx.cam.pitch += ctx.settings.sensitivity * dy;
				break;
			}
			case GLUT_LEFT_BUTTON: {
				ctx.cam.dist += ctx.settings.zoom_speed * dy;
				aga_boundf(
					&ctx.cam.dist,
					ctx.settings.min_zoom, ctx.settings.max_zoom);
				break;
			}
		}

		aga_af_chk("aga_setcam", aga_setcam(&ctx));
	}

	old_x = x;
	old_y = y;
}

static void display(void) {
	const float clear[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	if(ctx.settings.audio_enabled) {
		af_size_t written;
		aga_af_chk("aga_flushsnd", aga_flushsnd(&ctx.snddev, &written));

		pcm_pos += written;
		if(pcm_pos < pcm_len) {
			af_size_t cpy = AGA_MIN(sizeof(ctx.snddev.buf), pcm_len - pcm_pos);
			af_memcpy(ctx.snddev.buf, &pcm[pcm_pos], cpy);
		}
	}

	aga_af_chk("af_clear", af_clear(&ctx.af_ctx, clear));

	aga_af_chk("af_settex", af_settex(&ctx.af_ctx, &tex1));
	{
		glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(0.0f, 0.0f, -4.0f);

		aga_af_chk(
			"af_draw",
			af_drawbuf(&ctx.af_ctx, &buf, &ctx.vert, AF_TRIANGLE_FAN));
	}

	{
		glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(
				-ctx.cam.pos.decomp.x,
				-ctx.cam.pos.decomp.y,
				-ctx.cam.pos.decomp.z);
		glColor3f(1.0f, 1.0f, 1.0f);
		glutSolidTeapot(1.0f);
	}

	aga_af_chk("af_settex", af_settex(&ctx.af_ctx, &tex2));
	{
		glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glScalef(10.0f, 1.0f, 10.0f);
			glTranslatef(0.0f, -4.0f, 0.0f);
			glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

		aga_af_chk(
			"af_draw",
			af_drawbuf(&ctx.af_ctx, &buf, &ctx.vert, AF_TRIANGLE_FAN));
	}

	{
		glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glScalef(1.0f, 1.0f, 1.0f);
			glTranslatef(-5.0f, -2.0f, 0.0f);

		aga_af_chk(
			"af_draw",
			af_drawbuf(&ctx.af_ctx, &loadbuf, &ctx.vert, AF_TRIANGLES));
	}

	{
		static double r = 0.0;
		glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotated((r += 0.5), 1.0, 1.0, 1.0);

		glColor3d(sin(r / 50.0), cos(r / 50.0), 0.1);
		gluSphere(sphere, 50.0, 20, 20);
	}

	{
		static float r = 0.0f;
		glDisable(GL_TEXTURE_2D);

		glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(-3.0f, 0.0f, -1.0f);
			glScalef(0.01f, 0.01f, 0.01f);
			glRotatef((r += 1.0f), 0.0f, 0.0f, 1.0f);

		glColor3f(1.0f, 1.0f, 1.0f);
		glLineWidth(5.0f);
		glutStrokeCharacter(GLUT_STROKE_ROMAN, '#');
		glutStrokeCharacter(GLUT_STROKE_ROMAN, 'W');
		glutStrokeCharacter(GLUT_STROKE_ROMAN, 'O');
		glutStrokeCharacter(GLUT_STROKE_ROMAN, 'O');
		glutStrokeCharacter(GLUT_STROKE_ROMAN, 'T');
		glutStrokeCharacter(GLUT_STROKE_ROMAN, '!');

		glEnable(GL_TEXTURE_2D);
	}

	aga_af_chk("af_flush", af_flush(&ctx.af_ctx));

	glutSwapBuffers();
	glutPostRedisplay();
}

int main(int argc, char** argv) {
	af_size_t i;

	struct aga_vertex vertices[] = {
		{
			{ -1.0f,  1.0f, 0.0f },
			{  1.0f,  0.0f, 0.0f, 0.7f },
			{  0.0f,  1.0f },
			{  0.0f,  1.0f, 0.0f }
		},
		{
			{  1.0f,  1.0f, 0.0f },
			{  0.0f,  1.0f, 0.0f, 0.7f },
			{  1.0f,  1.0f },
			{  0.0f,  1.0f, 0.0f }
		},
		{
			{  1.0f, -1.0f, 0.0f },
			{  1.0f,  1.0f, 0.0f, 0.7f },
			{  1.0f,  0.0f },
			{  0.0f,  1.0f, 0.0f }
		},
		{
			{ -1.0f, -1.0f, 0.0f },
			{  0.0f,  0.0f, 1.0f, 0.7f },
			{  0.0f,  0.0f },
			{  0.0f,  1.0f, 0.0f }
		}
	};

	aga_af_chk("aga_init", aga_init(&ctx, &argc, argv));
	ctx.cam.dist = 3.0f;

	sphere = gluNewQuadric();
	gluQuadricTexture(sphere, GL_TRUE);

	glutDisplayFunc(display);
	glutMotionFunc(motion);
	glutMouseFunc(click);
	glutKeyboardFunc(key);

	aga_af_chk("af_mkbuf", af_mkbuf(&ctx.af_ctx, &buf, AF_BUF_VERT));
	aga_af_chk(
		"af_upload", af_upload(&ctx.af_ctx, &buf, vertices, sizeof(vertices)));

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

	puts((const char*) glGetString(GL_VERSION));

	aga_setcam(&ctx);

	{
		struct aga_conf_node root = { 0 };
		aga_af_chk("aga_mkconf", aga_mkconf("res/test.sgml", &root));
		aga_af_chk("aga_killconf", aga_killconf(&root));
	}

	{
		struct aga_scriptclass* fizz;
		struct aga_scriptinst inst;
		for(i = 0; i < ctx.scripteng.len; ++i) {
			struct aga_scriptclass* class = &ctx.scripteng.classes[i];
			char* name = class->name;
			printf("- found script class `%s'\n", name);

			if(af_streql(name, "fizz")) fizz = class;
		}
		aga_af_chk("aga_mkscriptinst", aga_mkscriptinst(fizz, &inst));
	}

	if(ctx.settings.audio_enabled) {
		aga_af_chk(
			"AGA_MK_LARGE_FILE_STRATEGY",
			AGA_MK_LARGE_FILE_STRATEGY(
				"res/nggyu-u8pcm-8k.raw", &pcm, &pcm_len));
	}

	glutMainLoop();

	return 0;
}
