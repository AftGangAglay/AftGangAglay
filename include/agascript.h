/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_H
#define AGA_SCRIPT_H

#include <afeirsa/aftypes.h>
#include <afeirsa/aferr.h>

struct aga_scriptmethod {
	void* method;
	char* name;
};

struct aga_scriptclass {
	void* class;
	char* name;

	struct aga_scriptmethod* methods;
	af_size_t len;
};

struct aga_scriptinst {
	void* object;
};

struct aga_scripteng {
	struct aga_scriptclass* classes;
	af_size_t len;
};

enum af_err aga_mkscripteng(
		struct aga_scripteng* eng, const char* script, const char* pypath);

enum af_err aga_killscripteng(struct aga_scripteng* eng);

enum af_err aga_mkscriptinst(
		struct aga_scriptclass* class, struct aga_scriptinst* inst);

enum af_err aga_killscriptinst(struct aga_scriptinst* inst);

#endif
