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

/* TODO: Re-enable icon. */
/* #define AGA_ICON_RESOURCE (10) */

enum aga_result aga_win32_error(const char*, const char*);
enum aga_result aga_win32_error_path(const char*, const char*, const char*);

void aga_setw32log(void);

#endif
