/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_CTX_H
#define AGA_CTX_H

#include <agasnd.h>
#include <agastartup.h>

#include <afeirsa/afeirsa.h>

struct aga_keymap;
struct aga_pointer;

struct aga_ctx {
	struct aga_opts* opts;

	struct af_ctx af_ctx;
	struct af_vert vert;

	af_bool_t die;

	struct aga_snddev snddev;

	struct aga_keymap* keymap;
	struct aga_pointer* pointer;

	void* transform_class;
	void* agan_dict;
};

enum af_err aga_init(struct aga_ctx* ctx, struct aga_opts* opts);
enum af_err aga_kill(struct aga_ctx* ctx);

#endif
