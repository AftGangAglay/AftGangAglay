/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/window.h>
#include <aga/gl.h>

#ifdef _WIN32
# include "win32window.h"
#else
# include "xwindow.h"
#endif
