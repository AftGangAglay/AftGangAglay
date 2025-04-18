/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef ASYS_MEMORY_H
#define ASYS_MEMORY_H

#include <asys/base.h>
#include <asys/result.h>

void asys_memory_zero(void*, asys_size_t);
void asys_memory_copy(void*, const void*, asys_size_t);
void asys_memory_move(void*, const void*, asys_size_t);

int asys_memory_compare(const void*, const void*, asys_size_t);

/*
 * NOTE: These internally handle system-specific error conditions and normalise
 * 		 Them to return null on OOM and a free-able pointer otherwise. Thus, it
 * 		 Is appropriate to respond to a null allocation return by returning
 * 		 `ASYS_RESULT_OOM' with no further EH calls.
 */
void* asys_memory_allocate(asys_size_t);
void* asys_memory_allocate_zero(asys_size_t, asys_size_t);
void* asys_memory_reallocate(void*, asys_size_t);
/* NOTE: Auto-frees the input pointer on OOM */
void* asys_memory_reallocate_safe(void*, asys_size_t);
void asys_memory_free(void*);

#endif
