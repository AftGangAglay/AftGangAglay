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
#ifndef APRO_DISABLE
# ifdef APRO_HAVE_SYS_TIME
	struct timeval tv;
	if(gettimeofday(&tv, 0) == -1) perror("gettimeofday");

	ts->seconds = tv.tv_sec;
	ts->microseconds = tv.tv_usec;
# else
	(void) ts;
# endif
#else
	(void) ts;
#endif
}

static apro_unit_t aga_stamp_us(struct apro_timestamp* ts) {
#ifndef APRO_DISABLE
	return (1000000 * ts->seconds) + ts->microseconds;
#else
	(void) ts;
	return 0;
#endif
}

void apro_stamp_start(enum apro_section section) {
#ifndef APRO_DISABLE
	(void) aga_getstamp(&aga_global_prof[section * 2]);
#else
	(void) section;
#endif
}

void apro_stamp_end(enum apro_section section) {
#ifndef APRO_DISABLE
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
#else
	(void) section;
#endif
}

apro_unit_t apro_stamp_us(enum apro_section section) {
#ifndef APRO_DISABLE
	return aga_stamp_us(&aga_global_prof[section * 2 + 1]);
#else
	return 0;
#endif
}

void apro_clear(void) {
#ifndef APRO_DISABLE
	memset(aga_global_prof, 0, sizeof(aga_global_prof));
#endif
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
		case APRO_CEVAL_CODE_EVAL_RISING: return "CEVAL_CODE_EVAL_RISING";
		case APRO_CEVAL_CODE_EVAL: return "CEVAL_CODE_EVAL";
		case APRO_CEVAL_CODE_EVAL_FALLING: return "CEVAL_CODE_EVAL_FALLING";
		case APRO_RES_SWEEP: return "RES_SWEEP";
		case APRO_SCRIPTGLUE_GETKEY: return "SCRIPTGLUE_GETKEY";
		case APRO_SCRIPTGLUE_GETMOTION: return "SCRIPTGLUE_GETMOTION";
		case APRO_SCRIPTGLUE_SETCURSOR: return "SCRIPTGLUE_SETCURSOR";
		case APRO_SCRIPTGLUE_SETCAM: return "SCRIPTGLUE_SETCAM";
		case APRO_SCRIPTGLUE_TEXT: return "SCRIPTGLUE_TEXT";
		case APRO_SCRIPTGLUE_FOGPARAM: return "SCRIPTGLUE_FOGPARAM";
		case APRO_SCRIPTGLUE_FOGCOL: return "SCRIPTGLUE_FOGCOL";
		case APRO_SCRIPTGLUE_CLEAR: return "SCRIPTGLUE_CLEAR";
		case APRO_SCRIPTGLUE_MKTRANS: return "SCRIPTGLUE_MKTRANS";
		case APRO_SCRIPTGLUE_GETCONF: return "SCRIPTGLUE_GETCONF";
		case APRO_SCRIPTGLUE_LOG: return "SCRIPTGLUE_LOG";
		case APRO_SCRIPTGLUE_DIE: return "SCRIPTGLUE_DIE";
		case APRO_SCRIPTGLUE_MKOBJ: return "SCRIPTGLUE_MKOBJ";
		case APRO_SCRIPTGLUE_INOBJ: return "SCRIPTGLUE_INOBJ";
		case APRO_SCRIPTGLUE_PUTOBJ: return "SCRIPTGLUE_PUTOBJ";
		case APRO_SCRIPTGLUE_KILLOBJ: return "SCRIPTGLUE_KILLOBJ";
		case APRO_SCRIPTGLUE_OBJTRANS: return "SCRIPTGLUE_OBJTRANS";
		case APRO_SCRIPTGLUE_OBJCONF: return "SCRIPTGLUE_OBJCONF";
		case APRO_SCRIPTGLUE_BITAND: return "SCRIPTGLUE_BITAND";
		case APRO_SCRIPTGLUE_BITSHL: return "SCRIPTGLUE_BITSHL";
		case APRO_SCRIPTGLUE_RANDNORM: return "SCRIPTGLUE_RANDNORM";
		case APRO_PUTOBJ_RISING: return "PUTOBJ_RISING";
		case APRO_PUTOBJ_LIGHT: return "PUTOBJ_LIGHT";
		case APRO_PUTOBJ_CALL: return "PUTOBJ_CALL";
		case APRO_PUTOBJ_FALLING: return "PUTOBJ_FALLING";
		case APRO_MAX: return "MAX";
	}
}
