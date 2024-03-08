/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_ERR_H
#define AGA_ERR_H

#include <agaenv.h>
#include <agaresult.h>

AGA_NORETURN void aga_abort(void);

const char* aga_result_name(enum aga_result);

void aga_check(const char*, const char*, enum aga_result);
void aga_soft(const char*, const char*, enum aga_result);

/* NOTE: Pass null to `loc' to suppress error message printout. */
enum aga_result aga_errno(const char*, const char*);
enum aga_result aga_errno_path(const char*, const char*, const char*);

#endif
