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

/*
 * TODO: This was rather poorly thought out and we should create a more
 * 		 Bespoke benchmarking system -- GL timing with pipeline usage
 * 		 Statistics, script timing.
 */
enum apro_section {
	APRO_PRESWAP, /* All operations before buffer swapping. */

	APRO_POLL, /* Top level window system/input poll. */

	APRO_SCRIPT_UPDATE, /* Script update call. */
	APRO_SCRIPT_INSTCALL_RISING, /* Rising edge for script Python exec. */
	APRO_SCRIPT_INSTCALL_EXEC, /* Script Python exec. */

	APRO_CEVAL_CODE_EVAL_RISING, /* Rising edge for ceval code eval. */
	APRO_CEVAL_CODE_EVAL, /* Ceval code eval. */
	APRO_CEVAL_CODE_EVAL_FALLING, /* Falling edge for ceval code eval. */

	APRO_RES_SWEEP, /* Resource pack sweep. */

	/* Scriptglue calls */
	APRO_SCRIPTGLUE_GETKEY,
	APRO_SCRIPTGLUE_GETMOTION,
	APRO_SCRIPTGLUE_SETCURSOR,
	APRO_SCRIPTGLUE_GETBUTTONS,

	APRO_SCRIPTGLUE_SETCAM,
	APRO_SCRIPTGLUE_TEXT,
	APRO_SCRIPTGLUE_FOGPARAM,
	APRO_SCRIPTGLUE_FOGCOL,
	APRO_SCRIPTGLUE_CLEAR,
	APRO_SCRIPTGLUE_MKTRANS,
	APRO_SCRIPTGLUE_SHADEFLAT,
	APRO_SCRIPTGLUE_GETPIX,
	APRO_SCRIPTGLUE_SETFLAG,
	APRO_SCRIPTGLUE_GETFLAG,

	APRO_SCRIPTGLUE_GETCONF,
	APRO_SCRIPTGLUE_LOG,
	APRO_SCRIPTGLUE_DIE,

	APRO_SCRIPTGLUE_MKOBJ,
	APRO_SCRIPTGLUE_INOBJ,
	APRO_SCRIPTGLUE_PUTOBJ,
	APRO_SCRIPTGLUE_KILLOBJ,
	APRO_SCRIPTGLUE_OBJTRANS,
	APRO_SCRIPTGLUE_OBJCONF,

	APRO_SCRIPTGLUE_BITAND,
	APRO_SCRIPTGLUE_BITSHL,
	APRO_SCRIPTGLUE_RANDNORM,
	APRO_SCRIPTGLUE_BITOR,

	APRO_PUTOBJ_RISING,
	APRO_PUTOBJ_LIGHT,
	APRO_PUTOBJ_CALL,
	APRO_PUTOBJ_FALLING,

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

/* NOTE: This won't work inside recursive call chains atm. */
void apro_stamp_start(enum apro_section);
void apro_stamp_end(enum apro_section);
apro_unit_t apro_stamp_us(enum apro_section);
void apro_clear(void);

const char* apro_section_name(enum apro_section);

#endif
