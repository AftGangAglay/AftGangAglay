/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agawin.h>
#include <agagl.h>

#ifdef AGA_GLX
# include "agaxwin.h"
#elif defined(AGA_WGL)
# include "agawwin.h"
#endif
