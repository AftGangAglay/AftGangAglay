/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_LOG_H
#define AGA_LOG_H

#include <agacore.h>

struct aga_logctx {
	void** targets;
	af_size_t len;
};

enum aga_logsev {
	AGA_NORM,
	AGA_WARN,
	AGA_ERR
};

/*
 * NOTE: This must be globally initialized before the logger can be used.
 * 		 It is not marked for TLS because threads barely exist yet.
 */
extern struct aga_logctx aga_logctx;

void aga_mklog(const char** targets, af_size_t len);
/*
 * NOTE: Called during fatal signals and `_fini' - you probably don't need to
 * 		 Call this yourself.
 */
void aga_killlog(void);

void aga_loghdr(void* s, const char* loc, enum aga_logsev sev);

/* NOTE: Handles errors internally to avoid nasty nested error states. */
void aga_log(const char* loc, const char* fmt, ...);

#endif
