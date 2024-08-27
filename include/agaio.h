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

#define AGA_COPY_ALL ((aga_size_t) -1)

enum aga_fileattr_req {
	AGA_FILE_MODIFIED,
	AGA_FILE_LENGTH,
	AGA_FILE_TYPE
};

enum aga_file_type {
	AGA_FILE_REGULAR,
	AGA_FILE_DIRECTORY
};

union aga_fileattr {
	aga_slong_t modified;
	aga_size_t length;
	enum aga_file_type type;
};

typedef enum aga_result (*aga_iterfn_t)(const char*, void*);

/* TODO: Remove most of these ops in non-devbuilds -- move to separate file. */

enum aga_result aga_iterate_dir(
		const char*, aga_iterfn_t, aga_bool_t, void*, aga_bool_t);

#ifdef AGA_DEVBUILD
enum aga_result aga_copyfile_path(const char*, const char*);
/*
 * Respects initial stream positions.
 * `len == AGA_COPY_ALL' results in a copy until EOF.
 */
enum aga_result aga_copyfile(void*, void*, aga_size_t);
#endif

enum aga_result aga_fileattr(
		void*, enum aga_fileattr_req, union aga_fileattr*);

enum aga_result aga_fileattr_path(
		const char*, enum aga_fileattr_req, union aga_fileattr*);

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
