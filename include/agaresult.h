/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_RESULT_H
#define AGA_RESULT_H

enum aga_result {
	AGA_RESULT_OK,
	AF_ERR_UNKNOWN,
	AF_ERR_BAD_PARAM,
	AF_ERR_BAD_OP,
	AF_ERR_MEM
};

#define AGA_CHK(c) \
	do { \
		enum af_err err = c; \
		if(err) return err; \
	} while(0)

#endif
