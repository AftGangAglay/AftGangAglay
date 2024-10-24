/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/io.h>
#include <aga/error.h>
#include <aga/log.h>
#include <aga/utility.h>

#define AGA_WANT_UNIX
#include <aga/std.h>

#define AGA_WANT_WINDOWS_H
#include <aga/win32.h>

/* TODO: Implement BSD-y `<sys/dir.h>' `direct' interface  */
enum aga_result aga_directory_iterate(
		const char* path, aga_directory_callback_t fn, aga_bool_t recurse,
		void* pass, aga_bool_t keep_going) {

#ifdef _WIN32
	/*
	 * TODO: These compat. functions appear to have come about in 2000-ish with
	 * 		 XP -- what did 9x/3.1 use to access these "portably"?
	 * 		 There is some dubious evidence that the `_dos_' variants of these
	 * 		 Existed earlier:
	 * 		 		https://jeffpar.github.io/kbarchive/kb/043/Q43144/
	 * 		 		https://www.ee.iitb.ac.in/student/~bhush/C%20Programming%20-%20Just%20the%20FAQs.pdf
	 * 		 Possibly as a Watcom/Borland exclusive extension:
	 *				https://openwatcom.org/ftp/manuals/1.5/clib.pdf
	 *		 There's also something weird with the MSDN docs here:
	 *		 		https://learn.microsoft.com/en-us/cpp/c-runtime-library/system-calls?view=msvc-170
	 *		 As this set of functions are the only ones present.
	 */

	aga_fixed_buf_t buf = { 0 };

	enum aga_result result;
	enum aga_result held_result = AGA_RESULT_OK;

	struct _finddata_t data;

#ifdef _WIN64
	intptr_t find;
#else
	long find;
#endif

	if(sprintf(buf, "%s\\*", path) < 0) {
		return aga_error_system(__FILE__, "sprintf");
	}

	/* TODO: Should this class of DOS compat function use `_doserrno'? */
	if((find = _findfirst(buf, &data)) == -1) {
		return aga_error_system_path(__FILE__, "_findfirst", path);
	}

	do {
		static aga_bool_t did_warn_deprecation = AGA_FALSE;

		if(data.name[0] == '.') continue;

		if(!did_warn_deprecation) {
			/* TODO: Address this in next major release. */
			aga_log(
					__FILE__,
					"warn: Windows directory iteration currently replaces "
					"`\\' with `/' during path concatenation -- this will be "
					"changed in a future release when config can mark inputs "
					"as paths to auto-replace path separators");

			did_warn_deprecation = !did_warn_deprecation;
		}

		if(sprintf(buf, "%s/%s", path, data.name) < 0) {
			result = aga_error_system(__FILE__, "sprintf");
			if(keep_going) {
				held_result = result;
				continue;
			}
			else return result;
		}

		if(data.attrib & _A_SUBDIR) {
			if(recurse) {
				result = aga_directory_iterate(
						buf, fn, recurse, pass, keep_going);

				if(result) {
					if(keep_going) {
						aga_error_check_path_soft(
								__FILE__, "aga_directory_iterate", buf,
								result);

						held_result = result;
						continue;
					}
					else return result;
				}
			}
			else continue;
		}
		else if((result = fn(buf, pass))) {
			if(keep_going) {
				aga_error_check_path_soft(
						__FILE__, "aga_directory_iterate::<callback>", buf,
						result);

				held_result = result;
				continue;
			}
			else return result;
		}
	} while(_findnext(find, &data) != -1);

	if(errno != ENOENT) return aga_error_system(__FILE__, "_findnext");

	return held_result;

#elif defined(AGA_HAVE_DIRENT)
	enum aga_result result;
	enum aga_result held_result = AGA_RESULT_OK;

	DIR* d;
	struct dirent* ent;
	union aga_file_attribute attr;

	if(!(d = opendir(path))) {
		return aga_error_system_path(__FILE__, "opendir", path);
	}

	/* TODO: Leaky EH. */
	while((ent = readdir(d))) {
		aga_fixed_buf_t buf = { 0 };

		if(ent->d_name[0] == '.') continue;

		if(sprintf(buf, "%s/%s", path, ent->d_name) < 0) {
			result = aga_error_system(__FILE__, "sprintf");
			if(keep_going) {
				held_result = result;
				continue;
			}
			else return result;
		}

		if((result = aga_file_attribute_path(buf, AGA_FILE_TYPE, &attr))) {
			if(keep_going) {
				aga_error_check_path_soft(
						__FILE__, "aga_file_attribute_path", buf, result);

				held_result = result;
				continue;
			}
			else return result;
		}

		if(attr.type == AGA_FILE_DIRECTORY) {
			if(recurse) {
				result = aga_directory_iterate(
						buf, fn, recurse, pass, keep_going);

				if(result) {
					if(keep_going) {
						aga_error_check_path_soft(
								__FILE__, "aga_directory_iterate", buf,
								result);

						held_result = result;
						continue;
					}
					else return result;
				}
			}
			else continue;
		}
		else if((result = fn(buf, pass))) {
			if(keep_going) {
				aga_error_check_path_soft(
						__FILE__, "aga_directory_iterate::<callback>", buf,
						result);

				held_result = result;
				continue;
			}
			else return result;
		}
	}

	if(closedir(d) == -1) return aga_error_system(__FILE__, "closedir");

	return held_result;
#else
	(void) path;
	(void) fn;
	(void) recurse;
	(void) pass;
	(void) keep_going;

	return AGA_RESULT_NOT_IMPLEMENTED;
#endif
}

#ifdef AGA_DEVBUILD

enum aga_result aga_file_copy_path(
		const char* to_path, const char* from_path) {

	enum aga_result result;

	/* TODO: Leaky error states. */

	void* to;
	void* from;

	if(!(to = fopen(to_path, "wb"))) {
		return aga_error_system_path(__FILE__, "fopen", to_path);
	}

	if(!(from = fopen(from_path, "rb"))) {
		return aga_error_system_path(__FILE__, "fopen", from_path);
	}

	if((result = aga_file_copy(to, from, AGA_COPY_ALL))) return result;

	if(fclose(to) == EOF) return aga_error_system(__FILE__, "fclose");
	if(fclose(from) == EOF) return aga_error_system(__FILE__, "fclose");

	return AGA_RESULT_OK;
}

enum aga_result aga_file_copy(void* to, void* from, aga_size_t len) {
	if(!to) return AGA_RESULT_BAD_PARAM;
	if(!from) return AGA_RESULT_BAD_PARAM;

	if(!len) return AGA_RESULT_OK;

	/* Generic impl. */

	{
		/* TODO: Make static. */
		aga_fixed_buf_t buf = { 0 };

		enum aga_result result;
		aga_size_t total = 0;
		aga_size_t sz;

		/*
		 * Lets us avoid an `fplen' in the caller and do less branching here.
		 */
		if(len == AGA_COPY_ALL) {
			while((sz = fread(buf, 1, sizeof(buf) - 1, from))) {
				if(fwrite(buf, 1, sz, to) < sz) {
					return aga_error_system(__FILE__, "fwrite");
				}
			}

			if(ferror(from)) return aga_error_system(__FILE__, "fread");

			return AGA_RESULT_OK;
		}

		while(AGA_TRUE) {
			aga_size_t rem = len - total;
			aga_size_t req = rem > sizeof(buf) - 1 ? sizeof(buf) - 1 : rem;

			if((result = aga_file_read(buf, req, from))) return result;
			if(fwrite(buf, 1, req, to) < req) {
				return aga_error_system(__FILE__, "fwrite");
			}

			if(req == rem) break;
			total += req;
		}

		return AGA_RESULT_OK;
	}
}

enum aga_result aga_path_tail(const char* path, aga_size_t size, void* data) {
	enum aga_result result;
	void* fp;

	if(!path) return AGA_RESULT_BAD_PARAM;
	if(!data) return AGA_RESULT_BAD_PARAM;

	if(!(fp = fopen(path, "rb"))) {
		return aga_error_system_path(__FILE__, "fopen", path);
	}

	/*
	 * TODO: Apparently binary streams are not guaranteed to support
	 * 		 `SEEK_END' -- Is this a case we need to handle?
	 */
	if(fseek(fp, -(long) size, SEEK_END)) {
		result = aga_error_system(__FILE__, "fseek");
		goto cleanup;
	}

	result = aga_file_read(data, size, fp);
	if(result) goto cleanup;

	if(fclose(fp) == EOF) return aga_error_system(__FILE__, "fclose");

	return AGA_RESULT_OK;

	cleanup: {
		if(fclose(fp) == EOF) (void) aga_error_system(__FILE__, "fclose");

		return result;
	}
}

enum aga_result aga_path_delete(const char* path) {
	if(!path) return AGA_RESULT_BAD_PARAM;

	if(remove(path)) return aga_error_system(__FILE__, "remove");

	return AGA_RESULT_OK;
}

#endif

#if defined(AGA_HAVE_SYS_STAT) && defined(AGA_HAVE_SYS_TYPES)
# define AGA_HAVE_STAT
#endif

/* TODO: More central NT POSIX subsystem wrapper. */
#ifdef _WIN32
# define AGA_HAVE_STAT
# define stat _stat
# define fstat _fstat
# define S_IFDIR _S_IFDIR
#endif

#ifdef AGA_HAVE_STAT

static enum aga_result aga_file_attribute_select_stat(
		struct stat* st, enum aga_file_attribute_type attr,
		union aga_file_attribute* out) {

	switch(attr) {
		default: return AGA_RESULT_BAD_PARAM;

		case AGA_FILE_MODIFIED: {
			out->modified = st->st_mtime;
			break;
		}

		case AGA_FILE_LENGTH: {
			out->length = st->st_size;
			break;
		}

		case AGA_FILE_TYPE: {
#ifdef S_ISDIR
			if(S_ISDIR(st->st_mode)) out->type = AGA_FILE_DIRECTORY;
			else
#elif defined(S_IFDIR)
			if(st->st_mode & S_IFDIR) out->type = AGA_FILE_DIRECTORY;
			else
#endif
			out->type = AGA_FILE_REGULAR;
			break;
		}
	}

	return AGA_RESULT_OK;
}
#else
static enum aga_result aga_file_attribute_type(
		const char* path, union aga_file_attr* out) {

# ifdef EISDIR
	/* Try and fall back to a solution using `fopen'. */
	FILE* f;

	if(!(f = fopen(path, "r"))) {
		if(errno == EISDIR) *isdir = AGA_TRUE;
		else return aga_error_system_path(__FILE__, "fopen", path);
	}
	else *isdir = AGA_FALSE;

	if(fclose(f) == EOF) {
		return aga_error_system(__FILE__, "fclose");
	}

	return AGA_RESULT_OK;
# else
	return AGA_ERROR_NOT_IMPLEMENTED;
# endif
}

enum aga_result aga_file_attribute_length(void* fp, aga_size_t* size) {
	long off;
	long tell;

	if((off = ftell(fp)) == -1) return aga_error_system(__FILE__, "ftell");

	if(fseek(fp, 0, SEEK_END)) {
		return aga_error_system(__FILE__, "fseek");
	}
	if((tell = ftell(fp)) == -1) return aga_error_system(__FILE__, "ftell");
	*size = (aga_size_t) tell;

	if(fseek(fp, off, SEEK_SET)) {
		return aga_error_system(__FILE__, "fseek");
	}

	return AGA_RESULT_OK;
}

static enum aga_result aga_file_attribute_select(
		void* fp, enum aga_file_attribute_type attr, union aga_file_attribute* out) {

	switch(attr) {
		default: return AGA_RESULT_BAD_PARAM;

		case AGA_FILE_MODIFIED: return AGA_RESULT_NOT_IMPLEMENTED;

		case AGA_FILE_LENGTH: return aga_file_attribute_length(fp, &out->length);

		/* If it's a file handle, it's a regular file. */
		case AGA_FILE_TYPE: {
			out->type = AGA_REGULAR;
			break;
		}
	}

	return AGA_RESULT_OK;
}
#endif

enum aga_result aga_file_attribute_path(
		const char* path, enum aga_file_attribute_type attr,
		union aga_file_attribute* out) {

	if(!path) return AGA_RESULT_BAD_PARAM;
	if(!out) return AGA_RESULT_BAD_PARAM;

#ifdef AGA_HAVE_STAT
	{
		struct stat st;

		if(stat(path, &st) == -1) {
			return aga_error_system_path(__FILE__, "stat", path);
		}

		return aga_file_attribute_select_stat(&st, attr, out);
	}
#else
	if(attr == AGA_FILE_TYPE) return aga_file_attribute_type(path, out);

	{
		enum aga_result result;

		void* fp;

		if(!(fp = fopen(path, "rb"))) {
			return aga_error_system_path(__FILE__, "fopen", path);
		}

		if((result = aga_file_attribute_select(fp, attr, out))) return result;

		if(fclose(fp) == EOF) return aga_error_system(__FILE__, "fclose");

		return AGA_RESULT_OK;
	}
#endif
}

enum aga_result aga_file_attribute(
		void* fp, enum aga_file_attribute_type attr,
		union aga_file_attribute* out) {

	if(!fp) return AGA_RESULT_BAD_PARAM;
	if(!attr) return AGA_RESULT_BAD_PARAM;
	if(!out) return AGA_RESULT_BAD_PARAM;

#ifdef AGA_HAVE_STAT
	{
		struct stat st;
		int fd;

		if((fd = fileno(fp)) == -1) {
			return aga_error_system(__FILE__, "fileno");
		}

		if(fstat(fd, &st) == -1) return aga_error_system(__FILE__, "fstat");

		return aga_file_attribute_select_stat(&st, attr, out);
	}
#else
	return aga_file_attribute_select(fp, attr, out);
#endif
}

enum aga_result aga_file_read(void* data, aga_size_t size, void* fp) {
	enum aga_result result;

	if(!data) return AGA_RESULT_BAD_PARAM;
	if(!fp) return AGA_RESULT_BAD_PARAM;

	if(fread(data, 1, size, fp) != size) {
		if(ferror(fp)) result = aga_error_system(__FILE__, "fread");
		else result = AGA_RESULT_EOF;

		clearerr(fp);

		return result;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_file_print_characters(int c, aga_size_t n, void* fp) {
	aga_size_t i;

	for(i = 0; i < n; ++i) {
		if(putc(c, fp) == EOF) return aga_error_system(__FILE__, "putc");
	}

	return AGA_RESULT_OK;
}

#ifdef AGA_HAVE_SPAWN
# ifdef AGA_NIXSPAWN
/*
 * We take an unusual result value to signal to the main process that the spawn
 * failed before we hit the user program.
 */
#define AGA_SPAWN_EXIT_MAGIC (113)

enum aga_result aga_process_spawn(
		const char* program, char** argv, const char* wd) {

	pid_t p;

	if(!program) return AGA_RESULT_BAD_PARAM;
	if(!argv) return AGA_RESULT_BAD_PARAM;

	if((p = fork()) == -1) return aga_error_system(__FILE__, "fork");

	if(!p) {
		if(wd) {
			if(chdir(wd) == -1) {
				(void) aga_error_system_path(__FILE__, "chdir", wd);
				exit(AGA_SPAWN_EXIT_MAGIC);
			}
		}
		if(execvp(program, argv) == -1) {
			(void) aga_error_system_path(__FILE__, "execvp", program);
			exit(AGA_SPAWN_EXIT_MAGIC);
		}
	}
	else {
		int res;
		if(waitpid(p, &res, 0) == -1) return aga_error_system(__FILE__, "wait");
		if((res = WEXITSTATUS(res))) {
			if(res == AGA_SPAWN_EXIT_MAGIC) {
				aga_log(__FILE__, "err: failed to start `%s'", program);
			}
			else {
				aga_log(
					__FILE__, "err: `%s' exited with exit code %i",
					program, res);
			}

			return AGA_RESULT_ERROR;
		}
		else aga_log(__FILE__, "`%s' exited with exit code 0", program);
	}

	return AGA_RESULT_OK;
}
# elif defined(AGA_WINSPAWN)

# define AGA_WANT_WINDOWS_H

# include <aga/win32.h>

enum aga_result aga_process_spawn(
		const char* program, char** argv, const char* wd) {

	aga_size_t len = 0;
	char* cli = 0;

	STARTUPINFOA startup = { 0 };
	PROCESS_INFORMATION info = { 0 };
	startup.cb = sizeof(startup);

	if(!program) return AGA_RESULT_BAD_PARAM;
	if(!argv) return AGA_RESULT_BAD_PARAM;

	for(; *argv; ++argv) {
		aga_size_t l = strlen(*argv);

		cli = aga_realloc(cli, len + l + 2);
		if(!cli) return aga_error_system(__FILE__, "aga_realloc");

		memcpy(cli + len, *argv, l);
		cli[len + l] = ' ';
		len += l + 1;
	}

	if(cli) cli[len] = 0;

	if(!CreateProcess(0, cli, 0, 0, FALSE, 0, 0, wd, &startup, &info)) {
		aga_free(cli);
		return aga_win32_error(__FILE__, "CreateProcess");
	}

	aga_free(cli);

	if(WaitForSingleObject(info.hProcess, INFINITE) == WAIT_FAILED) {
		return aga_win32_error(__FILE__, "WaitForSingleObject");
	}

	if(!CloseHandle(info.hProcess)) {
		return aga_win32_error(__FILE__, "CloseHandle");
	}
	if(!CloseHandle(info.hThread)) {
		return aga_win32_error(__FILE__, "CloseHandle");
	}

	return AGA_RESULT_OK;
}

# endif
#endif
