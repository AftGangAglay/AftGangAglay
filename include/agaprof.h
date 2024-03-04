/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PROF_H
#define AGA_PROF_H

#include <agaenv.h>
#include <agautil.h>

/*
 * Profile markers are baked into `agaprof' to make a simpler API with minimal
 * Overhead. `agaprof' is entirely an internal tool, so it doesn't really
 * Matter that it's designed this way.
 */

enum aga_prof_section {
	AGA_PROF_PRESWAP, /* All operations before buffer swapping. */

	AGA_PROF_POLL, /* Top level window system/input poll. */

	AGA_PROF_SCRIPT_UPDATE, /* Script update call. */
	AGA_PROF_SCRIPT_INSTCALL_RISING, /* Script `instcall' pre-Python exec. */
	AGA_PROF_SCRIPT_INSTCALL_EXEC, /* Script `instcall' Python exec. */

	AGA_PROF_RES_SWEEP, /* Resource pack sweep. */

	AGA_PROF_MAX
};

extern struct aga_timestamp aga_global_prof[];

#ifdef NDEBUG
# define aga_prof_stamp_start(section)
# define aga_prof_stamp_end(section)
# define aga_prof_stamp_us(section) (0ull)
# define aga_prof_clear()
#else
/* NOTE: This won't work inside recursive call chains atm. */
void aga_prof_stamp_start(enum aga_prof_section section);
void aga_prof_stamp_end(enum aga_prof_section section);
aga_ulong_t aga_prof_stamp_us(enum aga_prof_section section);
void aga_prof_clear(void);
#endif

#endif
