/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/log.h>
#include <aga/environment.h>
#include <aga/utility.h>
#include <aga/std.h>
#include <aga/win32.h>

#ifndef NDEBUG
# define AGA_LOG_DEFAULT_STREAM (stderr)
#else
# define AGA_LOG_DEFAULT_STREAM (stdout)
#endif

#define AGA_ISTTY(s) ((s) == stdout || (s) == stderr)

struct aga_log_context aga_log_context;

static void aga_onabrt(int signum) {
	(void) signum;

	aga_log_delete();
}

/* TODO: MSVC version of this destructor? */
#ifdef __has_attribute
# if __has_attribute(used)
__attribute__((used))
# endif
# if __has_attribute(destructor)
__attribute__((destructor))
# endif
#endif

void aga_ondestr(void) {
	aga_log_delete();
}

static void aga_perror(const char* proc) {
	perror(proc);
}

void aga_log_new(const char** targets, aga_size_t len) {
	aga_size_t i;

	aga_log_context.have_ansi = AGA_TRUE;
	aga_log_context.len = len;

	if(setvbuf(AGA_LOG_DEFAULT_STREAM, 0, _IONBF, 0)) aga_perror("setvbuf");

	if(!(aga_log_context.targets = aga_malloc(len * sizeof(FILE*)))) {
		static FILE* so[1];
		so[0] = AGA_LOG_DEFAULT_STREAM;
		aga_perror("malloc");
		aga_log_context.targets = (void**) so;
		aga_log_context.len = 1;
	}
	else {
		for(i = 0; i < len; ++i) {
			if(!targets[i]) { aga_log_context.targets[i] = AGA_LOG_DEFAULT_STREAM; }
			else if(!(aga_log_context.targets[i] = fopen(targets[i], "w"))) {
				aga_perror("fopen");
			}
		}
	}

	if(aga_getenv("AGA_FORCEANSI")) aga_log_context.have_ansi = AGA_TRUE;
#ifdef _WIN32
	else aga_win32_log_set();
#endif

	/* TODO: Scripttrace on fatal signal? */

	signal(SIGABRT, aga_onabrt);
}

void aga_log_delete(void) {
	aga_size_t i;
	for(i = 0; i < aga_log_context.len; ++i) {
		FILE* s = aga_log_context.targets[i];
		if(!s) continue;
		if(fflush(s) == EOF) aga_perror("fflush");
		if(AGA_ISTTY(s)) continue;
		if(fclose(s) == EOF) aga_perror("fclose");
	}
}

void aga_log_header(void* s, const char* loc, enum aga_log_severity sev) {
#define ESC "\033[0"
#define CYN ESC "36m"
#define YLW ESC "33m"
#define RED ESC "31m"
#define END ESC "m"

	const char* f = "[%s] ";
	if(!s) return;

	if(aga_log_context.have_ansi && AGA_ISTTY(s)) {
		switch(sev) {
			default: break;
			case AGA_NORM: f = CYN "[%s]" END " ";
				break;
			case AGA_WARN: f = YLW "[%s]" END " ";
				break;
			case AGA_ERR: f = RED "[%s]" END " ";
				break;
		}
	}

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4774) /* Variable referenced has different types. */
#endif
	if(fprintf(s, f, loc) < 0) perror("fprintf");
#ifdef _MSC_VER
# pragma warning(pop)
#endif

#undef ESC
#undef CYN
#undef YLW
#undef RED
#undef END
}

void aga_log(const char* loc, const char* fmt, ...) {
	aga_fixed_buf_t buf = { 0 };

	aga_size_t i;

	va_list ap;
	enum aga_log_severity sev = AGA_NORM;
	va_start(ap, fmt);

	if(vsprintf(buf, fmt, ap) < 0) aga_perror("vsprintf");

	if(aga_strneql(fmt, "warn", 4)) sev = AGA_WARN;
	if(aga_strneql(fmt, "err", 3)) sev = AGA_ERR;

	for(i = 0; i < aga_log_context.len; ++i) {
		FILE* s = aga_log_context.targets[i];
		if(!s) continue;

		aga_log_header(s, loc, sev);
		if(fputs(buf, s) == EOF) aga_perror("fputs");
		if(putc('\n', s) == EOF) aga_perror("putc");
	}

	va_end(ap);
}
