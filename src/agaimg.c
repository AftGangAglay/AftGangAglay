/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaimg.h>
#include <agaio.h>
#include <agastd.h>
#include <agapack.h>

#define AGA_IMG_COMP (4)

enum af_err aga_mkimg(struct aga_img* img, struct aga_res* res) {
	af_uchar_t* cdata;

	AF_PARAM_CHK(img);
	AF_PARAM_CHK(res);

	cdata = res->data;
	img->res = res;

	AF_VERIFY(res->size >= sizeof(af_uint32_t), AF_ERR_BAD_PARAM);

	af_memcpy(
		&img->width, cdata + res->size - sizeof(img->width),
		sizeof(img->width));

	/* Zero-size images are weird but shouldn't be a crash. */
	if(img->width) {
		af_size_t pixels = img->width * AGA_IMG_COMP;
		img->height = (res->size - sizeof(af_uint32_t)) / pixels;
	}
	else img->height = 0;

	AF_CHK(aga_acquireres(res));

	return AF_ERR_NONE;
}

enum af_err aga_killimg(struct aga_img* img) {
	AF_CHK(aga_releaseres(img->res));

	AF_PARAM_CHK(img);

	return AF_ERR_NONE;
}
