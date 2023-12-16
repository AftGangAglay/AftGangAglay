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
#endif

#ifdef __has_include
# if __has_include(<unistd.h>)
#  define AGA_HAVE_UNIX
# endif
#endif

#endif
