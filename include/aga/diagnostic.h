/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_DIAGNOSTIC_H
#define AGA_DIAGNOSTIC_H

#include <aga/environment.h>
#include <aga/log.h>

/* TODO: Turn off for noverify. */

#define AGA_DEPRECATED_IMPL(msg) \
	do { \
		static aga_bool_t _deprflag = AGA_TRUE; \
		if(_deprflag) { \
			aga_log(__FILE__, "warn: %s", msg); \
			_deprflag = AGA_FALSE; \
		} \
	} while(0)

#define AGA_DEPRECATED(curr, alt) \
	AGA_DEPRECATED_IMPL(#curr " is deprecated in favor of " #alt)

#endif
