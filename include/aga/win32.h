/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_WIN32_H
#define AGA_WIN32_H

#include <aga/result.h>

#ifdef _WIN32
# ifdef AGA_WANT_WINDOWS_H
#  ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4668) /* Symbol not defined as macro. */
#  endif

#  include <windows.h>
#  include <windowsx.h>
#  include <hidusage.h>
/* TODO: This might be Watcom/Borland only. */
#  include <dos.h>

#  ifdef _MSC_VER
#   pragma warning(pop)
#  endif
# endif
#endif

/* TODO: Re-enable icon. */
/* #define AGA_ICON_RESOURCE (10) */

void aga_win32_log_set(void);

#endif
