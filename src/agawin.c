/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agawin.h>
#include <agaerr.h>
#include <agastartup.h>
#include <agalog.h>
#define AGA_WANT_UNIX
#include <agastd.h>

#include <afeirsa/afgl.h>

#ifdef AF_GLXABI
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include "agaxwin.h"
#elif defined(AF_WGL)
# include "agawwin.h"
#endif
