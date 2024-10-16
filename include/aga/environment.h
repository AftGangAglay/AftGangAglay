/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_ENVIRONMENT_H
#define AGA_ENVIRONMENT_H

#ifdef __has_attribute
# if __has_attribute(noreturn)
#  define AGA_NORETURN __attribute__((noreturn))
# endif
# if __has_attribute(fallthrough)
#  define AGA_FALLTHROUGH __attribute__((fallthrough))
# endif
#endif

#ifndef AGA_NORETURN
# ifdef _MSC_VER
#  define AGA_NORETURN __declspec(noreturn)
# else
#  define AGA_NORETURN
# endif
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

/*
 * TODO: More cohesively detect POXIX.1-1990 based on spec. compliance to
 * 		 Determine our basic IO subsystems etc.
 */
#ifdef __has_include
# if __has_include(<unistd.h>)
#  define AGA_HAVE_UNISTD
# endif
# if __has_include(<getopt.h>)
#  define AGA_HAVE_GETOPT
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
# if __has_include(<dirent.h>)
#  define AGA_HAVE_DIRENT
# endif
#endif

/* Epsilon when comparing floats in transforms. */
#define AGA_TRANSFORM_TOLERANCE (0.1f)
#define AGA_LEN(arr) (sizeof((arr)) / sizeof((arr)[0]))

#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wlong-long"
#endif

typedef unsigned char aga_uchar_t;
typedef unsigned short aga_ushort_t;
typedef unsigned int aga_uint_t;
typedef unsigned long long aga_ulong_t;

typedef signed char aga_schar_t;
typedef signed short aga_sshort_t;
typedef signed int aga_sint_t;
typedef signed long long aga_slong_t;

typedef aga_ulong_t aga_size_t;

typedef aga_uchar_t aga_bool_t;

#define AGA_TRUE (1)
#define AGA_FALSE (0)

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

/*
 * NOTE: This exists for cases where we are forced to use fixed size buffers
 * 		 Due to limitations like the nonexistence of `vsnprintf'.
 * 		 This is NOT an excuse to use this pattern unnecessarily -- play nice
 * 		 With your buffers.
 */
typedef char aga_fixed_buf_t[2048 + 1];

#endif
