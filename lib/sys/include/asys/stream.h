/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+asys@pm.me>
 */

#ifndef ASYS_STREAM_H
#define ASYS_STREAM_H

#include <asys/base.h>
#include <asys/result.h>
#include <asys/filedata.h>

#ifdef ASYS_WIN32
typedef int asys_stream_native_t;
#elif defined(ASYS_UNIX)
typedef int asys_stream_native_t;
#elif defined(ASYS_STDC)
typedef void* asys_stream_native_t;
#else
/* TODO: RAM-disk support with an embedded pack? */
typedef int asys_stream_native_t;
#endif

struct asys_stream {
	asys_stream_native_t handle;
	/*
	 * TODO: In dev+debug builds -- remember the open mode for the stream and
	 * 		 Fail early with invalid operations.
	 */
	/*
	 * TODO: In dev/debug mode, remember the original file's path and use the
	 * 		 `_path' variants of error functions in all stream IO functions.
	 */
};

#define ASYS_COPY_ALL ((asys_size_t) -1)

enum asys_stream_whence {
	ASYS_SEEK_SET,
	ASYS_SEEK_END,
	ASYS_SEEK_CURRENT
};

enum asys_result asys_stream_new(struct asys_stream*, const char*);
enum asys_result asys_stream_delete(struct asys_stream*);

/* TODO: This shouldn't be necessary once we have all dependencies on-board. */
asys_stream_native_t asys_stream_native(struct asys_stream*);
void* asys_stream_stdc(struct asys_stream*);

enum asys_result asys_stream_seek(
		struct asys_stream*, enum asys_stream_whence, asys_offset_t);

enum asys_result asys_stream_tell(struct asys_stream*, asys_offset_t*);

enum asys_result asys_stream_read(
		struct asys_stream*, asys_size_t*, void*, asys_size_t);

/* NOTE: Replicates `fgets'-style line reads. */
enum asys_result asys_stream_read_line(
		struct asys_stream*, void*, asys_size_t);

enum asys_result asys_stream_attribute(
		struct asys_stream*, enum asys_file_attribute_field,
		union asys_file_attribute*);

/* NOTE: No stream-writing IO functions are available outside of dev builds. */
enum asys_result asys_stream_new_write(struct asys_stream*, const char*);

enum asys_result asys_stream_write(
		struct asys_stream*, const void*, asys_size_t);

enum asys_result asys_stream_write_format(
		struct asys_stream*, const char*, ...);

#ifdef ASYS_VARARGS_H
enum asys_result asys_stream_write_format_variadic(
		struct asys_stream*, const char*, va_list);
#endif

enum asys_result asys_stream_write_characters(
		struct asys_stream*, char, asys_size_t);

/*
 * Respects initial stream positions.
 * `count == ASYS_COPY_ALL' results in a copy until EOF.
 */
enum asys_result asys_stream_splice(
		struct asys_stream*, struct asys_stream*, asys_size_t);

#endif
