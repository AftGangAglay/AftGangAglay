/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaerr.h>
#include <agastd.h>
#include <agalog.h>
#include <agawin.h>

const char* aga_af_errname(enum af_err e) {
	switch(e) {
		default:;
			AF_FALLTHROUGH;
			/* FALLTHRU */
		case AF_ERR_NONE: return "none";

		case AF_ERR_UNKNOWN: return "unknown";
		case AF_ERR_BAD_PARAM: return "bad parameter";
		case AF_ERR_BAD_CTX: return "bad context";
		case AF_ERR_BAD_OP: return "bad operation";
		case AF_ERR_NO_GL: return "no opengl";
		case AF_ERR_MEM: return "out of memory";
	}

	return "none";
}

AGA_NORETURN void aga_abort(void) {
	static const char msg[] =
		"AftGangAglay has encountered a fatal error and cannot continue.\n"
		"Would you like to report the issue?\n"
		"Please include the file `aga.log' along with your report.";
	static const char report_uri[] =
		"https://github.com/AftGangAglay/AftGangAglay/issues/new";

	af_bool_t res;
	(void) aga_diag(msg, "Fatal Error", &res, AF_TRUE);

	if(res) (void) aga_shellopen(report_uri);

#ifdef _DEBUG
# ifdef _WINDOWS
	__debugbreak();
	AGA_UNREACHABLE;
# else
	abort();
# endif
#else
	exit(EXIT_FAILURE);
#endif
}

void aga_af_chk(const char* loc, const char* proc, enum af_err e) {
	if(!e) return;
	aga_af_soft(loc, proc, e);
	aga_abort();
}

void aga_af_soft(const char* loc, const char* proc, enum af_err e) {
	aga_log(loc, "err: %s: %s", proc, aga_af_errname(e));
}

enum af_err aga_af_errno(const char* loc, const char* proc) {
	return aga_af_patherrno(loc, proc, 0);
}

enum af_err aga_af_patherrno(
		const char* loc, const char* proc, const char* path) {

	if(loc) {
		if(path) aga_log(loc, "err: %s: %s `%s'", proc, strerror(errno), path);
		else aga_log(loc, "err: %s: %s", proc, strerror(errno));
	}
	switch(errno) {
		default: return AF_ERR_UNKNOWN;
		case 0: return AF_ERR_NONE;

#ifdef EBADF
		case EBADF: return AF_ERR_BAD_PARAM;
#endif
#ifdef ENOMEM
		case ENOMEM: return AF_ERR_MEM;
#endif
#ifdef EACCES
		case EACCES:
			AF_FALLTHROUGH;
			/* FALLTHRU */
#endif
#ifdef EOPNOTSUPP
		case EOPNOTSUPP: return AF_ERR_BAD_OP;
#endif
	}
}
