/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_RESULT_H
#define AGA_RESULT_H

#include <agaenv.h>

enum aga_result {
	AGA_RESULT_OK,
	AGA_RESULT_ERROR,
	AGA_RESULT_BAD_PARAM,
	AGA_RESULT_BAD_OP,
	AGA_RESULT_OOM
};

#define AGA_CHK(c) \
    do { \
        enum aga_result err = c; \
        if(err) return err; \
    } while(0)

#ifdef AGA_NO_VERIFY
# define AGA_PARAM_CHK(p)
# define AGA_VERIFY(expr, err)
#else
# define AGA_PARAM_CHK(p) if(!(p)) return AGA_RESULT_BAD_PARAM
# define AGA_VERIFY(expr, err) if(!(expr)) return (err)
#endif

#endif
