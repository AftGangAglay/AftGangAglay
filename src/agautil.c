/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agautil.h>
#include <agastd.h>

void* aga_memset(void* p, int c, aga_size_t n) {
#ifdef AGA_NO_STD
	aga_uchar_t* pc = p;
	aga_size_t i;

	for(i = 0; i < n; ++i) pc[i] = c;

	return p;
#else
	return memset(p, c, n);
#endif
}

void* aga_memcpy(void* to, const void* from, aga_size_t sz) {
#ifdef AGA_NO_STD
	aga_size_t i;
	aga_uchar_t* toc = to;
	const aga_uchar_t* fromc = from;

	for(i = 0; i < sz; ++i) {
		toc[i] = fromc[i];
	}

	return to;
#else
	return memcpy(to, from, sz);
#endif
}

aga_bool_t aga_streql(const char* a, const char* b) {
#ifdef AGA_NO_STD
	return aga_strneql(a, b, (aga_size_t) -1);
#else
	return !strcmp(a, b);
#endif
}

aga_bool_t aga_strneql(const char* a, const char* b, aga_size_t sz) {
#ifdef AGA_NO_STD
	aga_size_t i;

	for(i = 0; i < sz; ++i) {
		if(!a[i]) return !b[i];
		if(a[i] != b[i]) return AGA_FALSE;
	}

	return AGA_TRUE;
#else
	return !strncmp(a, b, sz);
#endif
}

aga_size_t aga_strlen(const char* s) {
#ifdef AGA_NO_STD
	aga_size_t i;

	for(i = 0; s[i]; ++i) continue;

	return i;
#else
	return strlen(s);
#endif
}

aga_slong_t aga_strtol(const char* s) {
	aga_size_t i;
	aga_size_t len = aga_strlen(s);
	aga_slong_t ret = 0;

	for(i = len; i > 0; --i) {
		char c = s[i - 1];

		if(c < '0' || c > '9') break;

		ret += (c - '0') * (aga_slong_t) ((len - i) * 10);
	}

	return ret;
}

double aga_strtod(const char* s) {
#ifdef AGA_NO_STD
	/* TODO: Standalone impl. */
	(void) s;
	return 0.0;
#else
	return strtod(s, 0);
#endif
}

/* TODO: Standalone/overridable allocator. */

void* aga_malloc(aga_size_t sz) {
#ifdef AGA_NO_STD
	(void) sz;
	return 0;
#else
	return malloc(sz);
#endif
}

/* TODO: Nostd + Win32. */
char* aga_getenv(const char* s) {
#ifdef AGA_NO_STD
	(void) s;
	return 0;
#else
	return getenv(s);
#endif
}

void* aga_calloc(aga_size_t n, aga_size_t sz) {
#ifdef AGA_NO_STD
	(void) n;
	(void) sz;
	return 0;
#else
	return calloc(n, sz);
#endif
}

void* aga_realloc(void* p, aga_size_t sz) {
#ifdef AGA_NO_STD
	(void) p;
	(void) sz;
	return 0;
#else
	void* new = realloc(p, sz);
	if(!new) free(p);
	return new;
#endif
}

void aga_free(void* p) {
#ifdef AGA_NO_STD
	(void) p;
#else
	free(p);
#endif
}
