/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/base.h>
#include <asys/result.h>
#include <asys/system.h>
#include <asys/stream.h>

#ifdef ASYS_UNIX
# include "sys/unix/detail.h"
#endif

/*
 * TODO: stdc detail.
 */
#if 0
enum asys_result asys_file_attribute_length_default(
		const char* path, union asys_file_attribute* attribute) {

	enum asys_result result;

	asys_offset_t offset;
	struct asys_stream stream;

	if((result = asys_stream_new(&stream, path))) return result;

	result = asys_stream_tell(&stream, &offset);
	if(result) goto cleanup;
	result = asys_stream_seek(&stream, ASYS_SEEK_END, 0);
	if(result) goto cleanup;
	result = asys_stream_tell(&stream, &attribute->length);
	if(result) goto cleanup;
	result = asys_stream_seek(&stream, ASYS_SEEK_SET, offset);
	if(result) goto cleanup;

	if((result = asys_stream_delete(&stream))) return result;

	return ASYS_RESULT_OK;

	cleanup: {
		/*
		 * TODO: Seek back to original position -- also verify we always do
		 * 		 In error conditions.
		 */

		asys_log_result(
				__FILE__, "asys_stream_delete", asys_stream_delete(&stream));

		return result;
	}
}
#endif
