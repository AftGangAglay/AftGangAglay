/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agalog.h>

#define AGA_WANT_UNIX
#include <agastd.h>
#undef AGA_WANT_UNIX

struct aga_logctx aga_logctx;

static void aga_onabrt(int signum) {
	(void) signum;

	aga_killlog();
}

static AGA_NORETURN void aga_logdierr(const char* proc) {
	perror(proc);
	exit(EXIT_FAILURE);
}

enum af_err aga_mklog(const char** targets, af_size_t len) {
	af_size_t i;

	AF_PARAM_CHK(targets);

	aga_logctx.len = len;

	if(!(aga_logctx.targets = malloc(len * sizeof(FILE*)))) return AF_ERR_MEM;

	for(i = 0; i < len; ++i) {
		if(!(aga_logctx.targets[i] = fopen(targets[i], "w+"))) {
			aga_logdierr("fopen");
		}
	}

	signal(SIGABRT, aga_onabrt);

	return AF_ERR_NONE;
}

enum af_err aga_killlog(void) {
	af_size_t i;
	for(i = 0; i < aga_logctx.len; ++i) {
		FILE* s = aga_logctx.targets[i];
		if(fileno(s) <= 2) continue;
		if(fflush(s) == EOF) aga_logdierr("fflush");
		if(fclose(s) == EOF) aga_logdierr("fclose");
	}

	return AF_ERR_NONE;
}

void aga_loghdr(void* s, const char* loc, enum aga_logsev sev) {
#define ESC "\033[0"
#define CYN ESC "36m"
#define YLW ESC "33m"
#define RED ESC "31m"
#define END ESC "m"
	const char* f;
	if(isatty(fileno(s))) {
		switch(sev) {
			default: break;
			case AGA_NORM: f = CYN "[%s]" END " "; break;
			case AGA_WARN: f = YLW "[%s]" END " "; break;
			case AGA_ERR: f = RED "[%s]" END " "; break;
		}
	}
	else f = "[%s] ";
	if(fprintf(s, f, loc) < 0) aga_logdierr("fprintf");
#undef ESC
#undef CYN
#undef YLW
#undef RED
#undef END
}

void aga_log(const char* loc, const char* fmt, ...) {
	aga_fixed_buf_t buf = { 0 };

	af_size_t i;

	va_list l;
	enum aga_logsev sev = AGA_NORM;
	va_start(l, fmt);

	if(vsprintf(buf, fmt, l) < 0) aga_logdierr("vsprintf");

	if(!strncmp(fmt, "warn", 4)) sev = AGA_WARN;
	if(!strncmp(fmt, "err", 3)) sev = AGA_ERR;

	for(i = 0; i < aga_logctx.len; ++i) {
		FILE* s = aga_logctx.targets[i];

		aga_loghdr(s, loc, sev);
		if(fputs(buf, s) == EOF) aga_logdierr("fputs");
		if(putc('\n', s) == EOF) aga_logdierr("putc");
 	}

	va_end(l);
}
