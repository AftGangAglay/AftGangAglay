/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_W32_H
#define AGA_W32_H

#include <agaresult.h>

#define AGA_ICON_RESOURCE (10)

#define AGA_AF_WINCHK(proc) \
    aga_chk(__FILE__, proc, aga_winerr(__FILE__, proc))

enum aga_result aga_pathwinerr(
		const char* loc, const char* proc, const char* path);

enum aga_result aga_winerr(const char* loc, const char* proc);

void aga_setw32log(void);

#endif
