/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_IO_H
#define AGA_IO_H

#include <agaenv.h>

#include <afeirsa/afeirsa.h>

#if (defined(AGA_HAVE_SYS_MMAN) && defined(AGA_HAVE_SYS_STAT))
# define AGA_HAVE_MAP
#endif

#if defined(AGA_HAVE_SYS_WAIT) && defined(AGA_HAVE_UNISTD)
# define AGA_NIXSPAWN
# define AGA_HAVE_SPAWN
#elif defined(_WIN32)
# define AGA_WINSPAWN
# define AGA_HAVE_SPAWN
#endif

enum af_err aga_open(const char* path, void** fp, af_size_t* size);

#ifdef AGA_HAVE_SPAWN
enum af_err aga_spawn_sync(const char* program, char** argv, const char* wd);
#endif

#endif
