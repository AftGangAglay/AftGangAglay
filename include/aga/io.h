/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_IO_H
#define AGA_IO_H

#include <aga/environment.h>

#include <aga/result.h>

#if defined(AGA_HAVE_SYS_WAIT) && defined(AGA_HAVE_UNISTD)
# define AGA_NIXSPAWN
# define AGA_HAVE_SPAWN
#elif defined(_WIN32)
# define AGA_WINSPAWN
# define AGA_HAVE_SPAWN
#endif

#define AGA_COPY_ALL ((aga_size_t) -1)

enum aga_file_attribute_type {
	AGA_FILE_MODIFIED,
	AGA_FILE_LENGTH,
	AGA_FILE_TYPE
};

enum aga_file_type {
	AGA_FILE_REGULAR,
	AGA_FILE_DIRECTORY
};

union aga_file_attribute {
	aga_slong_t modified;
	aga_size_t length;
	enum aga_file_type type;
};

typedef enum aga_result (*aga_directory_callback_t)(const char*, void*);

/* TODO: Remove most of these ops in non-devbuilds -- move to separate file. */

enum aga_result aga_directory_iterate(
		const char*, aga_directory_callback_t, aga_bool_t, void*, aga_bool_t);

#ifdef AGA_DEVBUILD
enum aga_result aga_file_copy_path(const char*, const char*);
/*
 * Respects initial stream positions.
 * `len == AGA_COPY_ALL' results in a copy until EOF.
 */
enum aga_result aga_file_copy(void*, void*, aga_size_t);
#endif

enum aga_result aga_file_attribute_path(
		const char*, enum aga_file_attribute_type, union aga_file_attribute*);

enum aga_result aga_file_attribute(
		void*, enum aga_file_attribute_type, union aga_file_attribute*);

enum aga_result aga_file_read(void*, aga_size_t, void*);

enum aga_result aga_file_print_characters(int, aga_size_t, void*);

#ifdef AGA_HAVE_SPAWN
enum aga_result aga_process_spawn(const char*, char**, const char*);
#endif

#endif
