/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_RESULT_H
#define AGA_RESULT_H

enum aga_result {
	AGA_RESULT_OK,
	AGA_RESULT_ERROR,
	AGA_RESULT_EOF,
	AGA_RESULT_BAD_PARAM,
	AGA_RESULT_BAD_OP,
	AGA_RESULT_OOM,
	AGA_RESULT_NOT_IMPLEMENTED,
	AGA_RESULT_MISSING_KEY,
	AGA_RESULT_BAD_TYPE
};

#endif
