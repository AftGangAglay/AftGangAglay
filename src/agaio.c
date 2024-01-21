/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaio.h>
#include <agaerr.h>
#include <agalog.h>
#define AGA_WANT_UNIX
#include <agastd.h>

enum af_err aga_open(const char* path, void** fp, af_size_t* size) {
	long tell;

	AF_PARAM_CHK(path);
	AF_PARAM_CHK(fp);
	AF_PARAM_CHK(size);

	if(!(*fp = fopen(path, "rb"))) {
		return aga_af_patherrno(__FILE__, "fopen", path);
	}

	if(fseek(*fp, 0, SEEK_END) == -1) return aga_af_errno(__FILE__, "fseek");

	if((tell = ftell(*fp)) == -1) return aga_af_errno(__FILE__, "ftell");
	*size = (af_size_t) tell;

	rewind(*fp);

	return AF_ERR_NONE;
}

#ifdef AGA_HAVE_SPAWN
# ifdef AGA_NIXSPAWN
/*
 * We take an unusual result value to signal to the main process that the spawn
 * failed before we hit the user program.
 */
#define AGA_SPAWN_EXIT_MAGIC (113)

enum af_err aga_spawn_sync(const char* program, char** argv, const char* wd) {
	pid_t p;

	AF_PARAM_CHK(program);
	AF_PARAM_CHK(argv);

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
# elif defined(AGA_WINSPAWN)
# include <agaw32.h>

# include <windows.h>

enum af_err aga_spawn_sync(const char* program, char** argv, const char* wd) {
	af_size_t len = 0;
	char* cli = 0;

	STARTUPINFOA startup = { 0 };
	PROCESS_INFORMATION info = { 0 };
	startup.cb = sizeof(startup);

	AF_PARAM_CHK(program);
	AF_PARAM_CHK(argv);

	for(; *argv; ++argv) {
		af_size_t l = af_strlen(*argv);
		char* tmp = realloc(cli, len + l + 2);
		if(!tmp) {
			free(cli);
			return aga_af_errno(__FILE__, "realloc");
		}
		cli = tmp;
		af_memcpy(cli + len, *argv, l);
		cli[len + l] = ' ';
		len += l + 1;
	}
	cli[len] = 0;

	if(!CreateProcessA(0, cli, 0, 0, FALSE, 0, 0, wd, &startup, &info)) {
		free(cli);
		return aga_af_winerr(__FILE__, "CreateProcessA");
	}

	free(cli);

	if(WaitForSingleObject(info.hProcess, INFINITE) == WAIT_FAILED) {
		return aga_af_winerr(__FILE__, "WaitForSingleObject");
	}

	if(!CloseHandle(info.hProcess)) {
		return aga_af_winerr(__FILE__, "CloseHandle");
	}
	if(!CloseHandle(info.hThread)) {
		return aga_af_winerr(__FILE__, "CloseHandle");
	}

	return AF_ERR_NONE;
}
# endif
#endif
