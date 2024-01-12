/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PACK_H
#define AGA_PACK_H

#include <afeirsa/afeirsa.h>

#define AGA_MAGIC ((af_uint32_t) 0xA6A)
static const af_uint32_t aga_magic_global = AGA_MAGIC;

#ifdef AF_VERIFY
# define AGA_MAGIC_SET(ptr, len) \
	(!!memcmp((char*) ptr + len - sizeof(aga_magic_global), \
		&aga_magic_global, sizeof(aga_magic_global)))
#else
# define AGA_MAGIC_SET(ptr, len) (AF_TRUE)
#endif

#endif
