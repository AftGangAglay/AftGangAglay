/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/result.h>
#include <aga/environment.h>

const char* aga_result_name(enum aga_result e) {
	switch(e) {
		default:; AGA_FALLTHROUGH;
			/* FALLTHROUGH */
		case AGA_RESULT_OK: return "none";

		case AGA_RESULT_ERROR: return "unknown";
		case AGA_RESULT_EOF: return "end of file";
		case AGA_RESULT_BAD_PARAM: return "bad parameter";
		case AGA_RESULT_BAD_OP: return "bad operation";
		case AGA_RESULT_OOM: return "out of memory";
		case AGA_RESULT_NOT_IMPLEMENTED: return "not implemented";
		case AGA_RESULT_MISSING_KEY: return "missing key";
		case AGA_RESULT_BAD_TYPE: return "bad type";
	}
}
