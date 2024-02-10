/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agautil.h>
#include <agastd.h>

aga_bool_t aga_streql(const char* a, const char* b) {
    return !strcmp(a, b);
}
