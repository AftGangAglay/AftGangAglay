/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+asys@pm.me>
 */

#ifndef ASYS_FILE_H
#define ASYS_FILE_H

#include <asys/base.h>
#include <asys/result.h>
#include <asys/filedata.h>

typedef enum asys_result (*aga_directory_callback_t)(const char*, void*);

/* NOTE: No path-wise IO functions are available outside of dev builds. */

enum asys_result asys_path_attribute(
		const char*, enum asys_file_attribute_field,
		union asys_file_attribute*);

enum asys_result asys_path_older(const char*, const char*, asys_bool_t*);

enum asys_result asys_path_tail(const char*, void*, asys_size_t);
enum asys_result asys_path_remove(const char*);

enum asys_result asys_path_iterate(
		const char*, aga_directory_callback_t, asys_bool_t, void*,
		asys_bool_t);

#endif
