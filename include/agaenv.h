/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_ENV_H
#define AGA_ENV_H

#ifdef __has_attribute
# if __has_attribute(destructor)
#  define AGA_DESTRUCTOR __attribute__((destructor))
# endif
# if __has_attribute(used)
#  define AGA_USED __attribute__((used))
# endif
# if __has_attribute(noreturn)
#  define AGA_NORETURN __attribute__((noreturn))
# endif
#endif

#ifdef __has_builtin
# if __has_builtin(__builtin_unreachable)
#  define AGA_UNREACHABLE __builtin_unreachable()
# endif
#endif

#ifndef AGA_DESTRUCTOR
# define AGA_DESTRUCTOR
#endif

#ifndef AGA_USED
# define AGA_USED
#endif

#ifndef AGA_NORETURN
# define AGA_NORETURN
#endif

#ifndef AGA_UNREACHABLE
# define AGA_UNREACHABLE
#endif

#ifdef __has_include
# if __has_include(<unistd.h>)
#  define AGA_HAVE_UNISTD
# endif
# if __has_include(<fcntl.h>)
#  define AGA_HAVE_FCNTL
# endif
# if __has_include(<getopt.h>)
#  define AGA_HAVE_GETOPT
# endif
# if __has_include(<poll.h>)
#  define AGA_HAVE_POLL
# endif
# if __has_include(<sys/mman.h>)
#  define AGA_HAVE_SYS_MMAN
# endif
# if __has_include(<sys/stat.h>)
#  define AGA_HAVE_SYS_STAT
# endif
# if __has_include(<sys/wait.h>)
#  define AGA_HAVE_SYS_WAIT
# endif
# if __has_include(<sys/ioctl.h>)
#  define AGA_HAVE_SYS_IOCTL
# endif
# if __has_include(<sys/soundcard.h>)
#  define AGA_HAVE_SYS_SOUNDCARD
# endif
#endif

#endif
