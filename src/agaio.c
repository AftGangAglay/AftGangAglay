/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaio.h>
#include <agaerr.h>
#include <agalog.h>
#include <agautil.h>

#define AGA_WANT_UNIX
#include <agastd.h>

#define AGA_WANT_WINDOWS_H
#include <agaw32.h>

/* TODO: Implement BSD-y `<sys/dir.h>' `direct' interface  */
enum aga_result aga_iterate_dir(
		const char* path, aga_iterfn_t fn, aga_bool_t recurse, void* pass,
		aga_bool_t keep_going) {

#ifdef AGA_HAVE_DIRENT
	enum aga_result result;
	enum aga_result held_result = AGA_RESULT_OK;

	DIR* d;
	struct dirent* ent;
	union aga_fileattr attr;

	if(!(d = opendir(path))) return aga_errno_path(__FILE__, "opendir", path);

	/* TODO: Leaky EH. */
	while((ent = readdir(d))) {
		aga_fixed_buf_t buf = { 0 };

		if(ent->d_name[0] == '.') continue;

		if(sprintf(buf, "%s/%s", path, ent->d_name) < 0) {
			result = aga_errno(__FILE__, "sprintf");
			if(keep_going) {
				held_result = result;
				continue;
			}
			else return result;
		}

		if((result = aga_fileattr_path(buf, AGA_FILE_TYPE, &attr))) {
			if(keep_going) {
				aga_soft(__FILE__, "aga_fileattr_path", result);
				held_result = result;
				continue;
			}
			else return result;
		}

		if(attr.type == AGA_FILE_DIRECTORY) {
			if(recurse) {
				result = aga_iterate_dir(buf, fn, recurse, pass, keep_going);
				if(result) {
					if(keep_going) {
						aga_soft(__FILE__, "aga_iterate_dir", result);
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
				aga_soft(__FILE__, "aga_iterate_dir::<callback>", result);
				held_result = result;
				continue;
			}
			else return result;
		}
	}

	if(closedir(d) == -1) return aga_errno(__FILE__, "closedir");

	return held_result;
#else
	return AGA_RESULT_NOT_IMPLEMENTED;
#endif
}

#ifdef AGA_DEVBUILD

enum aga_result aga_copyfile_path(const char* to, const char* from) {
	enum aga_result result;

# ifdef AGA_HAVE_COPYFILE
	/* OSX. */
	/* TODO: copyfile */
# endif

	/* TODO: Leaky error states. */

	void* tof;
	void* fromf;

	if(!(tof = fopen(to, "wb"))) return aga_errno_path(__FILE__, "fopen", to);
	if(!(fromf = fopen(from, "rb"))) {
		return aga_errno_path(__FILE__, "fopen", from);
	}

	if((result = aga_copyfile(tof, fromf, AGA_COPY_ALL))) return result;

	if(fclose(tof) == EOF) return aga_errno(__FILE__, "fclose");
	if(fclose(fromf) == EOF) return aga_errno(__FILE__, "fclose");

	return AGA_RESULT_OK;
}

enum aga_result aga_copyfile(void* to, void* from, aga_size_t len) {
	if(!to) return AGA_RESULT_BAD_PARAM;
	if(!from) return AGA_RESULT_BAD_PARAM;

	if(!len) return AGA_RESULT_OK;

	/* TODO: Use `uname' to detect runtime capabilities. */
# ifdef __linux__
#  if 0 && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 5 && \
		LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)

#   define AGA_COPYFILE_DONE
	/*
	 * `splice' here would not work prior to 2.6.30 as neither operand is a
	 * Pipe.
	 */
	/* TODO: splice */
#  elif 0 && defined(AGA_HAVE_SYS_SENDFILE) && \
		(LINUX_VERSION_CODE <= KERNEL_VERSION(2, 4, 0) || \
		LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))

#   define AGA_COPYFILE_DONE
	/*
	 * `sendfile' from Linux 2.4 to 2.6.33 requires that target be a socket.
	 */
	/* TODO: sendfile */
#  endif
# endif

# if 0 && !defined(AGA_COPYFILE_DONE) && \
		(__FreeBSD__ >= 13 || (__GLIBC__ >= 2 && __GLIBC_MINOR__ >= 27))

#  define AGA_COPYFILE_DONE
	/* TODO: copy_file_range */
# endif

# if 0 && !defined(AGA_COPYFILE_DONE) && defined(AGA_HAVE_COPYFILE)
	/* OSX. */
	/* TODO: fcopyfile */
# endif

# if 0 && !defined(AGA_COPYFILE_DONE) && defined(_WIN32)
	/* TODO: CopyFile */
# endif

# ifndef AGA_COPYFILE_DONE
	/* Generic impl. */

	{
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
					return aga_errno(__FILE__, "fwrite");
				}
			}

			if(ferror(from)) return aga_errno(__FILE__, "fread");

			return AGA_RESULT_OK;
		}

		while(AGA_TRUE) {
			aga_size_t rem = len - total;
			aga_size_t req = rem > sizeof(buf) - 1 ? sizeof(buf) - 1 : rem;

			if((result = aga_fread(buf, req, from))) return result;
			if(fwrite(buf, 1, req, to) < req) {
				return aga_errno(__FILE__, "fwrite");
			}

			if(req == rem) break;
			total += req;
		}

		return AGA_RESULT_OK;
	}
# endif
}

#endif

#if defined(AGA_HAVE_SYS_STAT) && defined(AGA_HAVE_SYS_TYPES)
# define AGA_HAVE_STAT
#endif

#ifdef _WIN32
# define AGA_HAVE_STAT
# define stat _stat
# define fstat _fstat
#endif

#ifdef AGA_HAVE_STAT
static enum aga_result aga_fileattr_select_stat(
		struct stat* st, enum aga_fileattr_req attr, union aga_fileattr* out) {

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
			if(S_ISDIR(st->st_mode)) out->type = AGA_FILE_DIRECTORY;
			else out->type = AGA_FILE_REGULAR;
			break;
		}
	}

	return AGA_RESULT_OK;
}
#else
static enum aga_result aga_fileattr_type(
		const char* path, union aga_file_attr* out) {

# ifdef EISDIR
	/* Try and fall back to a solution using `fopen'. */
	FILE* f;

	if(!(f = fopen(path, "r"))) {
		if(errno == EISDIR) *isdir = AGA_TRUE;
		else return aga_errno_path(__FILE__, "fopen", path);
	}
	else *isdir = AGA_FALSE;

	if(fclose(f) == EOF) return aga_errno_path(__FILE__, "fclose", path);

	return AGA_RESULT_OK;
# else
	return AGA_ERROR_NOT_IMPLEMENTED;
# endif
}

enum aga_result aga_fileattr_length(void* fp, aga_size_t* size) {
	long off;
	long tell;

	if((off = ftell(fp)) == -1) return aga_errno(__FILE__, "ftell");

	if(fseek(fp, 0, SEEK_END) == -1) {
		return aga_errno(__FILE__, "fseek");
	}
	if((tell = ftell(fp)) == -1) return aga_errno(__FILE__, "ftell");
	*size = (aga_size_t) tell;

	if(fseek(fp, off, SEEK_SET) == -1) {
		return aga_errno(__FILE__, "fseek");
	}

	return AGA_RESULT_OK;
}

static enum aga_result aga_fileattr_select(
		void* fp, enum aga_fileattr_req attr, union aga_fileattr* out) {

	switch(attr) {
		default: return AGA_RESULT_BAD_PARAM;

		case AGA_FILE_MODIFIED: return AGA_RESULT_NOT_IMPLEMENTED;

		case AGA_FILE_LENGTH: return aga_fileattr_length(fp, &out->length);

		/* If it's a file handle, it's a regular file. */
		case AGA_FILE_TYPE: {
			out->type = AGA_REGULAR;
			break;
		}
	}

	return AGA_RESULT_OK;
}
#endif

enum aga_result aga_fileattr(
		void* fp, enum aga_fileattr_req attr, union aga_fileattr* out) {

	if(!fp) return AGA_RESULT_BAD_PARAM;
	if(!attr) return AGA_RESULT_BAD_PARAM;
	if(!out) return AGA_RESULT_BAD_PARAM;

#ifdef AGA_HAVE_STAT
	{
		struct stat st;
		int fd;

		if((fd = fileno(fp)) == -1) return aga_errno(__FILE__, "fileno");
		if(fstat(fd, &st) == -1) return aga_errno(__FILE__, "fstat");

		return aga_fileattr_select_stat(&st, attr, out);
	}
#else
	return aga_fileattr_select(fp, attr, out);
#endif
}

enum aga_result aga_fileattr_path(
		const char* path, enum aga_fileattr_req attr,
		union aga_fileattr* out) {

	if(!path) return AGA_RESULT_BAD_PARAM;
	if(!out) return AGA_RESULT_BAD_PARAM;

#ifdef AGA_HAVE_STAT
	{
		struct stat st;

		if(stat(path, &st) == -1) {
			return aga_errno_path(__FILE__, "stat", path);
		}

		return aga_fileattr_select_stat(&st, attr, out);
	}
#else
	if(attr == AGA_FILE_TYPE) return aga_fileattr_type(path, out);

	{
		enum aga_result result;

		void* fp;

		if(!(fp = fopen(path, "rb"))) {
			return aga_errno_path(__FILE__, "fopen", path);
		}

		if((result = aga_fileattr_select(fp, attr, out))) return result;

		if(fclose(fp) == EOF) return aga_errno(__FILE__, "fclose");

		return AGA_RESULT_OK;
	}
#endif
}

enum aga_result aga_fread(void* data, aga_size_t size, void* fp) {
	enum aga_result result;

	if(!data) return AGA_RESULT_BAD_PARAM;
	if(!fp) return AGA_RESULT_BAD_PARAM;

	if(fread(data, 1, size, fp) != size) {
		if(ferror(fp)) result = aga_errno(__FILE__, "fread");
		else result = AGA_RESULT_EOF;

		clearerr(fp);

		return result;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_fputn(int c, aga_size_t n, void* fp) {
	aga_size_t i;

	for(i = 0; i < n; ++i) {
		if(putc(c, fp) == EOF) return aga_errno(__FILE__, "putc");
	}

	return AGA_RESULT_OK;
}

#ifdef AGA_HAVE_MAP
# ifdef AGA_NIXMAP
enum aga_result aga_mkmapfd(void* fp, struct aga_mapfd* fd) {
	if(!fd) return AGA_RESULT_BAD_PARAM;
	if(!fp) return AGA_RESULT_BAD_PARAM;

	return AGA_RESULT_OK;
}

enum aga_result aga_killmapfd(struct aga_mapfd* fd) {
	if(!fd) return AGA_RESULT_BAD_PARAM;

	return AGA_RESULT_OK;
}

enum aga_result aga_mkfmap(
		struct aga_mapfd* fd, aga_size_t off, aga_size_t size, void** ptr) {

	if(!fd) return AGA_RESULT_BAD_PARAM;
	if(!ptr) return AGA_RESULT_BAD_PARAM;

	return AGA_RESULT_OK;
}

enum aga_result aga_killfmap(void* ptr, aga_size_t size) {
	if(!ptr) return AGA_RESULT_BAD_PARAM;

	return AGA_RESULT_OK;
}
# elif defined(AGA_WINMAP)
enum aga_result aga_mkmapfd(void* fp, struct aga_mapfd* fd) {
	int fn;
	void* hnd;
	aga_size_t size;
	SECURITY_ATTRIBUTES attrib = { sizeof(attrib), 0, FALSE };

	if(!fd) return AGA_RESULT_BAD_PARAM;
	if(!fp) return AGA_RESULT_BAD_PARAM;

	if((fn = fileno(fp)) == -1) return aga_errno(__FILE__, "fileno");
	if((hnd = (void*) _get_osfhandle(fn)) == INVALID_HANDLE_VALUE) {
		return aga_errno(__FILE__, "_get_osfhandle");
	}

	AGA_CHK(aga_fplen(fp, &size));
	AGA_VERIFY(size != 0, AGA_RESULT_BAD_PARAM);

	fd->mapping = CreateFileMappingA(hnd, &attrib, PAGE_READONLY, 0, 0, 0);
	if(!fd->mapping) return aga_win32_error(__FILE__, "CreateFileMappingA");

	return AGA_RESULT_OK;
}

enum aga_result aga_killmapfd(struct aga_mapfd* fd) {
	if(!fd) return AGA_RESULT_BAD_PARAM;

	if(!CloseHandle(fd->mapping)) {
		return aga_win32_error(__FILE__, "CloseHandle");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_mkfmap(
		struct aga_mapfd* fd, aga_size_t off, aga_size_t size, void** ptr) {

	DWORD* p = (DWORD*) &off;

	if(!fd) return AGA_RESULT_BAD_PARAM;
	if(!ptr) return AGA_RESULT_BAD_PARAM;

	/*
	 * TODO: File offset needs to be a multiple of allocation granularity.
	 * 		 This needs a bit more engineering on our part to avoid making
	 * 		 A load of gaps in the respack to satisfy this. We'll probably need
	 * 		 A registry of close addresses or get the caller to bookkeep re:
	 * 		 The offset from the returned base mapping address. We'll have to
	 * 		 See if Windows is okay with overlapping mappings under such a
	 * 		 System as we ideally want each resource entry to be able to hold
	 * 		 Its own mapping.
	 */
	*ptr = MapViewOfFile(fd->mapping, FILE_MAP_READ, p[0], p[1], size);
	if(!*ptr) return aga_win32_error(__FILE__, "MapViewOfFile");

	return AGA_RESULT_OK;
}

enum aga_result aga_killfmap(void* ptr, aga_size_t size) {
	if(!ptr) return AGA_RESULT_BAD_PARAM;

	return AGA_RESULT_OK;
}
# endif
#endif

#ifdef AGA_HAVE_SPAWN
# ifdef AGA_NIXSPAWN
/*
 * We take an unusual result value to signal to the main process that the spawn
 * failed before we hit the user program.
 */
#define AGA_SPAWN_EXIT_MAGIC (113)

enum aga_result aga_spawn_sync(const char* program, char** argv, const char* wd) {
	pid_t p;

	if(!program) return AGA_RESULT_BAD_PARAM;
	if(!argv) return AGA_RESULT_BAD_PARAM;

	if((p = fork()) == -1) return aga_errno(__FILE__, "fork");

	if(!p) {
		if(wd) {
			if(chdir(wd) == -1) {
				(void) aga_errno_path(__FILE__, "chdir", wd);
				exit(AGA_SPAWN_EXIT_MAGIC);
			}
		}
		if(execvp(program, argv) == -1) {
			(void) aga_errno_path(__FILE__, "execvp", program);
			exit(AGA_SPAWN_EXIT_MAGIC);
		}
	}
	else {
		int res;
		if(waitpid(p, &res, 0) == -1) return aga_errno(__FILE__, "wait");
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

# include <agaw32.h>

enum aga_result aga_spawn_sync(
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
		if(!cli) return aga_errno(__FILE__, "aga_realloc");

		memcpy(cli + len, *argv, l);
		cli[len + l] = ' ';
		len += l + 1;
	}

	if(cli) cli[len] = 0;

	if(!CreateProcessA(0, cli, 0, 0, FALSE, 0, 0, wd, &startup, &info)) {
		aga_free(cli);
		return aga_win32_error(__FILE__, "CreateProcessA");
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
