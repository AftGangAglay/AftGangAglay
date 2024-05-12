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

enum aga_result aga_fplen(void* fp, aga_size_t* size) {
	if(!fp) return AGA_RESULT_BAD_PARAM;
	if(!size) return AGA_RESULT_BAD_PARAM;

#if defined(AGA_HAVE_SYS_STAT) && defined(AGA_HAVE_SYS_TYPES)
	{
		struct stat st;
		int fd;
		if((fd = fileno(fp)) == -1) return aga_errno(__FILE__, "fileno");
		if(fstat(fd, &st) == -1) return aga_errno(__FILE__, "fstat");
		*size = st.st_size;
	}
#else
	{
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
	}
#endif

	return AGA_RESULT_OK;
}

enum aga_result aga_fread(void* data, aga_size_t size, void* fp) {
	if(!data) return AGA_RESULT_BAD_PARAM;
	if(!fp) return AGA_RESULT_BAD_PARAM;

	if(fread(data, 1, size, fp) != size) {
		if(ferror(fp)) return aga_errno(__FILE__, "fread");
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
#  define AGA_WANT_WINDOWS_H
#  include <agaw32.h>

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
