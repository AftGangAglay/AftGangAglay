/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_RESGEN_H
#define AGA_RESGEN_H

#include <agaresult.h>

enum aga_result aga_resgen_img(const char*);
enum aga_result aga_resgen_model(const char*);
enum aga_result aga_resgen_pack(const char*);
enum aga_result aga_resgen_snd(const char*);

#endif
