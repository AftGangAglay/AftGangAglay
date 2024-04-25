/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_IO_H
#define AGA_IO_H

#include <agaenv.h>

#include <agaresult.h>

#if defined(AGA_HAVE_SYS_MMAN)
/* TODO: *nix-y file mapping.
# define AGA_NIXMAP
# define AGA_HAVE_MAP
 */
#elif defined(_WIN32)
/* TODO: Windows file mapping.
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

enum aga_result aga_fplen(void*, aga_size_t*);

enum aga_result aga_fread(void*, aga_size_t, void*);

enum aga_result aga_fputn(int, aga_size_t, void*);

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

enum aga_result aga_mkmapfd(void* fp, struct aga_mapfd* fd);
enum aga_result aga_killmapfd(struct aga_mapfd* fd);

/* Make a read-only mapping of a region of a file. */
enum aga_result aga_mkfmap(
		struct aga_mapfd* fd, aga_size_t off, aga_size_t size, void** ptr);
enum aga_result aga_killfmap(void* ptr, aga_size_t size);
#endif

#ifdef AGA_HAVE_SPAWN

enum aga_result aga_spawn_sync(const char*, char**, const char*);

#endif

#endif
