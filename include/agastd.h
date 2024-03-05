/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_STD_H
#define AGA_STD_H

#include <agaenv.h>

#ifdef _WIN32
# ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
# endif
#endif

#ifndef _MSC_VER
# ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 2
# endif
#endif

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4710)
#endif

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
#  include <io.h>
#  include <direct.h>
# endif
#endif

#ifdef _MSC_VER
# pragma warning(pop)
#endif

/* TODO: These probably shouldn't be here. */
FILE* aga_open_r(const char* path);
void aga_close(FILE* fp);

#ifdef _WIN32
# undef _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _MSC_VER
# undef _POSIX_C_SOURCE
#endif

#endif
