/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PCX_H
#define AGA_PCX_H

#include <afeirsa/aftypes.h>
#include <afeirsa/afdefs.h>

#define AGA_PCX_MAGIC (0x0A)

#ifdef AF_HAVE_GNU
# if __has_attribute(packed)
#  define AGA_PACKED __attribute__((packed))
# endif
#endif

struct aga_ega_palette {
	af_uchar_t data[48];
} AGA_PACKED;

enum aga_palette_mode {
	AGA_PAL_GRAY,
	AGA_PAL_COL
} AGA_PACKED;

struct aga_pcx_hdr {
	af_uint8_t magic;
	af_uint8_t ver;

	af_bool_t rle;
	af_uint8_t bits_per_plane;

	af_uint16_t min_x;
	af_uint16_t min_y;
	af_uint16_t max_x;
	af_uint16_t max_y;

	af_uint16_t width;
	af_uint16_t height;

	struct aga_ega_palette palette;

	af_uchar_t reserved0;

	af_uchar_t color_planes;

	af_uint16_t bytes_per_line;
	enum aga_palette_mode palette_mode;

	af_uint16_t screen_width;
	af_uint16_t screen_height;

	af_uchar_t reserved1[54];
} AGA_PACKED;

enum aga_img_type {
	AGA_RGB = 3,
	AGA_RGBA = 4
};

struct aga_img {
	enum aga_img_type type;
	af_uchar_t* data;
	af_size_t width, height;
};

enum af_err aga_pcx2img(
		struct aga_img* img, const af_uchar_t* pcx, af_size_t size);

#endif
