/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_IO_H
#define AGA_IO_H

#include <agaenv.h>

#include <afeirsa/afeirsa.h>

#if defined(AGA_HAVE_SYS_MMAN)
/*
# define AGA_NIXMAP
# define AGA_HAVE_MAP
 */
#elif defined(_WIN32)
/*
# define AGA_WINMAP
# define AGA_HAVE_MAP
 */
#endif

#if defined(AGA_HAVE_SYS_WAIT) && defined(AGA_HAVE_UNISTD)
# define AGA_NIXSPAWN
# define AGA_HAVE_SPAWN
#elif defined(_WIN32)
# define AGA_WINSPAWN
# define AGA_HAVE_SPAWN
#endif

enum af_err aga_fplen(void* fp, af_size_t* size);

#ifdef AGA_HAVE_MAP
# ifdef AGA_NIXMAP
struct aga_mapfd {
	void* fp;
};
# elif defined(AGA_WINMAP)
struct aga_mapfd {
	void* mapping;
};
# endif

enum af_err aga_mkmapfd(void* fp, struct aga_mapfd* fd);
enum af_err aga_killmapfd(struct aga_mapfd* fd);

/* Make a read-only mapping of a region of a file. */
enum af_err aga_mkfmap(
		struct aga_mapfd* fd, af_size_t off, af_size_t size, void** ptr);
enum af_err aga_killfmap(void* ptr, af_size_t size);
#endif

#ifdef AGA_HAVE_SPAWN
enum af_err aga_spawn_sync(const char* program, char** argv, const char* wd);
#endif

#endif
