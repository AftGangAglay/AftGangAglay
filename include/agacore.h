/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_CORE_H
#define AGA_CORE_H

#include <agasnd.h>
#include <agascript.h>
#include <agaconf.h>
#include <agawin.h>

#include <afeirsa/afeirsa.h>

#ifdef AF_HAVE_GNU
# if __has_attribute(noreturn)
#  define AGA_NORETURN __attribute__((noreturn))
# endif
#endif

/*
 * NOTE: This exists for cases where we are forced to use fixed size buffers
 * 		 Due to limitations like the nonexistence of `vsnprintf'.
 * 		 This is NOT an excuse to use this pattern unnecessarily - play nice
 * 		 With your buffers.
 */
typedef char aga_fixed_buf_t[2048 + 1];

struct aga_vertex {
	float col[4];
	float uv[2];
	float norm[3];
	float pos[3];
};

struct aga_ctx {
	struct af_ctx af_ctx;

	char** argv;
	int argc;

	void* dpy;
	int dpy_fd;
	int screen;
	struct aga_win win;
	void* glx;
	af_bool_t double_buffered;
	af_ulong_t wm_delete;
	af_bool_t die;

	int keysyms_per_keycode;
	int keycode_len;
	int keycode_min;
	af_ulong_t* keymap;
	af_bool_t* keystates;

	int pointer_dx;
	int pointer_dy;

	struct aga_snddev snddev;
	struct aga_scripteng scripteng;

	struct af_vert vert;

	struct aga_conf_node conf;
	struct aga_settings {
		/* Input */
		float sensitivity;
		float move_speed;

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

void aga_boundf(float* f, float min, float max);

#endif
