/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agalog.h>
#include <agaenv.h>
#include <agastd.h>

#ifdef _DEBUG
# define AGA_LOG_DEFAULT_STREAM (stderr)
#else
# define AGA_LOG_DEFAULT_STREAM (stdout)
#endif

#define AGA_ISTTY(s) ((s) == stdout || (s) == stderr)

struct aga_logctx aga_logctx;

static void aga_onabrt(int signum) {
	(void) signum;

	aga_killlog();
}

AGA_USED AGA_DESTRUCTOR void aga_ondestr(void) {
	aga_killlog();
}

void aga_mklog(const char** targets, af_size_t len) {
	af_size_t i;

	aga_logctx.len = len;

	if(!(aga_logctx.targets = malloc(len * sizeof(FILE*)))) {
		static FILE* so[1];
		so[0] = AGA_LOG_DEFAULT_STREAM;
		perror("malloc");
		aga_logctx.targets = (void**) so;
		aga_logctx.len = 1;
	}
	else {
		for(i = 0; i < len; ++i) {
			if(!targets[i]) aga_logctx.targets[i] = AGA_LOG_DEFAULT_STREAM;
			else if(!(aga_logctx.targets[i] = fopen(targets[i], "w+"))) {
				perror("fopen");
			}
		}
	}

	signal(SIGABRT, aga_onabrt);
}

void aga_killlog(void) {
	af_size_t i;
	for(i = 0; i < aga_logctx.len; ++i) {
		FILE* s = aga_logctx.targets[i];
		if(!s) continue;
		if(fflush(s) == EOF) perror("fflush");
		if(AGA_ISTTY(s)) continue;
		if(fclose(s) == EOF) perror("fclose");
	}
}

void aga_loghdr(void* s, const char* loc, enum aga_logsev sev) {
#define ESC "\033[0"
#define CYN ESC "36m"
#define YLW ESC "33m"
#define RED ESC "31m"
#define END ESC "m"

	const char* f;
	if(!s) return;

	/* TODO: Enable virtual terminal sequences for Windows. */
	if(AGA_ISTTY(s)) {
		switch(sev) {
			default: break;
			case AGA_NORM: f = CYN "[%s]" END " "; break;
			case AGA_WARN: f = YLW "[%s]" END " "; break;
			case AGA_ERR: f = RED "[%s]" END " "; break;
		}
	}
	else f = "[%s] ";

	if(fprintf(s, f, loc) < 0) perror("fprintf");

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

	if(vsprintf(buf, fmt, l) < 0) perror("vsprintf");

	if(!strncmp(fmt, "warn", 4)) sev = AGA_WARN;
	if(!strncmp(fmt, "err", 3)) sev = AGA_ERR;

	for(i = 0; i < aga_logctx.len; ++i) {
		FILE* s = aga_logctx.targets[i];
		if(!s) continue;

		aga_loghdr(s, loc, sev);
		if(fputs(buf, s) == EOF) perror("fputs");
		if(putc('\n', s) == EOF) perror("putc");
 	}

	va_end(l);
}
