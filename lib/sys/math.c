/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/math.h>
#define ASYS_FORCE_STD_INCLUDE
#include <asys/system.h>

double asys_fabs(double x) {
	return x < 0.0 ? -x : x;
}

double asys_random(void) {
	/* TODO: Non-stdc impl. */
	return (double) rand() / (double) RAND_MAX;
}
