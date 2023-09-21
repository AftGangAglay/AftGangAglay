/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agalog.h>

struct aga_logctx aga_logctx;

static void aga_onabrt(int signum) {
	(void) signum;

	aga_killlog();
}

enum af_err aga_mklog(const char** targets, af_size_t len) {
	af_size_t i;

	AF_PARAM_CHK(targets);

	aga_logctx.len = len;

	if(!(aga_logctx.targets = malloc(len * sizeof(FILE*)))) return AF_ERR_MEM;

	for(i = 0; i < len; ++i) {
		if(!(aga_logctx.targets[i] = fopen(targets[i], "w+"))) {
			perror("fopen");
			abort();
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
		if(fflush(s) == EOF) {
			perror("fflush");
			abort();
		}
		if(fclose(s) == EOF) {
			perror("fclose");
			abort();
		}
	}

	return AF_ERR_NONE;
}

void aga_log(const char* loc, const char* fmt, ...) {

	aga_fixed_buf_t buf = { 0 };

	af_size_t i;

	va_list l;
	va_start(l, fmt);

	if(vsprintf(buf, fmt, l) < 0) {
		perror("vsprintf");
		abort();
	}

	for(i = 0; i < aga_logctx.len; ++i) {
		if(fprintf(aga_logctx.targets[i], "[%s] %s\n", loc, buf) < 0) {
			perror("fprintf");
			abort();
		}
 	}

	va_end(l);
}
