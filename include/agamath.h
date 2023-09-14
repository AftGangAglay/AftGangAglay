/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_MATH_H
#define AGA_MATH_H

union aga_vec3 {
	struct aga_vec3_decomp {
		float x, y, z;
	} decomp;
	float comp[3];
};

#define AGA_MIN(x, y) ((x) < (y) ? (x) : (y))
#define AGA_MAX(x, y) ((x) > (y) ? (x) : (y))

#define AGA_PI (3.14159265358979323846)
#define AGA_RADS (AGA_PI / 180.0)

#endif
