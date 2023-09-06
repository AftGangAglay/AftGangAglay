/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PCX_H
#define AGA_PCX_H

#include <afeirsa/aftypes.h>
#include <afeirsa/afdefs.h>

#define AGA_IMG_COMP 4

/* NOTE: All images are unpadded RGBA8 */
struct aga_img {
	af_uint32_t* data;
	af_size_t width, height;
};

enum af_err aga_tiff2img(struct aga_img* img, const char* path);

#endif
