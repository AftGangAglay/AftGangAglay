/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_ERROR_H
#define AGA_ERROR_H

#include <aga/environment.h>
#include <aga/result.h>

AGA_NORETURN void aga_error_abort(void);

void aga_error_check(const char*, const char*, enum aga_result);
void aga_error_check_soft(const char*, const char*, enum aga_result);

/* NOTE: Pass null to `loc' to suppress error message printout. */
enum aga_result aga_error_system(const char*, const char*);
enum aga_result aga_error_system_path(const char*, const char*, const char*);

enum aga_result aga_win32_error(const char*, const char*);
enum aga_result aga_win32_error_path(const char*, const char*, const char*);

enum aga_result aga_error_gl(const char*, const char*);

#endif
