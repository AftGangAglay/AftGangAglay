/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agastd.h>
#include <agaprof.h>

struct aga_timestamp aga_global_prof[AGA_PROF_MAX * 2] = { 0 };

void aga_prof_stamp_start(enum aga_prof_section section) {
	(void) aga_getstamp(&aga_global_prof[section * 2]);
}

void aga_prof_stamp_end(enum aga_prof_section section) {
	struct aga_timestamp stamp;
	struct aga_timestamp* start = &aga_global_prof[section * 2];
	struct aga_timestamp* new = &aga_global_prof[section * 2 + 1];
	aga_ulong_t ds;
	aga_ulong_t duss;

	(void) aga_getstamp(&stamp);

	ds = stamp.seconds - start->seconds;
	duss = stamp.microseconds - start->microseconds;

	new->seconds += ds;
	new->microseconds += duss;
}

aga_ulong_t aga_prof_stamp_us(enum aga_prof_section section) {
	return aga_stamp_us(&aga_global_prof[section * 2 + 1]);
}

void aga_prof_clear(void) {
	memset(aga_global_prof, 0, sizeof(aga_global_prof));
}
