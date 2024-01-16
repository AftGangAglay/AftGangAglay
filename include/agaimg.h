/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_IMG_H
#define AGA_IMG_H

#include <afeirsa/afeirsa.h>

struct aga_res;

/* NOTE: All images are unpadded RGBA8. */
/*
 * TODO: We can remove this idea of an "img" once we have the new resource
 * 		 System up and running and just have `mkteximg'.
 */
struct aga_img {
	af_uint32_t width, height;
	struct aga_res* res;
};

enum af_err aga_mkimg(struct aga_img* img, struct aga_res* res);
enum af_err aga_killimg(struct aga_img* img);

#endif
