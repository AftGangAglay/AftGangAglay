/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_CORE_H
#define AGA_CORE_H

/*
 * TODO: We should consider hiding all non-AGA includes in implementations to
 * 		 Keep a sane, abstracted public API.
 */

#include <agamath.h>
#include <agasnd.h>
#include <agascript.h>
#include <agaconf.h>
#include <agawin.h>

#include <afeirsa/afeirsa.h>
#include <afeirsa/afgl.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#ifdef AF_HAVE_GNU
# if __has_attribute(noreturn)
#  define AGA_NORETURN __attribute__((noreturn))
# endif
#endif

struct aga_vertex {
	float pos[3];
	float col[4];
	float uv[2];
	float norm[3];
};

struct aga_cam {
	union aga_vec3 pos;
	float yaw;
	float pitch;
	float dist;
};

struct aga_ctx {
	struct af_ctx af_ctx;

	char** argv;
	int argc;

	void* dpy;
	int dpy_fd;
	int screen;
	struct aga_win win;
	GLXContext glx;
	af_bool_t double_buffered;
	af_ulong_t wm_delete;
	af_bool_t die;

	struct aga_snddev snddev;
	struct aga_scripteng scripteng;

	struct af_vert vert;
	struct aga_cam cam;

	struct aga_conf_node conf;
	struct aga_settings {
		/* Input */
		float sensitivity;
		float zoom_speed;
		float min_zoom, max_zoom;

		/* Display */
		float fov;
		unsigned width, height;

		/* Audio */
		af_bool_t audio_enabled;
		const char* audio_dev;

		/* Script */
		const char* startup_script;
		/* `:' separated list of search paths */
		const char* python_path;
	} settings;
};

enum af_err aga_init(struct aga_ctx* ctx, int argc, char** argv);
enum af_err aga_kill(struct aga_ctx* ctx);

enum af_err aga_setcam(struct aga_ctx* ctx);

/*
 * NOTE: The use of `chk' is somewhat inconsistent in that the base `AF_'
 * 		 Macros bubble up soft errors whereas the `aga_*_chk' family of
 * 		 Functions are fatal. Just something to keep in mind when error
 * 		 Handling. We may want to add a soft-error form of the errno handling
 * 		 At some point, returning `AF_ERR_ERRNO' and having the user do a
 * 		 `perror' or something.
 */
void aga_af_chk(const char* proc, enum af_err e);
AGA_NORETURN void aga_errno_chk(const char* proc);
AGA_NORETURN void aga_fatal(const char* fmt, ...);

void aga_boundf(float* f, float min, float max);

#endif
