/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_H
#define AGA_SCRIPT_H

#include <agaenv.h>
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
		struct aga_scripteng*, const char*, struct aga_respack*, const char*);

enum aga_result aga_killscripteng(struct aga_scripteng*);

enum aga_result aga_setscriptptr(struct aga_scripteng*, const char*, void*);

void* aga_getscriptptr(const char*);

enum aga_result aga_findclass(
		struct aga_scripteng*, struct aga_scriptclass*, const char*);

enum aga_result aga_mkscriptinst(
		struct aga_scriptclass*, struct aga_scriptinst*);

enum aga_result aga_killscriptinst(struct aga_scriptinst*);

enum aga_result aga_instcall(struct aga_scriptinst*, const char*);

void aga_script_trace(void);

aga_bool_t aga_script_err(const char*, enum aga_result);

aga_bool_t aga_script_gl_err(const char*);

enum aga_result aga_mkmod(void**);

#endif
