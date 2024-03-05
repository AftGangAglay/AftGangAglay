/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef APRO_H
#define APRO_H

/*
 * Profile markers are baked into `apro' to make a simpler API with minimal
 * Overhead. `agaprof' is entirely an internal tool, so it doesn't really
 * Matter that it's designed this way.
 */

enum apro_section {
	APRO_PRESWAP, /* All operations before buffer swapping. */

	APRO_POLL, /* Top level window system/input poll. */

	APRO_SCRIPT_UPDATE, /* Script update call. */
	APRO_SCRIPT_INSTCALL_RISING, /* Script `instcall' pre-Python exec. */
	APRO_SCRIPT_INSTCALL_EXEC, /* Script `instcall' Python exec. */

	APRO_CEVAL_CALL_RISING,
	APRO_CEVAL_CALL_EVAL,

	APRO_RES_SWEEP, /* Resource pack sweep. */

	APRO_MAX
};

#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wlong-long"
#endif
typedef unsigned long long apro_unit_t;
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

struct apro_timestamp {
	apro_unit_t seconds;
	apro_unit_t microseconds;
};

extern struct apro_timestamp aga_global_prof[];

#ifdef NDEBUG
# define apro_stamp_start(section)
# define apro_stamp_end(section)
# define apro_stamp_us(section) (0ull)
# define apro_clear()
#else

/* NOTE: This won't work inside recursive call chains atm. */
void apro_stamp_start(enum apro_section section);
void apro_stamp_end(enum apro_section section);
apro_unit_t apro_stamp_us(enum apro_section section);
void apro_clear(void);
#endif

#endif
