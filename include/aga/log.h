/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_LOG_H
#define AGA_LOG_H

#include <aga/environment.h>
#include <aga/result.h>

struct aga_log_context {
	void** targets;
	aga_size_t len;
	aga_bool_t have_ansi;
};

enum aga_log_severity {
	AGA_NORM, AGA_WARN, AGA_ERR
};

/*
 * NOTE: This must be globally initialized before the logger can be used.
 * 		 It is not marked for TLS because threads barely exist yet.
 */
extern struct aga_log_context aga_log_context;

void aga_log_new(const char**, aga_size_t);

/*
 * NOTE: Called during fatal signals and `_fini' - you probably don't need to
 * 		 Call this yourself.
 */
void aga_log_delete(void);

void aga_log_header(void*, const char*, enum aga_log_severity);

/* NOTE: Handles errors internally to avoid nasty nested error states. */
void aga_log(const char*, const char*, ...);

#endif
