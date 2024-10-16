/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/log.h>
#include <aga/std.h>
#include <aga/error.h>

#ifdef _WIN32
# define AGA_WANT_WINDOWS_H
# include <aga/win32.h>

/* TODO: Fix subsystem setting. */
# ifdef NDEBUG
int main(int, char**);
/*
 * TODO: This isn't very 1992 friendly -- we should handle our "main" function
 * 		 In an agnostic way and call it appropriately from here or std main.
 * 		 This also lets us register our class properly.
 */
int __stdcall WinMain(
		HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
		int nShowCmd) {

	(void) hInstance;
	(void) hPrevInstance;
	(void) lpCmdLine;
	(void) nShowCmd;

	return main(__argc, __argv);
}
# endif

void aga_win32_log_set(void) {
# ifndef NDEBUG
	DWORD handle = STD_ERROR_HANDLE;
# else
	DWORD handle = STD_OUTPUT_HANDLE;
# endif

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
		aga_log_context.have_ansi = AGA_FALSE;
		(void) aga_win32_error(__FILE__, "SetConsoleMode");
		aga_log(__FILE__, "We don't seem to have ANSI terminal support");
	}
}

#endif
