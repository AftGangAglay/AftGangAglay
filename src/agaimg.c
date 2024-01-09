/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaimg.h>
#include <agaio.h>
#include <agastd.h>

#define AGA_IMG_COMP (4)
#define AGA_IMG_MAGIC (0xA6A)
#define AGA_IMG_FOOTER_SIZE (2 * sizeof(af_uint32_t))

enum af_err aga_mkimg(struct aga_img* img, const char* path) {
	static const af_uint32_t magic = AGA_IMG_MAGIC;

	af_size_t size;
	af_uchar_t* cdata;

	AF_PARAM_CHK(img);
	AF_PARAM_CHK(path);

	AF_CHK(aga_mklargefile(path, &img->data, &size));
	cdata = img->data;

	AF_VERIFY(size >= AGA_IMG_FOOTER_SIZE, AF_ERR_BAD_PARAM);

#ifndef AF_VERIFY
	if(!!memcmp(cdata + size - sizeof(magic), &magic, sizeof(magic))) {
		return AF_ERR_BAD_PARAM;
	}
#else
	(void) magic;
#endif

	af_memcpy(
		&img->width, cdata + size - AGA_IMG_FOOTER_SIZE, AGA_IMG_FOOTER_SIZE);

	/* Zero-size images are weird but shouldn't be a crash. */
	if(img->width) {
		af_size_t pixels = img->width * AGA_IMG_COMP;
		img->height = (size - AGA_IMG_FOOTER_SIZE) / pixels;
	}
	else img->height = 0;

	return AF_ERR_NONE;
}

enum af_err aga_killimg(struct aga_img* img) {
	af_size_t size;

	AF_PARAM_CHK(img);

	size = (img->width * img->height) + AGA_IMG_FOOTER_SIZE;
	return aga_killlargefile(img->data, size);
}

enum af_err aga_mkteximg(
		struct af_ctx* ctx, struct aga_img* img, struct af_buf* tex,
		af_bool_t filter) {

	af_size_t img_size;

	AF_PARAM_CHK(img);
	AF_PARAM_CHK(tex);

	img_size = img->width * img->height * AGA_IMG_COMP;

	AF_CHK(af_mkbuf(ctx, tex, AF_BUF_TEX));

	tex->tex_width = img->width;
	tex->tex_filter = filter;

	AF_CHK(af_upload(ctx, tex, img->data, img_size));

	return AF_ERR_NONE;
}
