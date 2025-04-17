/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/system.h>
#include <asys/stream.h>
#include <asys/error.h>
#include <asys/string.h>
#include <asys/log.h>

/*
 * TODO: In dev builds measure noncontiguous reads to provide optimization
 * 		 Feedback -- backwards seeks are worse.
 */

enum asys_result asys_stream_new(
		struct asys_stream* stream, const char* path) {

#ifdef ASYS_WIN32
	enum asys_result result;

	/*
	 * TODO: The original open flag constant was just `READ' but for modern VC
	 * 		 We appear to need `OF_READ'.
	 */
	if((stream->handle = _lopen(path, OF_READ)) == HFILE_ERROR) {
		result = ASYS_RESULT_ERROR;
		asys_log_result_path(__FILE__, "_lopen", path, result);
		return result;
	}

	/*asys_log(
			__FILE__,
			"_lopen(\"%s\", OF_READ) -> return: %d, errno: %s",
			path, stream->hfile, strerror(errno));*/

	return ASYS_RESULT_OK;
#elif defined(ASYS_UNIX)
	stream->fd = 0;

	if((stream->fd = open(path, O_RDONLY)) == -1) {
		return asys_result_errno_path(__FILE__, "open", path);
	}

	return ASYS_RESULT_OK;
#elif defined(ASYS_STDC)
	stream->fp = 0;

	if(!(stream->fp = fopen(path, "r"))) {
		return asys_result_errno_path(__FILE__, "fopen", path);
	}

	return ASYS_RESULT_OK;
#else
	(void) stream;
	(void) path;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_stream_new_write(
		struct asys_stream* stream, const char* path) {

#ifdef AGA_DEVBUILD
# ifdef ASYS_WIN32
	enum asys_result result;

	if((stream->handle = _lcreat(path, 0)) == HFILE_ERROR) {
		result = ASYS_RESULT_ERROR;
		asys_log_result_path(__FILE__, "_lcreat", path, result);
		return result;
	}

	return ASYS_RESULT_OK;
# elif defined(ASYS_UNIX)
	stream->fd = 0;

	if((stream->fd = open(path, O_WRONLY | O_CREAT, 0666)) == -1) {
		return asys_result_errno_path(__FILE__, "open", path);
	}

	return ASYS_RESULT_OK;
# elif defined(ASYS_STDC)
	stream->fp = 0;

	if(!(stream->fp = fopen(path, "w"))) {
		return asys_result_errno_path(__FILE__, "fopen", path);
	}

	return ASYS_RESULT_OK;
# else
	(void) stream;
	(void) path;

	return ASYS_RESULT_NOT_IMPLEMENTED;
# endif
#else
	(void) stream;
	(void) path;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_stream_delete(struct asys_stream* stream) {
#ifdef ASYS_WIN32
	enum asys_result result;

	if(_lclose(stream->handle) == HFILE_ERROR) {
		result = ASYS_RESULT_ERROR;
		asys_log_result(__FILE__, "_lclose", result);
		return result;
	}

	return ASYS_RESULT_OK;
#elif defined(ASYS_UNIX)
	if(stream->fd && close(stream->fd) == -1) {
		return asys_result_errno(__FILE__, "close");
	}

	return ASYS_RESULT_OK;
#elif defined(ASYS_STDC)
	if(stream->fp && fclose(stream->fp) == EOF) {
		return asys_result_errno(__FILE__, "fclose");
	}

	return ASYS_RESULT_OK;
#else
	(void) stream;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

asys_stream_native_t asys_stream_native(struct asys_stream* stream) {
	return stream->handle;
}

#ifdef ASYS_WIN32
# include <stdio.h>
# include <fcntl.h>
#endif
void* asys_stream_stdc(struct asys_stream* stream) {
#ifdef ASYS_WIN32
	void* file;
	int fd;

	fd = _open_osfhandle((asys_native_long_t) stream->handle , _O_RDONLY);
	if(fd == -1) {
		asys_log_result(__FILE__, "_open_osfhandle", ASYS_RESULT_ERROR);
		return 0;
	}

	if(!(file = fdopen(fd, "r"))) {
		asys_log_result(__FILE__, "fdopen", ASYS_RESULT_ERROR);
		return 0;
	}

	return file;
#elif defined(ASYS_UNIX)
	return fdopen(stream->fd, "r");
#elif defined(ASYS_STDC)
	return stream->fp;
#else
	(void) stream;
	return 0;
#endif
}

#if defined(ASYS_UNIX) || defined(ASYS_STDC)
static int asys_stream_whence_to_stdio(enum asys_stream_whence whence) {
	switch(whence) {
		default: return -1;
		case ASYS_SEEK_SET: return SEEK_SET;
		case ASYS_SEEK_END: return SEEK_END;
		case ASYS_SEEK_CURRENT: return SEEK_CUR;
	}
}
#elif defined(ASYS_WIN32)
static int asys_stream_whence_to_win32(enum asys_stream_whence whence) {
	switch(whence) {
		default: return -1;
		case ASYS_SEEK_SET: return 0;
		case ASYS_SEEK_END: return 2;
		case ASYS_SEEK_CURRENT: return 1;
	}
}
#endif

enum asys_result asys_stream_seek(
		struct asys_stream* stream, enum asys_stream_whence whence,
		asys_offset_t offset) {

#ifdef ASYS_WIN32
	enum asys_result result;

	int seek_whence = asys_stream_whence_to_win32(whence);

	if(_llseek(stream->handle, (LONG) offset, seek_whence) == HFILE_ERROR) {
		result = ASYS_RESULT_ERROR;
		asys_log_result(__FILE__, "_llseek", result);
		return result;
	}

	return ASYS_RESULT_OK;
#elif defined(ASYS_UNIX)
	int seek_whence = asys_stream_whence_to_stdio(whence);

	if(lseek(stream->fd, offset, seek_whence) == -1) {
		return asys_result_errno(__FILE__, "lseek");
	}

	return ASYS_RESULT_OK;
#elif defined(ASYS_STDC)
	int seek_whence = asys_stream_whence_to_stdio(whence);

	if(fseek(stream->fp, offset, seek_whence) == -1) {
		return asys_result_errno(__FILE__, "fseek");
	}

	return ASYS_RESULT_OK;
#else
	(void) stream;
	(void) whence;
	(void) offset;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

/* TODO: This doesn't need to exist with some rework. */
enum asys_result asys_stream_tell(
		struct asys_stream* stream, asys_offset_t* offset) {

#ifdef ASYS_WIN32
	enum asys_result result;

	if((*offset = _llseek(stream->handle, 0, 1)) == HFILE_ERROR) {
		result = ASYS_RESULT_ERROR;
		asys_log_result(__FILE__, "_llseek", result);
		return result;
	}

	return ASYS_RESULT_OK;
#elif defined(ASYS_UNIX)
	if((*offset = lseek(stream->fd, 0, SEEK_CUR)) == -1) {
		return asys_result_errno(__FILE__, "lseek");
	}

	return ASYS_RESULT_OK;
#elif defined(ASYS_STDC)
	long result;

	result = ftell(stream->fp);
	if(result == -1) return asys_result_errno(__FILE__, "ftell");

	*offset = (asys_offset_t) result;

	return ASYS_RESULT_OK;
#else
	(void) stream;
	(void) offset;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_stream_read(
		struct asys_stream* stream, asys_size_t* read_count, void* buffer,
		asys_size_t count) {

#ifdef ASYS_WIN32
	enum asys_result result;

	/* TODO: Need to detect EOF for Python readline. */
	long read_result = _hread(stream->handle, buffer, (LONG) count);
	if(read_count) *read_count = read_result;

	if(read_result == -1L) {
		result = ASYS_RESULT_ERROR;
		asys_log_result(__FILE__, "_hread", result);
		return result;
	}
	else if(read_result < (long) count) return ASYS_RESULT_EOF;

	return ASYS_RESULT_OK;
#elif defined(ASYS_UNIX)
	ssize_t result = read(stream->fd, buffer, count);
	if(read_count) *read_count = (asys_size_t) result;

	if(result == (ssize_t) count) return ASYS_RESULT_OK;
	else if(result == -1) return asys_result_errno(__FILE__, "read");
	else return ASYS_RESULT_EOF;
#elif defined(ASYS_STDC)
	asys_size_t result = fread(buffer, 1, count, stream->fp);
	if(read_count) *read_count = result;

	if(feof(stream->fp)) {
		clearerr(stream->fp);
		return ASYS_RESULT_EOF;
	}
	else if(ferror(stream->fp)) {
		clearerr(stream->fp);
		return asys_result_errno(__FILE__, "fread");
	}

	return ASYS_RESULT_OK;
#else
	(void) stream;
	(void) read_count;
	(void) buffer;
	(void) count;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_stream_read_line(
		struct asys_stream* stream, void* buffer, asys_size_t count) {

	enum asys_result result;

	char character;
	char* bytes = buffer;
	char* current = buffer;

	while(!(result = asys_stream_read(stream, 0, &character, 1))) {
		if((asys_size_t) (current - bytes) == count - 1) break;

		*current++ = character;

		if(character == '\n') break;
	}

	*current = 0;

	return result;
}

/* TODO: File kind of a stream is always `FILE'. */
enum asys_result asys_stream_attribute(
		struct asys_stream* stream, enum asys_file_attribute_type type,
		union asys_file_attribute* attribute) {

#ifdef ASYS_WIN32
	enum asys_result result;

	if(type == ASYS_FILE_TYPE) {
		attribute->type = ASYS_FILE_REGULAR;
		return ASYS_RESULT_OK;
	}

	switch(type) {
		default: return ASYS_RESULT_BAD_PARAM;

		/*
		 * NOTE: The era-accurate call (`_getftime') no longer exists in
		 * 		 Modern win32 so we need this mess.
		 */
		case ASYS_FILE_MODIFIED: {
			ULARGE_INTEGER integer;
			FILETIME modified;
			HANDLE handle;

			handle = (void*) (asys_native_long_t) stream->handle;
			if(!GetFileTime(handle, 0, 0, &modified)) {
				result = ASYS_RESULT_ERROR;
				asys_log_result(__FILE__, "GetFileTime", result);
				return result;
			}

			integer.LowPart = modified.dwLowDateTime;
			integer.HighPart = modified.dwHighDateTime;
			attribute->modified = integer.QuadPart;

			return ASYS_RESULT_OK;
		}

		/*
		 * NOTE: The era-accurate call (`_filelength') was changed at some
		 * 		 Point to use `fd's rather than `HFILE' so it cannot be used
		 * 		 On modern Windows.
		 */
		case ASYS_FILE_LENGTH: {
			ULARGE_INTEGER integer;
			DWORD low;
			HANDLE handle;

			handle = (void*) (asys_native_long_t) stream->handle;
			low = GetFileSize(handle, &integer.HighPart);
			if(low == INVALID_FILE_SIZE) {
				result = ASYS_RESULT_ERROR;
				asys_log_result(__FILE__, "GetFileSize", result);
				return result;
			}

			integer.LowPart = low;

			attribute->length = integer.QuadPart;

			return ASYS_RESULT_OK;
		}
	}
#elif defined(ASYS_UNIX)
	struct stat buffer;

	if(fstat(stream->fd, &buffer) == -1) {
		return asys_result_errno(__FILE__, "fstat");
	}

	return asys_file_attribute_stat(&buffer, type, attribute);
#elif defined(ASYS_STDC)
	(void) stream;
	(void) type;
	(void) attribute;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#else
	(void) stream;
	(void) type;
	(void) attribute;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_stream_write(
		struct asys_stream* stream, const void* buffer, asys_size_t count) {

#ifdef AGA_DEVBUILD
# ifdef ASYS_WIN32
	enum asys_result result;

	if(_hwrite(stream->handle, buffer, (long) count) == -1L) {
		result = ASYS_RESULT_ERROR;
		asys_log_result(__FILE__, "_hwrite", result);
		return result;
	}

	return ASYS_RESULT_OK;
# elif defined(ASYS_UNIX)
	if(write(stream->fd, buffer, count) == -1) {
		return asys_result_errno(__FILE__, "write");
	}

	return ASYS_RESULT_OK;
# elif defined(ASYS_STDC)
	fwrite(buffer, 1, count, stream->fp);

	if(ferror(stream->fp)) {
		clearerr(stream->fp);
		return asys_result_errno(__FILE__, "fwrite");
	}

	return ASYS_RESULT_OK;
# else
	(void) stream;
	(void) buffer;
	(void) count;

	return ASYS_RESULT_NOT_IMPLEMENTED;
# endif
#else
	(void) stream;
	(void) buffer;
	(void) count;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_stream_write_format(
		struct asys_stream* stream, const char* format, ...) {

#ifdef AGA_DEVBUILD
	enum asys_result result;

	va_list list;

	va_start(list, format);

	result = asys_stream_write_format_variadic(stream, format, list);

	va_end(list);

	return result;
#else
	(void) stream;
	(void) format;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_stream_write_format_variadic(
		struct asys_stream* stream, const char* format, va_list list) {

#ifdef AGA_DEVBUILD
	static asys_fixed_buffer_t buffer = { 0 };

	enum asys_result result;
	asys_size_t count;

	result = asys_string_format_variadic(&buffer, &count, format, list);
	if(result) return result;

	return asys_stream_write(stream, buffer, count);
#else
	(void) stream;
	(void) format;
	(void) list;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_stream_write_characters(
		struct asys_stream* stream, char character, asys_size_t count) {

#ifdef AGA_DEVBUILD
	enum asys_result result;
	asys_size_t i;

	for(i = 0; i < count; ++i) {
		if((result = asys_stream_write(stream, &character, 1))) return result;
	}

	return ASYS_RESULT_OK;
#else
	(void) stream;
	(void) character;
	(void) count;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_stream_splice(
		struct asys_stream* to, struct asys_stream* from, asys_size_t count) {

#ifdef AGA_DEVBUILD
	static asys_fixed_buffer_t buffer = { 0 };

	enum asys_result result;

	asys_size_t total = 0;

	/*
	 * Lets us avoid a call to get the file length in the caller and do less
	 * branching in here.
	 */
	if(count == ASYS_COPY_ALL) {
		asys_size_t read_count;

		while(ASYS_TRUE) {
			enum asys_result held_result;

			result = asys_stream_read(
					from, &read_count, buffer, sizeof(buffer));

			if(result && result != ASYS_RESULT_EOF) return result;
			if(!read_count) break;

			held_result = result;

			result = asys_stream_write(to, buffer, read_count);
			if(result) return result;

			if(held_result == ASYS_RESULT_EOF) break;
		}

		return ASYS_RESULT_OK;
	}

	while(ASYS_TRUE) {
		asys_size_t remaining = count - total;
		asys_size_t to_read;

		if(remaining > sizeof(buffer) - 1) to_read = sizeof(buffer) - 1;
		else to_read = remaining;

		result = asys_stream_read(from, 0, buffer, to_read);
		if(result) return result;

		result = asys_stream_write(to, buffer, to_read);
		if(result) return result;

		if(to_read == remaining) break;
		total += to_read;
	}

	return ASYS_RESULT_OK;
#else
	(void) to;
	(void) from;
	(void) count;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}
