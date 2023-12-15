/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_STD_H
#define AGA_STD_H

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#ifdef AGA_WANT_UNIX
# ifdef AGA_HAVE_UNIX
#  include <unistd.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <getopt.h>
#  include <sys/mman.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/fcntl.h>
#  include <sys/wait.h>
# endif
# ifdef _WINDOWS
#  include <direct.h>
# endif
#endif

#endif
