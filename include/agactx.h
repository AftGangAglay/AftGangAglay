/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_CTX_H
#define AGA_CTX_H

#include <agawin.h>
#include <agasnd.h>
#include <agastartup.h>

#include <afeirsa/afeirsa.h>

struct aga_ctx {
	struct aga_opts* opts;

	struct af_ctx af_ctx;
	struct af_vert vert;

	struct aga_winenv winenv;
	struct aga_win win;
	af_bool_t die;

	struct aga_keymap keymap;
	af_bool_t* keystates;

	int pointer_dx;
	int pointer_dy;

	struct aga_snddev snddev;

	/* TODO: Move to object definitions instead of as a flag here. */
	af_bool_t tex_filter;
	void* transform_class;
	void* agan_dict;
};

enum af_err aga_init(
		struct aga_ctx* ctx, struct aga_opts* opts, int argc, char** argv);
enum af_err aga_kill(struct aga_ctx* ctx);

#endif
