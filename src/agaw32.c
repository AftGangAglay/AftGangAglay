/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifdef _WIN32
# define AGA_WANT_WINDOWS_H
#endif

#include <agaw32.h>
#include <agalog.h>

#ifdef _WIN32

#include <agastd.h>

/* TODO: Fix subsystem setting. */
#ifndef _DEBUG
int main(int, char**);
int __stdcall WinMain(
		HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
		int nShowCmd) {

	(void) hInstance;
	(void) hPrevInstance;
	(void) lpCmdLine;
	(void) nShowCmd;

	return main(__argc, __argv);
}
#endif

enum aga_result aga_win32_error_path(
		const char* loc, const char* proc, const char* path) {

	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
				  FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD err;
	DWORD written;
	LPSTR buf;

	if(loc) {
		if(!(err = GetLastError())) return AGA_RESULT_OK;

		if(!(written = FormatMessageA(flags, 0, err, 0, (LPSTR) &buf, 0, 0))) {
			aga_log(__FILE__, "err: FormatMessageA failed");
			return AGA_RESULT_ERROR;
		}

		/* Remove tailing \r\n. */
		buf[written - 1] = 0;
		buf[written - 2] = 0;

		if(path) { aga_log(loc, "err: %s: %s `%s'", proc, buf, path); }
		else { aga_log(loc, "err: %s: %s", proc, buf); }

		if(LocalFree(buf)) {
			aga_log(__FILE__, "err: LocalFree failed");
			return AGA_RESULT_ERROR;
		}
	}

	return AGA_RESULT_ERROR;
}

enum aga_result aga_win32_error(const char* loc, const char* proc) {
	return aga_win32_error_path(loc, proc, 0);
}

void aga_setw32log(void) {
#ifdef _DEBUG
	DWORD handle = STD_ERROR_HANDLE;
#else
	DWORD handle = STD_INPUT_HANDLE;
#endif

	HANDLE con = GetStdHandle(handle);
	DWORD mode = 0;

	if(con == INVALID_HANDLE_VALUE) {
		(void) aga_win32_error(__FILE__, "GetStdHandle");
		return;
	}

	if(!con) return;

	if(!GetConsoleMode(con, &mode)) {
		(void) aga_win32_error(__FILE__, "GetConsoleMode");
	}

	if(!SetConsoleMode(con, mode | ENABLE_VIRTUAL_TERMINAL_INPUT)) {
		aga_logctx.have_ansi = AGA_FALSE;
		(void) aga_win32_error(__FILE__, "SetConsoleMode");
		aga_log(__FILE__, "We don't seem to have ANSI terminal support");
	}
}

#endif
