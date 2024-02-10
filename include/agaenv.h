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
# if __has_attribute(unused)
#  define AGA_UNUSED __attribute__((unused))
# endif
# if __has_attribute(noreturn)
#  define AGA_NORETURN __attribute__((noreturn))
# endif
# if __has_attribute(fallthrough)
#  define AGA_FALLTHROUGH __attribute__((fallthrough))
# endif
#endif

#ifndef AGA_DESTRUCTOR
# define AGA_DESTRUCTOR
#endif

#ifndef AGA_USED
# define AGA_USED
#endif

#ifndef AGA_UNUSED
# define AGA_UNUSED
#endif

#ifndef AGA_NORETURN
# define AGA_NORETURN
#endif

#ifndef AGA_FALLTHROUGH
# define AGA_FALLTHROUGH
#endif

#ifdef __has_builtin
# if __has_builtin(__builtin_unreachable)
#  define AGA_UNREACHABLE __builtin_unreachable()
# endif
#endif

#ifndef AGA_UNREACHABLE
# define AGA_UNREACHABLE
#endif

#ifdef __has_include
# if __has_include(<unistd.h>)
#  define AGA_HAVE_UNISTD
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
# if __has_include(<sys/wait.h>)
#  define AGA_HAVE_SYS_WAIT
# endif
# if __has_include(<sys/stat.h>)
#  define AGA_HAVE_SYS_STAT
# endif
# if __has_include(<sys/types.h>)
#  define AGA_HAVE_SYS_TYPES
# endif
# if __has_include(<fcntl.h>)
#  define AGA_HAVE_FCNTL
# endif
# if __has_include(<sys/ioctl.h>)
#  define AGA_HAVE_SYS_IOCTL
# endif
# if __has_include(<sys/soundcard.h>)
#  define AGA_HAVE_SYS_SOUNDCARD
# endif
#endif

#define AGA_LEN(arr) (sizeof((arr)) / sizeof(*(arr)))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"

typedef unsigned char aga_uchar_t;
typedef unsigned short aga_ushort_t;
typedef unsigned int aga_uint_t;
typedef unsigned long long aga_ulong_t;

typedef signed char aga_schar_t;
typedef signed short aga_sshort_t;
typedef signed int aga_sint_t;
typedef signed long long aga_slong_t;

typedef aga_uchar_t aga_uint8_t;
typedef aga_ushort_t aga_uint16_t;
typedef aga_uint_t aga_uint32_t;
typedef aga_ulong_t aga_uint64_t;

typedef aga_schar_t aga_sint8_t;
typedef aga_sshort_t aga_sint16_t;
typedef aga_sint_t aga_sint32_t;
typedef aga_slong_t aga_sint64_t;

typedef aga_ulong_t aga_size_t;

typedef aga_uchar_t aga_bool_t;

#define AF_TRUE (1)
#define AF_FALSE (0)

#pragma GCC diagnostic pop

#endif
