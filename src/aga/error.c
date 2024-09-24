/*
* SPDX-License-Identifier: GPL-3.0-or-later
* Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
*/

#include <aga/error.h>
#include <aga/std.h>
#include <aga/log.h>
#include <aga/window.h>
#include <aga/gl.h>

AGA_NORETURN void aga_error_abort(void) {
#ifdef NDEBUG
	static const char msg[] =
		"AftGangAglay has encountered a fatal error and cannot continue.\n"
		"Would you like to report the issue?\n"
		"Please include the file `aga.log' along with your report.";
	static const char report_uri[] =
		"https://github.com/AftGangAglay/AftGangAglay/issues/new";

	aga_bool_t res;
	(void) aga_dialog(msg, "Fatal Error", &res, AGA_TRUE);

	if(res) (void) aga_shell_open(report_uri);

	exit(EXIT_FAILURE);
#else
# if !defined(AGA_ABORT_DONE) && defined(_MSC_VER)
#  define AGA_ABORT_DONE
	__debugbreak();
# endif

# if !defined(AGA_ABORT_DONE) && defined(__has_builtin)
#  if __has_builtin(__builtin_trap)
#   define AGA_ABORT_DONE
	__builtin_trap();
#  endif
# endif

# if !defined(AGA_ABORT_DONE)
	abort();
# endif
#endif
}

void aga_error_check(const char* loc, const char* proc, enum aga_result e) {
	if(!e) return;

	aga_error_check_soft(loc, proc, e);
	aga_error_abort();
}

void aga_error_check_soft(
		const char* loc, const char* proc, enum aga_result e) {

	if(!e) return;

	aga_log(loc, "err: %s: %s", proc, aga_result_name(e));
}

enum aga_result aga_error_system(const char* loc, const char* proc) {
	return aga_error_system_path(loc, proc, 0);
}

enum aga_result aga_error_system_path(
		const char* loc, const char* proc, const char* path) {

	if(loc) {
		if(path) aga_log(loc, "err: %s: %s `%s'", proc, strerror(errno), path);
		else aga_log(loc, "err: %s: %s", proc, strerror(errno));
	}

	switch(errno) {
		default: return AGA_RESULT_ERROR;
		case 0: return AGA_RESULT_OK;

# ifdef EBADF
		case EBADF: return AGA_RESULT_BAD_PARAM;
# endif
# ifdef ENOMEM
		case ENOMEM: return AGA_RESULT_OOM;
# endif
# ifdef EACCES
		case EACCES: return AGA_RESULT_BAD_OP;
# endif
# ifdef EOPNOTSUPP
		case EOPNOTSUPP: return AGA_RESULT_NOT_IMPLEMENTED;
# endif
	}
}

static enum aga_result aga_gl_result(aga_uint_t err) {
	switch(err) {
		default: return AGA_RESULT_ERROR;
		case GL_INVALID_ENUM: return AGA_RESULT_BAD_TYPE;
		case GL_INVALID_VALUE: return AGA_RESULT_BAD_PARAM;
		case GL_INVALID_OPERATION: return AGA_RESULT_BAD_OP;
		case GL_OUT_OF_MEMORY: return AGA_RESULT_OOM;
		case GL_STACK_UNDERFLOW: return AGA_RESULT_OOM;
		case GL_STACK_OVERFLOW: return AGA_RESULT_OOM;
	}
}

#ifdef _WIN32
# define AGA_WANT_WINDOWS_H
# include <aga/win32.h>

enum aga_result aga_win32_error_path(
		const char* loc, const char* proc, const char* path) {

	static const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD err;
	DWORD written;
	LPSTR buf;

	if(loc) {
		if(!(err = GetLastError())) return AGA_RESULT_OK;

		if(!(written = FormatMessage(flags, 0, err, 0, (LPSTR) &buf, 0, 0))) {
			aga_log(__FILE__, "err: FormatMessage failed");
			return AGA_RESULT_ERROR;
		}

		/* Remove tailing \r\n. */
		buf[written - 1] = 0;
		buf[written - 2] = 0;

		if(path) aga_log(loc, "err: %s: %s `%s'", proc, buf, path);
		else aga_log(loc, "err: %s: %s", proc, buf);

		if(LocalFree(buf)) {
			aga_log(__FILE__, "err: LocalFree failed");
			return AGA_RESULT_ERROR;
		}
	}

	return AGA_RESULT_ERROR;
}

enum aga_result aga_win32_error(const char* loc, const char* proc) {
	return aga_win32_error_path(loc, proc, 0);
}

#endif

enum aga_result aga_error_gl(const char* loc, const char* proc) {
	enum aga_result err = AGA_RESULT_OK;

	unsigned res;

	while((res = glGetError())) {
		err = aga_gl_result(res);
		if(loc) { /* Null `loc' acts to clear the GL error state. */
			const char* str = (const char*) gluErrorString(res);
			if(!str) str = "unknown error";
			aga_log(loc, "err: %s: %s", proc, str);
		}
	}

	return err;
}
