/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_GLUE_H
#define AGA_SCRIPT_GLUE_H

#include <modsupport.h>

static object* aga_script_test(object* self, object* arg) {
	printobject(self, stdout, 0);
	putchar('\n');
	printobject(arg, stdout, 0);
	putchar('\n');

	return None;
}

enum af_err aga_mkmod(void) {
	struct methodlist methods[] = {
		{ "test", aga_script_test },
		{ 0, 0 }
	};

	object* module;
	object* dict;

	module = initmodule("aga", methods);
	AF_VERIFY(module, AF_ERR_UNKNOWN);

	{
		object* str = newstringobject((char*) glGetString(GL_VERSION));
		dict = getmoduledict(module);
		INCREF(dict);

		dictinsert(dict, "foo", str);
		aga_scriptchk();
	}

	return AF_ERR_NONE;
}

enum af_err aga_killmod(void) {
	return AF_ERR_NONE;
}

#endif
