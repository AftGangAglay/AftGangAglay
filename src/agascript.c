/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <afeirsa/afeirsa.h>

#include <agacore.h>
#include <agascript.h>

/* NOTE: This is cursed beyond cursed but old C code do be like that. */
#define main __attribute__((weak)) main
#include <pythonmain.c>
#include <config.c>
#undef main

int setpythonpath(char* path);
int setpythonargv(int argc, char** argv);
int flushline(void);

enum af_err aga_test_script(const char* script, const char* pypath) {
	AF_PARAM_CHK(script);

	initall();

	setpythonpath((char*) pypath); /* libpython predates `const' */
	setpythonargv(0, 0);

	{
		FILE* f;

		object* module;
		object* dict;
		object* result;
		node* node;
		int err;

		codeobject* code;

		if(!(f = fopen(script, "r"))) aga_errno_chk("fopen");

		module = add_module("__main__");
		AF_VERIFY(module, AF_ERR_UNKNOWN);

		dict = getmoduledict(module);
		err = parsefile(f, (char*) script, &gram, file_input, 0, 0, &node);

		AF_VERIFY(err == E_DONE, AF_ERR_UNKNOWN);

		AF_VERIFY(dict, AF_ERR_UNKNOWN);
		AF_VERIFY(dict, AF_ERR_UNKNOWN);

		code = compile(node, (char*) script);
		freetree(node);
		AF_VERIFY(code, AF_ERR_UNKNOWN);

		result = eval_code(code, dict, dict, 0);
		DECREF(code);

		AF_VERIFY(result, AF_ERR_UNKNOWN);
		DECREF(result);
	}

	flushline();
	doneimport();

	err_clear();

	return AF_ERR_NONE;
}
