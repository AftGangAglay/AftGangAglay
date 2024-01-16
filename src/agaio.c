/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaio.h>
#include <agaerr.h>
#include <agalog.h>
#define AGA_WANT_UNIX
#include <agastd.h>

enum af_err aga_read(const char* path, void** ptr, af_size_t* size) {
	FILE* f;
	long off;

	AF_PARAM_CHK(path);
	AF_PARAM_CHK(ptr);

	if(!(f = fopen(path, "rb"))) {
		return aga_af_patherrno(__FILE__, "fopen", path);
	}

	if(fseek(f, 0, SEEK_END) == -1) return aga_af_errno(__FILE__, "fseek");
	if((off = ftell(f)) == -1) return aga_af_errno(__FILE__, "ftell");

	rewind(f);

	AF_VERIFY(*ptr = malloc((af_size_t) off), AF_ERR_MEM);
	if((*size = fread(*ptr, sizeof(af_uchar_t), off, f)) != (af_size_t) off) {
		if(ferror(f)) return aga_af_patherrno(__FILE__, "fread", path);
		if(feof(f)) aga_log(__FILE__, "warn: EOF during read of `%s'", path);
	}
	if(fclose(f) == EOF) return aga_af_errno(__FILE__, "fclose");

	return AF_ERR_NONE;
}

#ifdef AGA_HAVE_SPAWN
/*
 * We take an unusual result value to signal to the main process that the spawn
 * failed before we hit the user program.
 */
#define AGA_SPAWN_EXIT_MAGIC (113)

enum af_err aga_spawn_sync(const char* program, char** argv, const char* wd) {
	pid_t p;

	if((p = fork()) == -1) return aga_af_errno(__FILE__, "fork");

	if(!p) {
		if(wd) {
			if(chdir(wd) == -1) {
				(void) aga_af_patherrno(__FILE__, "chdir", wd);
				exit(AGA_SPAWN_EXIT_MAGIC);
			}
		}
		if(execvp(program, argv) == -1) {
			(void) aga_af_patherrno(__FILE__, "execvp", program);
			exit(AGA_SPAWN_EXIT_MAGIC);
		}
	}
	else {
		int res;
		if(waitpid(p, &res, 0) == -1) return aga_af_errno(__FILE__, "wait");
		if((res = WEXITSTATUS(res))) {
			if(res == AGA_SPAWN_EXIT_MAGIC) {
				aga_log(__FILE__, "err: failed to start `%s'", program);
			}
			else {
				aga_log(
					__FILE__, "err: `%s' exited with exit code %i",
					program, res);
			}

			return AF_ERR_UNKNOWN;
		}
		else aga_log(__FILE__, "`%s' exited with exit code 0", program);
	}

	return AF_ERR_NONE;
}
#endif

#ifdef AGA_HAVE_MAP
enum af_err aga_mkfmap(const char* path, void** ptr, af_size_t* size) {
	struct stat statbuf;
	int fd;

	AF_PARAM_CHK(path);
	AF_PARAM_CHK(ptr);

	if((fd = open(path, O_RDONLY)) == -1) {
		return aga_af_patherrno(__FILE__, "open", path);
	}
	if(fstat(fd, &statbuf) == -1) {
		return aga_af_patherrno(__FILE__, "stat", path);
	}
	*size = statbuf.st_size;

	*ptr = mmap(0, *size, PROT_READ, MAP_PRIVATE, fd, 0);
	if(*ptr == (void*) -1) {
		return aga_af_patherrno(__FILE__, "mmap", path);
	}

	if(close(fd) == -1) {
		return aga_af_patherrno(__FILE__, "close", path);
	}

	return AF_ERR_NONE;
}

enum af_err aga_killfmap(void* ptr, af_size_t size) {
	AF_PARAM_CHK(ptr);

	if(munmap(ptr, size) == -1) return aga_af_errno(__FILE__, "munmap");

	return AF_ERR_NONE;
}
#endif

enum af_err aga_mklargefile(
		const char* path, void** ptr, af_size_t* size) {

#ifdef AGA_HAVE_MAP
	return aga_mkfmap(path, ptr, size);
#else
	return aga_read(path, ptr, size);
#endif
}
enum af_err aga_killlargefile(void* ptr, af_size_t size) {
#ifdef AGA_HAVE_MAP
	return aga_killfmap(ptr, size);
#else
	free(ptr);
	(void) size;
	return AF_ERR_NONE;
#endif
}
