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

/*
 * NOTE: This must be globally initialized before the logger can be used.
 * 		 It is not marked for TLS because threads barely exist yet.
 */
extern struct aga_logctx aga_logctx;

enum af_err aga_mklog(const char** targets, af_size_t len);
enum af_err aga_killlog(void);

/* NOTE: Handles errors internally to avoid nasty nested error states. */
void aga_log(const char* loc, const char* fmt, ...);

#endif
