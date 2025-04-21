/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/math.h>
#include <asys/system.h>

double asys_math_acos(double x) {
#ifdef ASYS_STDC
	return acos(x);
#else
#endif
}

double asys_math_asin(double x) {
#ifdef ASYS_STDC
	return asin(x);
#else
#endif
}

double asys_math_atan(double x) {
#ifdef ASYS_STDC
	return atan(x);
#else
#endif
}

double asys_math_ceil(double x) {
#ifdef ASYS_STDC
	return ceil(x);
#else
#endif
}

double asys_math_cos(double x) {
#ifdef ASYS_STDC
	return cos(x);
#else
#endif
}

double asys_math_cosh(double x) {
#ifdef ASYS_STDC
	return cosh(x);
#else
#endif
}

double asys_math_exp(double x) {
#ifdef ASYS_STDC
	return exp(x);
#else
#endif
}

double asys_math_fabs(double x) {
#ifdef ASYS_STDC
	return fabs(x);
#else
	return x < 0.0 ? -x : x;
#endif
}

double asys_math_floor(double x) {
#ifdef ASYS_STDC
	return floor(x);
#else
#endif
}

double asys_math_log(double x) {
#ifdef ASYS_STDC
	return log(x);
#else
#endif
}

double asys_math_log10(double x) {
#ifdef ASYS_STDC
	return log10(x);
#else
#endif
}

double asys_math_sin(double x) {
#ifdef ASYS_STDC
	return sin(x);
#else
#endif
}

double asys_math_sinh(double x) {
#ifdef ASYS_STDC
	return sinh(x);
#else
#endif
}

double asys_math_sqrt(double x) {
#ifdef ASYS_STDC
	return sqrt(x);
#else
#endif
}

double asys_math_tan(double x) {
#ifdef ASYS_STDC
	return tan(x);
#else
#endif
}

double asys_math_tanh(double x) {
#ifdef ASYS_STDC
	return tanh(x);
#else
#endif
}

double asys_math_atan2(double x, double y) {
#ifdef ASYS_STDC
	return atan2(x, y);
#else
#endif
}

double asys_math_fmod(double x, double y) {
#ifdef ASYS_STDC
	return fmod(x, y);
#else
#endif
}

double asys_math_pow(double x, double y) {
#ifdef ASYS_STDC
	return pow(x, y);
#else
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
