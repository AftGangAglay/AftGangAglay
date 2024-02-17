/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaw32.h>
#include <agalog.h>

#ifdef _WIN32

#include <agastd.h>

#include <windows.h>

#ifndef _DEBUG
int WinMain(
		HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
		int nShowCmd) {

	extern int main(int, char**);

	(void) hInstance;
	(void) hPrevInstance;
	(void) lpCmdLine;
	(void) nShowCmd;

	return main(__argc, __argv);
}
#endif

enum aga_result aga_pathwinerr(
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

		if(path) aga_log(loc, "err: %s: %s `%s'", proc, buf, path);
		else aga_log(loc, "err: %s: %s", proc, buf);

		if(LocalFree(buf)) {
			aga_log(__FILE__, "err: LocalFree failed");
			return AGA_RESULT_ERROR;
		}
	}

	return AGA_RESULT_ERROR;
}

enum aga_result aga_winerr(const char* loc, const char* proc) {
	return aga_pathwinerr(loc, proc, 0);
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
		(void) aga_winerr(__FILE__, "GetStdHandle");
		return;
	}

	if(!con) return;

	if(!GetConsoleMode(con, &mode)) {
		(void) aga_winerr(__FILE__, "GetConsoleMode");
	}

	if(!SetConsoleMode(con, mode | ENABLE_VIRTUAL_TERMINAL_INPUT)) {
		aga_logctx.have_ansi = AF_FALSE;
		(void) aga_winerr(__FILE__, "SetConsoleMode");
		aga_log(__FILE__, "We don't seem to have ANSI terminal support");
	}
}

#endif
