/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_H
#define AGA_SCRIPT_H

#include <afeirsa/afeirsa.h>

struct aga_ctx;

struct aga_scriptmethod {
	void* method;
	char* name;
};

struct aga_scriptclass {
	void* class;
	char* name;
};

struct aga_scriptinst {
	struct aga_scriptclass* class;
	void* object;
};

struct aga_scripteng {
	void* global;
	struct aga_scriptclass* classes;
	af_size_t len;
};

enum af_err aga_mkscripteng(
		struct aga_ctx* ctx, struct aga_scripteng* scripteng,
		const char* script, const char* pypath, int argc, char** argv);

enum af_err aga_killscripteng(struct aga_scripteng* eng);

enum af_err aga_findclass(
		struct aga_scripteng* eng, struct aga_scriptclass** class,
		const char* name);

enum af_err aga_mkscriptinst(
		struct aga_scriptclass* class, struct aga_scriptinst* inst);

enum af_err aga_killscriptinst(struct aga_scriptinst* inst);
enum af_err aga_instcall(struct aga_scriptinst* inst, const char* name);

#endif
