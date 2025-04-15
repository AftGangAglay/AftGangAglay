/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef ASYS_MATH_H
#define ASYS_MATH_H

#include <asys/base.h>

#if defined(ASYS_STDC) || defined(ASYS_FORCE_STD_INCLUDE) /* TODO: Temp!*/
# include <math.h>
#endif

#define ASYS_PI (3.14159265358979323846)
#define ASYS_E (2.71828182845904523536)

/* Returns a normalized random value. */
double asys_random(void);

#endif
