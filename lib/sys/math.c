/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/math.h>
#include <asys/system.h>

double asys_fabs(double x) {
#ifdef ASYS_STDC
	return fabs(x);
#else
	return x < 0.0 ? -x : x;
#endif
}

double asys_fmod(double x, double y) {
#ifdef ASYS_STDC
	return fmod(x, y);
#else
	return glibc_fmod(x, y);
#endif
}

#ifndef ASYS_STDC
# include <glibc/stdlib/__random.c>
#endif

double asys_random(void) {
#ifdef ASYS_STDC
	return (double) rand() / (double) RAND_MAX;
#else
	return (double) glibc___random() / (double) ASYS_NATIVE_LONG_MAX;
#endif
}
