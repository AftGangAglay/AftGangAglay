/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/memory.h>
#include <asys/system.h>
#include <asys/error.h>
#include <asys/log.h>

void asys_memory_zero(void* pointer, asys_size_t count) {
/* TODO: Detect `bzero'. */
/* TODO: Does `memset' work on Windows without CRT? */
#ifdef ASYS_STDC
	memset(pointer, 0, count);
#else
	asys_size_t i;
	char* bytes = pointer;

	for(i = 0; i < count; ++i) bytes[i] = 0;
#endif
}

void asys_memory_copy(void* to, const void* from, asys_size_t count) {
#ifdef ASYS_WIN32
	hmemcpy(to, from, count);
#elif defined(ASYS_STDC)
	memcpy(to, from, count);
#else
	asys_size_t i;
	char* to_bytes = to;
	const char* from_bytes = from;

	for(i = 0; i < count; ++i) to_bytes[i] = from_bytes[i];
#endif
}

/* TODO: Temporary -- make a better tracker. */
#if defined(ASYS_WIN32) && defined(ASYS_TRACK_MEMORY)
static asys_size_t aga_global_memory_use = 0;
#endif

void* asys_memory_allocate(asys_size_t size) {
#ifdef ASYS_WIN32
	void* pointer;

	pointer = GlobalAlloc(0, size);
	if(!pointer) asys_log_result(__FILE__, "GlobalAlloc", ASYS_RESULT_OOM);

# ifdef ASYS_TRACK_MEMORY
	aga_global_memory_use += size;
	asys_log(
			__FILE__,
			"Allocated block of size `" ASYS_NATIVE_ULONG_FORMAT "' "
			"(total `" ASYS_NATIVE_ULONG_FORMAT "')",
			size, aga_global_memory_use);
# endif

	return pointer;
#elif defined(ASYS_STDC)
	void* pointer;

	pointer = malloc(size);
	if(!pointer) (void) asys_result_errno(__FILE__, "malloc");

	return pointer;
#elif defined(ASYS_UNIX)
	/* TODO: Was there a more *nix-y way to do allocations pre-std? */
	(void) size;

	return 0;
#else
	/* TODO: Heap implementation? */
	(void) size;

	return 0;
#endif
}

void* asys_memory_allocate_zero(asys_size_t count, asys_size_t size) {
#ifdef ASYS_WIN32
	void* pointer;

	pointer = GlobalAlloc(GMEM_ZEROINIT, count * size);
	if(!pointer) asys_log_result(__FILE__, "GlobalAlloc", ASYS_RESULT_OOM);

# ifdef ASYS_TRACK_MEMORY
	aga_global_memory_use += size;
	asys_log(
			__FILE__,
			"Allocated block of size `" ASYS_NATIVE_ULONG_FORMAT "' "
			"(total `" ASYS_NATIVE_ULONG_FORMAT "')",
			size, aga_global_memory_use);
# endif

	return pointer;
#elif defined(ASYS_STDC)
	void* pointer;

	pointer = calloc(count, size);
	if(!pointer) (void) asys_result_errno(__FILE__, "calloc");

	return pointer;
#elif defined(ASYS_UNIX)
	(void) size;
	(void) count;

	return 0;
#else
	(void) size;
	(void) count;

	return 0;
#endif
}

void asys_memory_free(void* pointer) {
#ifdef ASYS_WIN32
# ifdef ASYS_TRACK_MEMORY
	asys_size_t size;
# endif

	if(!pointer) return;

# ifdef ASYS_TRACK_MEMORY
	if(!(size = GlobalSize(pointer))) {
		asys_log_result(__FILE__, "GlobalSize", ASYS_RESULT_ERROR);
	}
# endif

	if(GlobalFree(pointer)) {
		asys_log_result(__FILE__, "GlobalFree", ASYS_RESULT_ERROR);
	}

# ifdef ASYS_TRACK_MEMORY
	aga_global_memory_use -= size;
	asys_log(
			__FILE__,
			"Freed block of size `" ASYS_NATIVE_ULONG_FORMAT "' "
			"(total `" ASYS_NATIVE_ULONG_FORMAT "')",
			size, aga_global_memory_use);
# endif
#elif defined(ASYS_STDC)
	free(pointer);
#elif defined(ASYS_UNIX)
	(void) pointer;
#else
	(void) pointer;
#endif
}

/*
 * TODO: Add zeroed realloc to take advantage of `GlobalReAlloc' native zero
 * 		 Init.
 */
void* asys_memory_reallocate(void* pointer, asys_size_t size) {
#ifdef ASYS_WIN32
# ifdef ASYS_TRACK_MEMORY
	asys_size_t old_size;
	asys_isize_t delta;
# endif

	if(!pointer) {
# ifdef ASYS_TRACK_MEMORY
		old_size = 0;
		delta = size;
# endif

		if(!(pointer = GlobalAlloc(0, size))) {
			asys_log_result(__FILE__, "GlobalAlloc", ASYS_RESULT_OOM);
		}
	}
	else {
# ifdef ASYS_TRACK_MEMORY
		if(!(old_size = GlobalSize(pointer))) {
			asys_log_result(__FILE__, "GlobalSize", ASYS_RESULT_ERROR);
		}
# endif

# ifdef ASYS_TRACK_MEMORY
		delta = size - old_size;
# endif

		if(!(pointer = GlobalReAlloc(pointer, size, GMEM_MOVEABLE))) {
			asys_log_result(__FILE__, "GlobalReAlloc", ASYS_RESULT_OOM);
		}
	}

# ifdef ASYS_TRACK_MEMORY
	/* TODO: Message and don't adjust size on allocation failure. */
	aga_global_memory_use += delta;
	asys_log(
			__FILE__,
			"Reallocated block of size `" ASYS_NATIVE_ULONG_FORMAT "' "
			"to `" ASYS_NATIVE_ULONG_FORMAT "' "
			"(total `" ASYS_NATIVE_ULONG_FORMAT "')",
			old_size, size, aga_global_memory_use);
# endif

	return pointer;
#elif defined(ASYS_STDC)
	if(!(pointer = realloc(pointer, size))) {
		(void) asys_result_errno(__FILE__, "realloc");
	}

	return pointer;
#elif defined(ASYS_UNIX)
	(void) pointer;
	(void) size;

	return 0;
#else
	(void) pointer;
	(void) size;

	return 0;
#endif
}

/* NOTE: Auto-frees the input pointer on OOM */
void* asys_memory_reallocate_safe(void* pointer, asys_size_t size) {
	void* new;

	if(!(new = asys_memory_reallocate(pointer, size))) {
		asys_memory_free(pointer);
	}

	return new;
}
