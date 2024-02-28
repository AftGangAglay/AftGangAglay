/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaerr.h>
#include <agastd.h>
#include <agalog.h>
#include <agawin.h>

const char* aga_aga_errname(enum aga_result e) {
	switch(e) {
		default: {
			AGA_FALLTHROUGH;
			/* FALLTHRU */
		}
		case AGA_RESULT_OK: return "none";

		case AGA_RESULT_ERROR: return "unknown";
		case AGA_RESULT_BAD_PARAM: return "bad parameter";
		case AGA_RESULT_BAD_OP: return "bad operation";
		case AGA_RESULT_OOM: return "out of memory";
	}

	return "none";
}

AGA_NORETURN void aga_abort(void) {
#ifdef NDEBUG
	static const char msg[] =
		"AftGangAglay has encountered a fatal error and cannot continue.\n"
		"Would you like to report the issue?\n"
		"Please include the file `aga.log' along with your report.";
	static const char report_uri[] =
		"https://github.com/AftGangAglay/AftGangAglay/issues/new";

	aga_bool_t res;
	(void) aga_diag(msg, "Fatal Error", &res, AGA_TRUE);

	if(res) (void) aga_shellopen(report_uri);
#endif

#ifdef _DEBUG
# ifdef _WIN32
	__debugbreak();
	AGA_UNREACHABLE;
# else
	abort();
# endif
#else
	exit(EXIT_FAILURE);
#endif
}

void aga_chk(const char* loc, const char* proc, enum aga_result e) {
	if(!e) return;
	aga_soft(loc, proc, e);
	aga_abort();
}

void aga_soft(const char* loc, const char* proc, enum aga_result e) {
	aga_log(loc, "err: %s: %s", proc, aga_aga_errname(e));
}

enum aga_result aga_errno(const char* loc, const char* proc) {
	return aga_patherrno(loc, proc, 0);
}

enum aga_result aga_patherrno(
		const char* loc, const char* proc, const char* path) {

	if(loc) {
		if(path) {
			aga_log(
					loc, "err: %s: %s `%s'", proc, strerror(errno), path);
		}
		else { aga_log(loc, "err: %s: %s", proc, strerror(errno)); }
	}
	switch(errno) {
		default: return AGA_RESULT_ERROR;
		case 0: return AGA_RESULT_OK;

#ifdef EBADF
		case EBADF: return AGA_RESULT_BAD_PARAM;
#endif
#ifdef ENOMEM
		case ENOMEM: return AGA_RESULT_OOM;
#endif
#ifdef EACCES
		case EACCES: return AGA_RESULT_BAD_OP;
#endif
#ifdef EOPNOTSUPP
		case EOPNOTSUPP: return AGA_RESULT_BAD_OP;
#endif
	}
}
