/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_ERR_H
#define AGA_ERR_H

#include <agaenv.h>
#include <agaresult.h>

/* TODO: General GL wrapper header to put this and other GL general stuff. */
/* TODO: Turn off in noverify. */
#define AGA_GL_CHK(proc) \
	do { \
		enum aga_result err = aga_glerr(__FILE__, proc); \
		if(err) return err; \
	} while(0)

AGA_NORETURN void aga_abort(void);
void aga_chk(const char* loc, const char* proc, enum aga_result e);

const char* aga_aga_errname(enum aga_result e);
/* NOTE: Pass null to `loc' to suppress error message printout. */
enum aga_result aga_errno(const char* loc, const char* proc);
enum aga_result aga_patherrno(
		const char* loc, const char* proc, const char* path);
void aga_soft(const char* loc, const char* proc, enum aga_result e);

enum aga_result aga_glerr(const char* loc, const char* proc);

#endif
