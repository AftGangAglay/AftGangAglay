/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_ERR_H
#define AGA_ERR_H

#include <agaenv.h>

#include <afeirsa/afeirsa.h>

AGA_NORETURN void aga_abort(void);
void aga_af_chk(const char* loc, const char* proc, enum af_err e);

const char* aga_af_errname(enum af_err e);
/* NOTE: Pass null to `proc' to suppress error message printout. */
enum af_err aga_af_errno(const char* loc, const char* proc);
enum af_err aga_af_patherrno(
		const char* loc, const char* proc, const char* path);
void aga_af_soft(const char* loc, const char* proc, enum af_err e);

#endif
