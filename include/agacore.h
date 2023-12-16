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
#include <agaenv.h>

#include <afeirsa/afeirsa.h>

/*
 * NOTE: This exists for cases where we are forced to use fixed size buffers
 * 		 Due to limitations like the nonexistence of `vsnprintf'.
 * 		 This is NOT an excuse to use this pattern unnecessarily - play nice
 * 		 With your buffers.
 */
typedef char aga_fixed_buf_t[2048 + 1];

struct aga_ctx {
	struct af_ctx af_ctx;

	struct aga_winenv winenv;
	struct aga_win win;
	af_bool_t die;

	struct aga_keymap keymap;
	af_bool_t* keystates;

	int pointer_dx;
	int pointer_dy;

	af_bool_t audio_enabled;
	struct aga_snddev snddev;

	/* TODO: Move to object definitions instead of as a flag here. */
	af_bool_t tex_filter;
	void* transform_class;
	void* agan_dict;

	struct af_vert vert;

	const char* conf_path;
	struct aga_conf_node conf;
};

enum af_err aga_init(struct aga_ctx* ctx, int argc, char** argv);
enum af_err aga_kill(struct aga_ctx* ctx);

void aga_af_chk(const char* loc, const char* proc, enum af_err e);

const char* aga_af_errname(enum af_err e);
/* NOTE: Pass null to `proc' to suppress error message printout. */
enum af_err aga_af_errno(const char* loc, const char* proc);
enum af_err aga_af_patherrno(
		const char* loc, const char* proc, const char* path);
void aga_af_soft(const char* loc, const char* proc, enum af_err e);

#endif
