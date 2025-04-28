/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef ASYS_BASE_H
#define ASYS_BASE_H

/* C environment. */

#ifdef _MSC_VER
# define ASYS_VISUALC
#endif

#ifdef __GNUC__
# define ASYS_GNUC
#endif

#ifdef __STDC__
# define ASYS_STDC
#endif

/* Target system. */

#ifdef _WIN32
# undef ASYS_STDC
# define ASYS_WIN32
# ifdef _WIN64
#  define ASYS_WIN64
#  define ASYS_LP32
# endif
#endif

/* TODO: This assumes all *nix-y systems define these -- is this true? */
#if defined(unix) || defined(__unix) || defined(__unix__)
# define ASYS_UNIX
#endif

/* Utility macros. */

#ifdef ASYS_GNUC
# define ASYS_EXTENSION __extension__
#endif

#ifndef ASYS_EXTENSION
# define ASYS_EXTENSION
#endif

/* TODO: Check attributes from old gcc release. */
#ifdef __has_attribute
# if __has_attribute(fallthrough)
#  define ASYS_FALLTHROUGH __attribute__((fallthrough))
# endif
#endif

#ifndef ASYS_FALLTHROUGH
# define ASYS_FALLTHROUGH
#endif

#define ASYS_LENGTH(arr) (sizeof((arr)) / sizeof((arr)[0]))

/* Native long defines/typedefs for lp32 vs. lp64 */

#ifdef ASYS_LP32
ASYS_EXTENSION typedef long long asys_native_long_t;
ASYS_EXTENSION typedef unsigned long long asys_native_ulong_t;
# ifdef ASYS_WIN32
/*
 * NOTE: `wsprintf' only supports hex for target native long formatting and
 * 		 Does not support signed `long long' formatting at all.
 */
#  define ASYS_NATIVE_LONG_FORMAT "0x%IX"
#  define ASYS_NATIVE_ULONG_FORMAT "0x%IX"
# else
#  define ASYS_NATIVE_LONG_FORMAT "%lld"
#  define ASYS_NATIVE_ULONG_FORMAT "%llu"
# endif
# define ASYS_MAKE_NATIVE_LONG(v) (ASYS_EXTENSION v##LL)
# define ASYS_MAKE_NATIVE_ULONG(v) (ASYS_EXTENSION v##ULL)
#else /* lp64 */
typedef long asys_native_long_t;
typedef unsigned long asys_native_ulong_t;
# define ASYS_NATIVE_LONG_FORMAT "%ld"
# define ASYS_NATIVE_ULONG_FORMAT "%lu"
# define ASYS_MAKE_NATIVE_LONG(v) (v##L)
# define ASYS_MAKE_NATIVE_ULONG(v) (v##UL)
#endif

/* Base typedefs. */

typedef unsigned char asys_uchar_t;
typedef unsigned int asys_uint_t;

typedef asys_native_long_t asys_isize_t;
typedef asys_native_ulong_t asys_size_t;
typedef asys_native_long_t asys_offset_t;
/* TODO: Find a way to switch this to be era-accurate (time32 vs. time64). */
typedef asys_native_long_t asys_time_t;

#define ASYS_NATIVE_ULONG_MAX ((asys_native_ulong_t) -1)
#define ASYS_NATIVE_LONG_MAX ((asys_native_long_t) (ASYS_NATIVE_ULONG_MAX / 2))

#define ASYS_UINT_MAX ((unsigned) -1)
#define ASYS_INT_MAX ((int) (ASYS_UINT_MAX / 2))

typedef enum asys_bool { ASYS_TRUE = 1, ASYS_FALSE = 0 } asys_bool_t;

/* NOTE: `wsprintf''s maximum buffer size is `1024'. */
# define ASYS_FIXED_BUFFER_SIZE (1024 + 1)

/*
 * NOTE: This exists for cases where we are forced to use fixed size buffers
 * 		 Due to limitations like the nonexistence of `vsnprintf'.
 * 		 This is NOT an excuse to use this pattern unnecessarily -- play nice
 * 		 With your buffers.
 */
typedef char asys_fixed_buffer_t[ASYS_FIXED_BUFFER_SIZE];

#endif
