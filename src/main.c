/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agapcx.h>

static GLUquadric* sphere;

struct aga_ctx ctx;

struct af_drawlist drawlist;
struct af_buf buf;
struct af_buf tex;

static af_bool_t did_click = AF_FALSE;
static int last_button = -1;
static int click_pos[2] = { -1, -1 };

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

	aga_af_chk("af_clear", af_clear(&ctx.af_ctx, clear));

	{
		glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(0.0f, 0.0f, -4.0f);
		aga_af_chk("af_draw", af_draw(&ctx.af_ctx, &drawlist));
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

	{
		glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glScalef(10.0f, 1.0f, 10.0f);
			glTranslatef(0.0f, -4.0f, 0.0f);
			glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
		aga_af_chk("af_draw", af_draw(&ctx.af_ctx, &drawlist));
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

	enum _ { texdim = 128 };
	af_uchar_t texdata[texdim * texdim * 4];

	struct af_drawop drawops[2];

	drawops[0].type = AF_SETTEX;
	drawops[0].data.settex = &tex;

	drawops[1].type = AF_DRAWBUF;
	drawops[1].data.drawbuf.vert = &ctx.vert;
	drawops[1].data.drawbuf.buf = &buf;
	drawops[1].data.drawbuf.primitive = AF_TRIANGLE_FAN;

	{
		af_size_t i;

		af_uchar_t* file;
		long len;
		size_t r;

		{
			FILE* f = fopen("res/arse.pcx", "r");
			int res;

			if(!f) aga_errno_chk("fopen");

			res = fseek(f, 0, SEEK_END);
			if(res == -1) aga_errno_chk("fseek");

			len = ftell(f);
			if(len == -1) aga_errno_chk("ftell");

			rewind(f);

			if(!(file = malloc((size_t) len))) aga_errno_chk("malloc");

			r = fread(file, sizeof(af_uchar_t), (size_t) len, f);
			if(r < (size_t) len && ferror(f)) aga_errno_chk("fread");

			res = fclose(f);
			if(res == -1) aga_errno_chk("fclose");
		}

		srand(time(0));
		for(i = 0; i < sizeof(texdata); ++i) texdata[i] = rand();
	}


	/* TODO: Load defaults from file */
	ctx.settings.sensitivity = 0.25f;
	ctx.settings.zoom_speed = 0.1f;
	ctx.settings.min_zoom = 2.0f;
	ctx.settings.max_zoom = 10.0f;

	ctx.settings.width = 640;
	ctx.settings.height = 480;
	ctx.settings.fov = 60.0f;

	aga_af_chk("aga_init", aga_init(&ctx, &argc, argv));
	ctx.cam.dist = 3.0f;

	sphere = gluNewQuadric();
	gluQuadricTexture(sphere, GL_TRUE);

	glutDisplayFunc(display);
	glutMotionFunc(motion);
	glutMouseFunc(click);

	aga_af_chk(
		"af_mkbuf", af_mkbuf(&ctx.af_ctx, &buf, AF_BUF_VERT));
	aga_af_chk(
		"af_upload", af_upload(&ctx.af_ctx, &buf, vertices, sizeof(vertices)));

	aga_af_chk(
		"af_mkbuf", af_mkbuf(&ctx.af_ctx, &tex, AF_BUF_TEX));
	tex.tex_width = texdim;
	aga_af_chk(
		"af_upload", af_upload(&ctx.af_ctx, &tex, texdata, sizeof(texdata)));

	aga_af_chk(
		"af_mkdrawlist",
		af_mkdrawlist(&ctx.af_ctx, &drawlist, drawops, AF_ARRLEN(drawops)));

	puts((const char*) glGetString(GL_VERSION));

	aga_setcam(&ctx);

	glutMainLoop();

	aga_af_chk("af_killdrawlist", af_killdrawlist(&ctx.af_ctx, &drawlist));
	aga_af_chk("af_killbuf", af_killbuf(&ctx.af_ctx, &tex));
	aga_af_chk("af_killbuf", af_killbuf(&ctx.af_ctx, &buf));

	aga_af_chk("aga_kill", aga_kill(&ctx));

	return 0;
}
