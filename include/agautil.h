/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_UTIL_H
#define AGA_UTIL_H

#include <agaenv.h>
#include <agaresult.h>

#define AGA_SWAPF(a, b) \
	do { \
		float scratch = b; \
		b = a; \
		a = scratch; \
	} while(0)

enum aga_stream_whence {
	AGA_STREAM_SET,
	AGA_STREAM_END,
	AGA_STREAM_CURRENT
};

enum aga_stream_type {
	AGA_STREAM_FILE,
	AGA_STREAM_MEM
};

struct aga_stream {
	enum aga_stream_type type;
	aga_size_t size;

	union {
		void* file;
		struct aga_mem_stream {
			void* p;
			aga_size_t off;
		} mem;
	} data;
};

enum aga_result aga_mkstream(const char*, struct aga_stream*);
enum aga_result aga_mkstream_mem(void*, aga_size_t, struct aga_stream*);
enum aga_result aga_killstream(struct aga_stream*);

enum aga_result aga_stream_seek(
		struct aga_stream*, enum aga_stream_whence, aga_slong_t);

void* aga_memset(void*, int, aga_size_t);
void* aga_memcpy(void*, const void*, aga_size_t);

aga_bool_t aga_streql(const char*, const char*);
aga_bool_t aga_strneql(const char*, const char*, aga_size_t);
aga_size_t aga_strlen(const char*);

aga_slong_t aga_strtol(const char*);
double aga_strtod(const char*);

char* aga_getenv(const char*);

/*
 * NOTE: `aga_realloc' frees the in pointer on error. This is different from
 * 		 Normal `realloc' but keeping the original pointer on error isn't
 * 		 Something we really need - it complicates EH.
 */
/*
 * NOTE: EH for these is still propagated via. errno for simplicity. Any
 * 		 Alternative impls should set errno in these wrappers if they don't
 * 		 Do so natively.
 */
void* aga_malloc(aga_size_t);
void* aga_calloc(aga_size_t, aga_size_t);
void* aga_realloc(void*, aga_size_t);
void aga_free(void*);

char* aga_strdup(const char*);

#endif
