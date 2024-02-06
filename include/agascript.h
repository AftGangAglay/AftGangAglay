/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_H
#define AGA_SCRIPT_H

#include <agaresult.h>

#define AGA_SCRIPT_KEYMAP ("keymap")
#define AGA_SCRIPT_POINTER ("pointer")
#define AGA_SCRIPT_OPTS ("opts")
#define AGA_SCRIPT_AFCTX ("afctx")
#define AGA_SCRIPT_AFVERT ("afvert")
#define AGA_SCRIPT_SNDDEV ("snddev")
#define AGA_SCRIPT_DIE ("die")
#define AGA_SCRIPT_WINENV ("winenv")
#define AGA_SCRIPT_WIN ("win")
#define AGA_SCRIPT_PACK ("respack")

struct aga_res;
struct aga_respack;

struct aga_scriptclass {
	void* class;
};

struct aga_scriptinst {
	struct aga_scriptclass* class;
	void* object;
};

struct aga_scripteng {
	void* global;
	void* agandict;
};

enum af_err aga_mkscripteng(
		struct aga_scripteng* eng, const char* script, int argc,
		char** argv, struct aga_respack* pack, const char* pypath);
enum af_err aga_killscripteng(struct aga_scripteng* eng);

enum af_err aga_setscriptptr(
		struct aga_scripteng* eng, const char* key, void* value);

enum af_err aga_findclass(
		struct aga_scripteng* eng, struct aga_scriptclass* class,
		const char* name);
enum af_err aga_mkscriptinst(
		struct aga_scriptclass* class, struct aga_scriptinst* inst);
enum af_err aga_killscriptinst(struct aga_scriptinst* inst);
enum af_err aga_instcall(struct aga_scriptinst* inst, const char* name);

#endif
