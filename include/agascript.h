/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_H
#define AGA_SCRIPT_H

#include <agaresult.h>

#define AGA_SCRIPT_KEYMAP ("keymap")
#define AGA_SCRIPT_POINTER ("pointer")
#define AGA_SCRIPT_OPTS ("opts")
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

enum aga_result aga_mkscripteng(
		struct aga_scripteng* eng, const char* script, int argc,
		char** argv, struct aga_respack* pack, const char* pypath);
enum aga_result aga_killscripteng(struct aga_scripteng* eng);

enum aga_result aga_setscriptptr(
		struct aga_scripteng* eng, const char* key, void* value);
void* aga_getscriptptr(const char* key);

enum aga_result aga_findclass(
		struct aga_scripteng* eng, struct aga_scriptclass* class,
		const char* name);
enum aga_result aga_mkscriptinst(
		struct aga_scriptclass* class, struct aga_scriptinst* inst);
enum aga_result aga_killscriptinst(struct aga_scriptinst* inst);
enum aga_result aga_instcall(struct aga_scriptinst* inst, const char* name);

void aga_script_trace(void);
aga_bool_t aga_script_err(const char* proc, enum aga_result err);
aga_bool_t aga_script_glerr(const char* proc);

enum aga_result aga_mkmod(void** dict);

#endif
