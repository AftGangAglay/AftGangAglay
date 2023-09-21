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

void aga_log(const char* loc, const char* fmt, ...) {

	static const char cfstr[] = "\033[0;36m[%s]\033[0m %s\n";
	static const char fstr[] = "[%s] %s\n";
	aga_fixed_buf_t buf = { 0 };

	af_size_t i;

	va_list l;
	va_start(l, fmt);

	if(vsprintf(buf, fmt, l) < 0) aga_logdierr("vsprintf");

	for(i = 0; i < aga_logctx.len; ++i) {
		FILE* s = aga_logctx.targets[i];
		const char* f = fstr;
		if(isatty(fileno(s))) f = cfstr;

		if(fprintf(s, f, loc, buf) < 0) aga_logdierr("fprintf");
 	}

	va_end(l);
}
