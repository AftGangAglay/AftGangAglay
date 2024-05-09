/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_DIAG_H
#define AGA_DIAG_H

#include <agaenv.h>
#include <agalog.h>

/* TODO: Turn off for noverify. */

#define AGA_DEPRSTAMP(msg) \
	do { \
		static aga_bool_t _deprflag = AGA_TRUE; \
		if(_deprflag) { \
			aga_log(__FILE__, "warn: %s", msg); \
			_deprflag = AGA_FALSE; \
		} \
	} while(0)

#define AGA_DEPRCALL(curr, alt) \
	AGA_DEPRSTAMP(#curr " is deprecated in favor of " #alt)

#endif
