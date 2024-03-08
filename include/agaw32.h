/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_W32_H
#define AGA_W32_H

#include <agaresult.h>

#ifdef _WIN32
# ifdef AGA_WANT_WINDOWS_H
#  ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4668) /* Symbol not defined as macro. */
#  endif
#  include <windows.h>
#  include <windowsx.h>
#  include <hidusage.h>
#  ifdef _MSC_VER
#   pragma warning(pop)
#  endif
# endif
#endif

#define AGA_ICON_RESOURCE (10)

#define AGA_AF_WINCHK(proc) \
    aga_chk(__FILE__, proc, aga_winerr(__FILE__, proc))

enum aga_result aga_pathwinerr(
		const char* loc, const char* proc, const char* path);

enum aga_result aga_winerr(const char* loc, const char* proc);

void aga_setw32log(void);

#endif
