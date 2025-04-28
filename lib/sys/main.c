/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/base.h>
#include <asys/log.h>
#include <asys/error.h>
#include <asys/system.h>
#include <asys/main.h>

#ifdef ASYS_WIN32
/*
 * NOTE: The Windows 3.1 guide to programming section 14.3 states that we need
 * 		 To declare these ourselves -- this does not work under modern Windows
 * 		 As they require dllimport (depending on CRT).
 */
/*
extern int __argc;
extern char** __argv;
 */

/*
 * TODO: For some reason this doesn't link correctly under MinGW? Do we need to
 * 		 Use `--whole-archive' or reorder our links to place `WinMain' in a
 * 		 Separate library?
 * 		 For now this file just gets included into aga's build directly.
 */
int PASCAL WinMain(
		HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show) {

	enum asys_result result;

	struct asys_main_data main_data;
	WNDCLASS window_class;

	(void) command_line;

	main_data.argv = __argv;
	main_data.argc = __argc;
	main_data.show = show;
	main_data.module = instance;

	if(!previous) {
		asys_log_result(
				__FILE__, "asys_win32_register_class",
				asys_win32_register_class(&window_class, instance));

		window_class.lpszClassName = asys_global_win32_class_name;

		if(!RegisterClass(&window_class)) {
			asys_result_fatal(__FILE__, "RegisterClass", ASYS_RESULT_ERROR);
		}
	}

	asys_result_check(__FILE__, "asys_main", asys_main(&main_data));

	if(!UnregisterClass(asys_global_win32_class_name, 0)) {
		result = ASYS_RESULT_ERROR;
		asys_log_result(__FILE__, "UnregisterClass", result);
		return 1;
	}

	return 0;
}
#else
int main(int argc, char** argv) {
	struct asys_main_data main_data;

	main_data.argv = argv;
	main_data.argc = argc;

	asys_result_check(__FILE__, "asys_main", asys_main(&main_data));

	return 0;
}
#endif
