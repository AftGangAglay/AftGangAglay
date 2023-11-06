/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PCX_H
#define AGA_PCX_H

#include <afeirsa/afeirsa.h>

#define AGA_IMG_COMP 4

/* NOTE: All images are unpadded RGBA8 */
struct aga_img {
	af_uint32_t* data;
	af_size_t width, height;
};

enum af_err aga_mkimg(struct aga_img* img, const char* path);
enum af_err aga_killimg(struct aga_img* img);
enum af_err aga_mkteximg(
		struct af_ctx* ctx, struct aga_img* img, struct af_buf* tex,
		af_bool_t filter);

#endif
