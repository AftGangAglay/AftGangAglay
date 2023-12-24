/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaw32.h>
#include <agalog.h>

#ifdef _WINDOWS

#include <windows.h>

enum af_err aga_af_pathwinerr(
		const char* loc, const char* proc, const char* path) {

	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD err;
	DWORD written;
	LPSTR buf;

	if(loc) {
		if(!(err = GetLastError())) return AF_ERR_NONE;

		if(!(written = FormatMessageA(flags, 0, err, 0, (LPSTR) &buf, 0, 0))) {
			aga_log(__FILE__, "err: FormatMessageA failed");
			return AF_ERR_UNKNOWN;
		}

		/* Remove tailing \r\n. */
		buf[written - 1] = 0;
		buf[written - 2] = 0;

		if(path) aga_log(loc, "err: %s: %s `%s'", proc, buf, path);
		else aga_log(loc, "err: %s: %s", proc, buf);

		if(LocalFree(buf)) {
			aga_log(__FILE__, "err: LocalFree failed");
			return AF_ERR_UNKNOWN;
		}
	}

	return AF_ERR_UNKNOWN;
}

enum af_err aga_af_winerr(const char* loc, const char* proc) {
	return aga_af_pathwinerr(loc, proc, 0);
}

void aga_setw32log(void) {
#ifdef _DEBUG
	DWORD handle = STD_ERROR_HANDLE;
#else
	DWORD handle = STD_INPUT_HANDLE;
#endif

	HANDLE con = GetStdHandle(handle);
	DWORD mode;

	if(!GetConsoleMode(con, &mode)) {
		aga_logctx.have_ansi = AF_FALSE;
		(void) aga_af_winerr(__FILE__, "GetConsoleMode");
		aga_log(__FILE__, "We don't seem to have ANSI terminal support");
	}

	if(!SetConsoleMode(con, mode | ENABLE_VIRTUAL_TERMINAL_INPUT)) {
		aga_logctx.have_ansi = AF_FALSE;
		(void) aga_af_winerr(__FILE__, "SetConsoleMode");
		aga_log(__FILE__, "We don't seem to have ANSI terminal support");
	}
}

#endif
