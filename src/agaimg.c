/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agaimg.h>

/* Hiding larger libs like libtiff is desireable */
#include <tiff.h>
#include <tiffio.h>

enum af_err aga_mkimg(struct aga_img* img, const char* path) {
	TIFF* t;
	int result;
	af_uint32_t width, height;

	AF_PARAM_CHK(img);
	AF_PARAM_CHK(path);

	if(!(t = TIFFOpen(path, "r"))) return AF_ERR_UNKNOWN;

	/* We want to treat TIFFs as single images */
	AF_VERIFY(TIFFLastDirectory(t), AF_ERR_BAD_PARAM);

	result = TIFFGetField(t, TIFFTAG_IMAGEWIDTH, &width);
	AF_VERIFY(result == 1, AF_ERR_UNKNOWN);
	result = TIFFGetField(t, TIFFTAG_IMAGELENGTH, &height);
	AF_VERIFY(result == 1, AF_ERR_UNKNOWN);

	img->width = width;
	img->height = height;

	img->data = malloc(width * height * AGA_IMG_COMP);
	AF_VERIFY(img->data, AF_ERR_MEM);

	result = TIFFReadRGBAImage(t, width, height, img->data, 0);
	AF_VERIFY(result == 1, AF_ERR_UNKNOWN);

	TIFFClose(t);

	return AF_ERR_NONE;
}

enum af_err aga_killimg(struct aga_img* img) {
	AF_PARAM_CHK(img);

	free(img->data);

	return AF_ERR_NONE;
}

enum af_err aga_mkteximg(
		struct af_ctx* ctx, struct aga_img* img, struct af_buf* tex) {

	af_size_t img_size = img->width * img->height * AGA_IMG_COMP;

	AF_PARAM_CHK(img);
	AF_PARAM_CHK(tex);

	AF_CHK(af_mkbuf(ctx, tex, AF_BUF_TEX));

	tex->tex_width = img->width;

	AF_CHK(af_upload(ctx, tex, img->data, img_size));

	return AF_ERR_NONE;
}
