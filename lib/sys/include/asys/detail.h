/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+asys@pm.me>
 */

#ifndef ASYS_DETAIL_H
#define ASYS_DETAIL_H

#include <asys/base.h>
#include <asys/result.h>

#ifdef ASYS_UNIX
# include <asys/filedata.h>

struct stat;

enum asys_result asys_file_attribute_stat(
		struct stat*, enum asys_file_attribute_field,
		union asys_file_attribute*);
#endif

#endif
