/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/base.h>
#include <asys/result.h>
#include <asys/system.h>
#include <asys/stream.h>

#ifdef ASYS_UNIX
#include <asys/system.h>

enum asys_result asys_file_attribute_stat(
		struct stat* buffer, enum asys_file_attribute_field type,
		union asys_file_attribute* attribute) {

	switch(type) {
		default: return ASYS_RESULT_BAD_PARAM;

		case ASYS_FILE_MODIFIED: {
			attribute->modified = buffer->st_mtime;
			break;
		}

		case ASYS_FILE_LENGTH: {
			attribute->length = buffer->st_size;
			break;
		}

		case ASYS_FILE_TYPE: {
#if !defined(S_ISDIR)
# ifdef S_IFDIR
#  define ASYS_ISDIR(mode) (!!(mode & S_IFDIR))
# else
#  define ASYS_ISDIR(mode) (ASYS_FALSE)
# endif
#else
# define ASYS_ISDIR S_ISDIR
#endif

			if(ASYS_ISDIR(buffer->st_mode)) {
				attribute->type = ASYS_FILE_DIRECTORY;
			}
			else attribute->type = ASYS_FILE_REGULAR;

			break;
		}
	}

	return ASYS_RESULT_OK;
}
#endif
