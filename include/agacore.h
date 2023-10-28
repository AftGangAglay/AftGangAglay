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

	/* TODO: Document what fields `agawwin' yoinks. */
	void* dpy;
	int dpy_fd;
	int screen;
	struct aga_win win;
	void* glx;
	af_bool_t double_buffered;
	af_ulong_t wm_delete;
	af_bool_t die;
	af_bool_t debugdraw;

	af_size_t frame_verts;
	af_size_t frame_us;

	int keysyms_per_keycode;
	int keycode_len;
	int keycode_min;
	af_ulong_t* keymap;
	/* TODO: Ensure keystates are in-bounds */
	af_bool_t* keystates;

	int pointer_dx;
	int pointer_dy;

	unsigned font_base;

	struct aga_snddev snddev;

	struct aga_scripteng scripteng;
	void* transform_class;
	void* agan_dict;

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

/*
 * NOTE: The required systems for a startup are scripting and graphics.
 * 		 Otherwise, we have a path to continuing execution with reduced
 * 		 Capabilities.
 */
enum af_err aga_init(struct aga_ctx* ctx, int argc, char** argv);
enum af_err aga_kill(struct aga_ctx* ctx);

void aga_af_chk(const char* loc, const char* proc, enum af_err e);
AGA_NORETURN void aga_errno_chk(const char* loc, const char* proc);

const char* aga_af_errname(enum af_err e);
/* NOTE: Pass null to `proc' to suppress error message printout. */
enum af_err aga_af_errno(const char* loc, const char* proc);
enum af_err aga_af_patherrno(
		const char* loc, const char* proc, const char* path);
void aga_af_soft(const char* loc, const char* proc, enum af_err e);

void aga_boundf(float* f, float min, float max);

struct aga_timestamp {
	/* NOTE: Not supposed to be a public API time format. */
	long sec;
	long usec;
};

enum af_err aga_startstamp(struct aga_timestamp* ts);
enum af_err aga_endstamp(const struct aga_timestamp* ts, af_size_t* us);

#endif
