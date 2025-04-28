/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/file.h>
#include <asys/system.h>
#include <asys/error.h>
#include <asys/stream.h>
#include <asys/log.h>
#include <asys/string.h>

enum asys_result asys_path_attribute(
		const char* path, enum asys_file_attribute_field type,
		union asys_file_attribute* attribute) {

#ifdef AGA_DEVBUILD
# ifdef ASYS_WIN32
	enum asys_result result;

	struct asys_stream stream;

	if(type == ASYS_FILE_TYPE) {
		DWORD attributes;

		/*
		 * Not era-accurate call (`_getfileattr').
		 * Did the historical version return `-1' aswell?
		 */
		if((attributes = GetFileAttributes(path)) == (DWORD) -1) {
			result = ASYS_RESULT_ERROR;
			asys_log_result_path(__FILE__, "GetFileAttributes", path, result);
			return result;
		}

		/* TODO: Does this constant work everywhere today? */
		if(attributes & _A_SUBDIR) attribute->type = ASYS_FILE_DIRECTORY;
		else attribute->type = ASYS_FILE_REGULAR;

		return ASYS_RESULT_OK;
	}

	result = asys_stream_new(&stream, path);
	if(result) return result;

	result = asys_stream_attribute(&stream, type, attribute);
	if(result) goto cleanup;

	result = asys_stream_delete(&stream);
	if(result) return result;

	return ASYS_RESULT_OK;

	cleanup: {
		asys_log_result(
				__FILE__, "asys_stream_delete", asys_stream_delete(&stream));

		return result;
	}
# elif defined(ASYS_UNIX)
	struct stat buffer;

	if(stat(path, &buffer) == -1) {
		return asys_result_errno_path(__FILE__, "stat", path);
	}

	return asys_file_attribute_stat(&buffer, type, attribute);
# else
	(void) path;
	(void) type;
	(void) attribute;

	return ASYS_RESULT_NOT_IMPLEMENTED;
# endif
#else
	(void) path;
	(void) type;
	(void) attribute;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

enum asys_result asys_path_older(
		const char* a, const char* b, asys_bool_t* a_older) {

	enum asys_result result;

	union asys_file_attribute a_attribute = { 0 };
	union asys_file_attribute b_attribute = { 0 };

	result = asys_path_attribute(a, ASYS_FILE_MODIFIED, &a_attribute);
	if(result) return result;

	result = asys_path_attribute(b, ASYS_FILE_MODIFIED, &b_attribute);
	if(result) return result;

	*a_older = a_attribute.modified <= b_attribute.modified;

	return ASYS_RESULT_OK;
}

enum asys_result asys_path_tail(
		const char* path, void* buffer, asys_size_t count) {

#ifdef AGA_DEVBUILD
	enum asys_result result;

	struct asys_stream stream;

	result = asys_stream_new(&stream, path);
	if(result) return result;

	result = asys_stream_seek(&stream, ASYS_SEEK_END, -(asys_offset_t) count);
	if(result) goto cleanup;

	result = asys_stream_read(&stream, 0, buffer, count);
	if(result) goto cleanup;

	return asys_stream_delete(&stream);

	cleanup: {
		asys_log_result(
				__FILE__, "asys_stream_delete", asys_stream_delete(&stream));

		return result;
	}
#else
	(void) path;
	(void) buffer;
	(void) count;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif

}

enum asys_result asys_path_remove(const char* path) {
#ifdef AGA_DEVBUILD
# ifdef ASYS_WIN32
	enum asys_result result;

	if(_unlink(path) == -1) {
		/*
		 * TODO: Ensure all Windows call errors log like this for parity with
		 * 		 *nix-y/stdc EH.
		 */
		result = ASYS_RESULT_ERROR;
		asys_result_check_path(__FILE__, "_unlink", path, result);
		return result;
	}

	return ASYS_RESULT_OK;
# elif defined(ASYS_UNIX)
	(void) path;

	return ASYS_RESULT_NOT_IMPLEMENTED;
# elif defined(ASYS_STDC)
	if(remove(path) == -1) {
		return asys_result_errno_path(__FILE__, "remove", path);
	}

	return ASYS_RESULT_OK;
# else
	(void) path;

	return ASYS_RESULT_NOT_IMPLEMENTED;
# endif
#else
	(void) path;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

/* TODO: Implement BSD-y `<sys/dir.h>' `direct' interface  */
enum asys_result asys_path_iterate(
		const char* path, aga_directory_callback_t callback,
		asys_bool_t recurse, void* pass, asys_bool_t keep_going) {

#ifdef ASYS_WIN32
	/*
	 * TODO: DOS' C interface shipped `_dos_' functions for these which no
	 * 		 Longer appear to exist:
	 * 		 		https://github.com/microsoft/MS-DOS/blob/main/
	 * 		 				/v4.0/src/TOOLS/BLD/INC/DOS.H
	 * 		 But were at least present until Windows 3.1's SDK (and probably
	 * 		 9x/ME if we go and check) so we should switch between NT-y
	 * 		 Behaviour and the `_dos_' functions based on target. Did programs
	 * 		 Using these really just stop working whenever they were removed?
	 */

	/* NOTE: This cannot be static as this function recurses. */
	asys_fixed_buffer_t buffer = { 0 };

	enum asys_result result;
	enum asys_result held_result = ASYS_RESULT_OK;

	/* TODO: `_dos_' variants used `find_t'. */
	struct _finddata_t data;

	asys_native_long_t find;

	result = asys_string_format(&buffer, 0, "%s\\*", path);
	if(result) return result;

	/*
	 * TODO: Should this class of DOS compat function use `_doserrno' or
	 * 		 `dosexterr'?
	 */
	if((find = _findfirst(buffer, &data)) == -1) {
		result = ASYS_RESULT_ERROR;
		asys_log_result(__FILE__, "_findfirst", result);
		return result;
	}

	do {
		if(data.name[0] == '.') continue;

		result = asys_string_format(&buffer, 0, "%s\\%s", path, data.name);
		if(result) {
			if(keep_going) {
				held_result = result;
				continue;
			}
			else return result;
		}

		if(data.attrib & _A_SUBDIR) {
			if(recurse) {
				result = asys_path_iterate(
						buffer, callback, recurse, pass, keep_going);

				if(result) {
					if(keep_going) {
						asys_log_result_path(
								__FILE__, "asys_path_iterate", buffer,
								result);

						held_result = result;
						continue;
					}
					else return result;
				}
			}
			else continue;
		}
		else if((result = callback(buffer, pass))) {
			if(keep_going) {
				asys_log_result_path(
						__FILE__, "asys_path_iterate::<callback>", buffer,
						result);

				held_result = result;
				continue;
			}
			else return result;
		}
	} while(_findnext(find, &data) != -1);

	return held_result;
#elif defined(ASYS_UNIX)
	enum asys_result result;
	enum asys_result held_result = ASYS_RESULT_OK;

	DIR* d;
	struct dirent* ent;
	union asys_file_attribute attr;

	if(!(d = opendir(path))) {
		return asys_result_errno_path(__FILE__, "opendir", path);
	}

	/* TODO: Leaky EH. */
	while((ent = readdir(d))) {
		asys_fixed_buffer_t buffer = { 0 };

		if(ent->d_name[0] == '.') continue;

		if(sprintf(buffer, "%s/%s", path, ent->d_name) < 0) {
			result = asys_result_errno(__FILE__, "sprintf");
			if(keep_going) {
				held_result = result;
				continue;
			}
			else return result;
		}

		if((result = asys_path_attribute(buffer, ASYS_FILE_TYPE, &attr))) {
			if(keep_going) {
				asys_log_result_path(
						__FILE__, "asys_path_attribute", buffer, result);

				held_result = result;
				continue;
			}
			else return result;
		}

		if(attr.type == ASYS_FILE_DIRECTORY) {
			if(recurse) {
				result = asys_path_iterate(
						buffer, callback, recurse, pass, keep_going);

				if(result) {
					if(keep_going) {
						asys_log_result_path(
								__FILE__, "asys_path_iterate", buffer,
								result);

						held_result = result;
						continue;
					}
					else return result;
				}
			}
			else continue;
		}
		else if((result = callback(buffer, pass))) {
			if(keep_going) {
				asys_log_result_path(
						__FILE__, "asys_path_iterate::<callback>", buffer,
						result);

				held_result = result;
				continue;
			}
			else return result;
		}
	}

	if(closedir(d) == -1) return asys_result_errno(__FILE__, "closedir");

	return held_result;
#else
	(void) path;
	(void) callback;
	(void) recurse;
	(void) pass;
	(void) keep_going;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}
