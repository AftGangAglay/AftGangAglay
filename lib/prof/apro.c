/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include "apro.h"

#include <string.h>
#include <stdio.h>

#ifdef __has_include
# if __has_include(<sys/time.h>)
#  define APRO_HAVE_SYS_TIME
#  include <sys/time.h>
# endif
#endif

struct apro_timestamp aga_global_prof[APRO_MAX * 2] = { 0 };

/* NOTE: `gettimeofday' was only standardised in POSIX.1-2001. */
static void aga_getstamp(struct apro_timestamp* ts) {
#ifdef APRO_HAVE_SYS_TIME
	struct timeval tv;
	if(gettimeofday(&tv, 0) == -1) perror("gettimeofday");

	ts->seconds = tv.tv_sec;
	ts->microseconds = tv.tv_usec;
#else
	(void) ts;
#endif
}

static apro_unit_t aga_stamp_us(struct apro_timestamp* ts) {
	return (1000000 * ts->seconds) + ts->microseconds;
}

void apro_stamp_start(enum apro_section section) {
	(void) aga_getstamp(&aga_global_prof[section * 2]);
}

void apro_stamp_end(enum apro_section section) {
	struct apro_timestamp stamp;
	struct apro_timestamp* start = &aga_global_prof[section * 2];
	struct apro_timestamp* new = &aga_global_prof[section * 2 + 1];
	apro_unit_t ds;
	apro_unit_t duss;

	(void) aga_getstamp(&stamp);

	ds = stamp.seconds - start->seconds;
	duss = stamp.microseconds - start->microseconds;

	new->seconds += ds;
	new->microseconds += duss;
}

apro_unit_t apro_stamp_us(enum apro_section section) {
	return aga_stamp_us(&aga_global_prof[section * 2 + 1]);
}

void apro_clear(void) {
	memset(aga_global_prof, 0, sizeof(aga_global_prof));
}

const char* apro_section_name(enum apro_section section) {
	switch(section) {
		default: return "";
		case APRO_PRESWAP: return "PRESWAP";
		case APRO_POLL: return "POLL";
		case APRO_SCRIPT_UPDATE: return "SCRIPT_UPDATE";
		case APRO_SCRIPT_INSTCALL_RISING: return "SCRIPT_INSTCALL_RISING";
		case APRO_SCRIPT_INSTCALL_EXEC: return "SCRIPT_INSTCALL_EXEC";
		case APRO_CEVAL_CALL_RISING: return "CEVAL_CALL_RISING";
		case APRO_CEVAL_CALL_EVAL: return "CEVAL_CALL_EVAL";
		case APRO_RES_SWEEP: return "RES_SWEEP";
		case APRO_MAX: return "MAX";
	}
}
