/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_DIAGNOSTIC_H
#define AGA_DIAGNOSTIC_H

#include <asys/base.h>
#include <asys/log.h>

#ifdef AGA_NOVERIFY
# define AGA_DEPRECATED_IMPL(msg) (void) msg
# define AGA_DEPRECATED(curr, alt) \
	do { \
		(void) curr; \
		(void) alt; \
	} while(0)
#else
# define AGA_DEPRECATED_IMPL(msg) \
	do { \
		static asys_bool_t _deprflag = ASYS_TRUE; \
		if(_deprflag) { \
			asys_log(__FILE__, "warn: %s", msg); \
			_deprflag = ASYS_FALSE; \
		} \
	} while(0)

# define AGA_DEPRECATED(curr, alt) \
	AGA_DEPRECATED_IMPL(curr " is deprecated in favor of " alt)
#endif

#endif
