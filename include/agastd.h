/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_STD_H
#define AGA_STD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#if defined(AGA_HAVE_UNIX) && defined(AGA_WANT_UNIX)
# include <unistd.h>
# include <unistd.h>
# include <fcntl.h>
# include <poll.h>
#endif

#endif
