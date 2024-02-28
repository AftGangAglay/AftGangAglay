/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_STD_H
#define AGA_STD_H

#include <agaenv.h>

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#ifdef AGA_WANT_UNIX
# undef AGA_WANT_UNIX
# ifdef AGA_HAVE_UNISTD
#  include <unistd.h>
# endif
# ifdef AGA_HAVE_GETOPT
#  include <getopt.h>
# endif
# ifdef AGA_HAVE_POLL
#  include <poll.h>
# endif
# ifdef AGA_HAVE_SYS_MMAN
#  include <sys/mman.h>
# endif
# ifdef AGA_HAVE_SYS_STAT
#  include <sys/stat.h>
# endif
# ifdef AGA_HAVE_SYS_TYPES
#  include <sys/types.h>
# endif
# ifdef AGA_HAVE_FCNTL
#  include <fcntl.h>
# endif
# ifdef AGA_HAVE_SYS_WAIT
#  include <sys/wait.h>
# endif
# ifdef AGA_HAVE_SYS_IOCTL
#  include <sys/ioctl.h>
# endif
# ifdef AGA_HAVE_SYS_SOUNDCARD
#  include <sys/soundcard.h>
# endif
# ifdef _WIN32
#  include <direct.h>
# endif
#endif

FILE* aga_open_r(const char* path);

void aga_close(FILE* fp);

#endif
