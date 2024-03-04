/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_UTIL_H
#define AGA_UTIL_H

#include <agaenv.h>
#include <agaresult.h>

struct aga_timestamp {
	aga_ulong_t seconds;
	aga_ulong_t microseconds;
};

enum aga_result aga_getstamp(struct aga_timestamp* ts);
aga_ulong_t aga_stamp_us(struct aga_timestamp* ts);

aga_bool_t aga_streql(const char* a, const char* b);

#endif
