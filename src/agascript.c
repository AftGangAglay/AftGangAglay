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

#ifdef _DEBUG
/* The extra debug info this enables is a bit too verbose. */
int debugging = 0;
#endif

enum af_err aga_mkscripteng(
		struct aga_scripteng* eng, const char* script, const char* pypath) {

	AF_PARAM_CHK(eng);
	AF_PARAM_CHK(script);
	AF_PARAM_CHK(pypath);

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

		code = compile(node, (char*) script);
		freetree(node);
		AF_VERIFY(code, AF_ERR_UNKNOWN);

		result = eval_code(code, dict, dict, 0);
		DECREF(code);
		flushline();

		{
			object* keys;
			int len;
			int i;

			AF_VERIFY(keys = getdictkeys(dict), AF_ERR_UNKNOWN);
			len = getlistsize(keys);

			/*
			 * NOTE: This is slightly wasteful but avoids a load of `realloc's.
			 *		 Most scripts are probably pretty light on non-class
			 *		 globals anyway.
			 */
			eng->len = (af_size_t) len;
			if(!(eng->classes = malloc(len * sizeof(object*)))) {
				aga_errno_chk("malloc");
			}

			for(i = 0; i < len; ++i) {
				object* key;
				object* value;
				char* key_name;

				AF_VERIFY(key = getlistitem(keys, i), AF_ERR_UNKNOWN);
				AF_VERIFY(key_name = getstringvalue(key), AF_ERR_UNKNOWN);

				AF_VERIFY(value = dictlookup(dict, key_name), AF_ERR_UNKNOWN);

				if(is_classobject(value)) eng->classes[i] = value;
			}
		}

		AF_VERIFY(result, AF_ERR_UNKNOWN);
		DECREF(result);

		if(fclose(f) == EOF) aga_errno_chk("fclose");
	}

	return AF_ERR_NONE;
}

enum af_err aga_killscripteng(struct aga_scripteng* eng) {
	AF_PARAM_CHK(eng);

	free(eng->classes);

	flushline();
	doneimport();

	err_clear();

	return AF_ERR_NONE;
}
