/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 *
 * Pre-generated `port.h' to circumvent automake.
 */

#ifndef _PORT_
#define _PORT_ 1

#include <sys/types.h>

#define HOST_FILLORDER FILLORDER_MSB2LSB
#define HOST_BIGENDIAN  1
#define HAVE_MMAP 1

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

typedef double dblparam_t;

#ifdef __STRICT_ANSI__
# define INLINE  __inline__
#else
# define INLINE  inline
#endif

#define GLOBALDATA(TYPE, NAME) extern TYPE NAME

#endif
