/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef ASYS_MAIN_H
#define ASYS_MAIN_H

#include <asys/base.h>
#include <asys/result.h>

struct asys_main_data {
#ifdef ASYS_WIN32
	void* module;
	void* window_class;
	int show;
#endif

	int argc;
	char** argv;
};

#ifdef ASYS_WIN32
extern const char* const asys_global_win32_class_name;

enum asys_result asys_win32_register_class(void*, void*);
#endif

enum asys_result asys_main(struct asys_main_data*);

#endif
