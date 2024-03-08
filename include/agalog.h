/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_LOG_H
#define AGA_LOG_H

#include <agaresult.h>

/*
 * NOTE: This exists for cases where we are forced to use fixed size buffers
 * 		 Due to limitations like the nonexistence of `vsnprintf'.
 * 		 This is NOT an excuse to use this pattern unnecessarily - play nice
 * 		 With your buffers.
 */
typedef char aga_fixed_buf_t[2048 + 1];

struct aga_logctx {
	void** targets;
	aga_size_t len;
	aga_bool_t have_ansi;
};

enum aga_logsev {
	AGA_NORM, AGA_WARN, AGA_ERR
};

/*
 * NOTE: This must be globally initialized before the logger can be used.
 * 		 It is not marked for TLS because threads barely exist yet.
 */
extern struct aga_logctx aga_logctx;

void aga_mklog(const char**, aga_size_t);

/*
 * NOTE: Called during fatal signals and `_fini' - you probably don't need to
 * 		 Call this yourself.
 */
void aga_killlog(void);

void aga_loghdr(void*, const char*, enum aga_logsev);

/* NOTE: Handles errors internally to avoid nasty nested error states. */
void aga_log(const char*, const char*, ...);

#endif
