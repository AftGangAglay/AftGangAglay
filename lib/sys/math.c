/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/math.h>
#include <asys/system.h>

/*
 * NOTE: Sun's "FDLIBM" appears to have been created in 1993 -- older SunOS
 * 		 Used some ASM jankery. Our resident glibc also doesn't try to
 * 		 Implement `sin' in the generic stub. We can trivially roll some of
 * 		 These but not others.
 */

double asys_math_acos(double x) {
	return acos(x);
}

double asys_math_asin(double x) {
	return asin(x);
}

double asys_math_atan(double x) {
	return atan(x);
}

double asys_math_ceil(double x) {
	return ceil(x);
}

double asys_math_cos(double x) {
	return cos(x);
}

double asys_math_cosh(double x) {
	return cosh(x);
}

double asys_math_exp(double x) {
	return exp(x);
}

double asys_math_fabs(double x) {
#ifdef ASYS_STDC
	return fabs(x);
#else
	return x < 0.0 ? -x : x;
#endif
}

double asys_math_floor(double x) {
	return floor(x);
}

double asys_math_log(double x) {
	return log(x);
}

double asys_math_log10(double x) {
	return log10(x);
}

double asys_math_sin(double x) {
	return sin(x);
}

double asys_math_sinh(double x) {
	return sinh(x);
}

double asys_math_sqrt(double x) {
	return sqrt(x);
}

double asys_math_tan(double x) {
	return tan(x);
}

double asys_math_tanh(double x) {
	return tanh(x);
}

double asys_math_atan2(double x, double y) {
	return atan2(x, y);
}

double asys_math_fmod(double x, double y) {
	return fmod(x, y);
}

double asys_math_pow(double x, double y) {
	return pow(x, y);
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
