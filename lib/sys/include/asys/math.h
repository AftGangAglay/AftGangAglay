/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef ASYS_MATH_H
#define ASYS_MATH_H

#include <asys/base.h>

#define ASYS_PI (3.14159265358979323846)
#define ASYS_E (2.71828182845904523536)

double asys_math_acos(double);
double asys_math_asin(double);
double asys_math_atan(double);
double asys_math_ceil(double);
double asys_math_cos(double);
double asys_math_cosh(double);
double asys_math_exp(double);
double asys_math_fabs(double);
double asys_math_floor(double);
double asys_math_log(double);
double asys_math_log10(double);
double asys_math_sin(double);
double asys_math_sinh(double);
double asys_math_sqrt(double);
double asys_math_tan(double);
double asys_math_tanh(double);

double asys_math_atan2(double, double);
double asys_math_fmod(double, double);
double asys_math_pow(double, double);

/* Returns a normalized random value. */
double asys_random(void);

#endif
