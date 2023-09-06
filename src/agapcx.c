/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agapcx.h>

static af_size_t aga_img_type_width(enum aga_img_type type) {
	switch(type) {
		case AGA_RGB: return 3;
		case AGA_RGBA: return 4;
	}
	return 0;
}

enum af_err aga_pcx2img(
		struct aga_img* img, const af_uchar_t* pcx, af_size_t size) {

	struct aga_pcx_hdr hdr;
	af_size_t i;

	AF_PARAM_CHK(img);
	AF_PARAM_CHK(pcx);

	AF_VERIFY(size >= sizeof(struct aga_pcx_hdr), AF_ERR_BAD_PARAM);

	af_memcpy(&hdr, pcx, sizeof(struct aga_pcx_hdr));

	AF_VERIFY(hdr.magic == AGA_PCX_MAGIC, AF_ERR_BAD_PARAM);

	img->width = (hdr.max_x - hdr.min_x) + 1;
	img->height = (hdr.max_y - hdr.min_y) + 1;

	img->data = malloc(
		img->width * img->height * aga_img_type_width(img->type));

	{
		extern int printf(const char* s, ...);
#pragma GCC diagnostic ignored "-Wformat-pedantic"
		__builtin_dump_struct(&hdr, &printf);
	}

	/*
	 * If we don't accept bit-packed PCX it makes the logic simpler
	 */
	AF_VERIFY(hdr.bits_per_plane == 8, AF_ERR_BAD_PARAM);

	{
		/*af_size_t width = aga_img_type_width(img->type);*/
		af_size_t pos = 0;
		const af_uchar_t* dat = pcx + sizeof(struct aga_pcx_hdr);
		for(i = 0; i < size - sizeof(struct aga_pcx_hdr); ++i) {
			if((dat[i] >> 6) == 0x3) {
				/* RLE pair */
				af_uint8_t a = dat[i];
				af_uint8_t b = dat[i + 1];
				af_size_t count = (a & 0x3F);
				af_size_t j;

				for(j = 0; j < count; ++j) img->data[pos] = b;

				pos += count;
				++i;
			}
			else {

			}
		}
	}

	return AF_ERR_NONE;
}
