/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+asys@pm.me>
 */

#ifndef ASYS_FILEDATA_H
#define ASYS_FILEDATA_H

#include <asys/base.h>

enum asys_file_attribute_field {
	ASYS_FILE_MODIFIED,
	ASYS_FILE_LENGTH,
	ASYS_FILE_TYPE
};

enum asys_file_type {
	ASYS_FILE_REGULAR,
	ASYS_FILE_DIRECTORY
};

union asys_file_attribute {
	asys_time_t modified;
	asys_size_t length;
	enum asys_file_type type;
};

#endif
