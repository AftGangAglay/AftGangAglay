/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_IMG_H
#define AGA_IMG_H

#include <afeirsa/afeirsa.h>

/* NOTE: All images are unpadded RGBA8 */
struct aga_img {
	af_uint32_t width, height;
	void* data;
};

/*
 * NOTE: The AGA raw image format isn't the "real" raw format but rather just
 * 		 Unpacked image data with the width and the magic appended as uint32.
 */
enum af_err aga_mkimg(struct aga_img* img, const char* path);

enum af_err aga_killimg(struct aga_img* img);
enum af_err aga_mkteximg(
		struct af_ctx* ctx, struct aga_img* img, struct af_buf* tex,
		af_bool_t filter);

#endif
