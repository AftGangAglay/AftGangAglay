/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_W32_H
#define AGA_W32_H

#include <agaresult.h>

#define AGA_ICON_RESOURCE (10)
#define AGA_EMBED_RESOURCE (11)

#define AGA_AF_WINCHK(proc) \
	aga_af_chk(__FILE__, proc, aga_af_winerr(__FILE__, proc))

enum af_err aga_af_pathwinerr(
		const char* loc, const char* proc, const char* path);
enum af_err aga_af_winerr(const char* loc, const char* proc);

void aga_setw32log(void);

#endif
