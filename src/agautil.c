/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agautil.h>
#include <agaerr.h>
#define AGA_WANT_UNIX
#include <agastd.h>

/* NOTE: `gettimeofday' was only standardised in POSIX.1-2001. */
enum aga_result aga_getstamp(struct aga_timestamp* ts) {
#ifdef AGA_HAVE_SYS_TIME
	struct timeval tv;
	if(gettimeofday(&tv, 0) == -1) return aga_errno(__FILE__, "gettimeofday");

	ts->seconds = tv.tv_sec;
	ts->microseconds = tv.tv_usec;

	return AGA_RESULT_OK;
#else
	(void) ts;

	return AGA_RESULT_ERROR;
#endif
}

aga_ulong_t aga_stamp_us(struct aga_timestamp* ts) {
	return (1000000 * ts->seconds) + ts->microseconds;
}

aga_bool_t aga_streql(const char* a, const char* b) {
	return !strcmp(a, b);
}
