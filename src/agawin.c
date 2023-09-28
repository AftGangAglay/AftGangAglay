/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agalog.h>

#define AGA_WANT_UNIX
#include <agastd.h>
#undef AGA_WANT_UNIX

#include <afeirsa/afgl.h>

#ifdef AF_GLXABI
# include "agaxwin.h"
#elif defined(AF_WGL)
# include "agawwin.h"
#endif
