/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/system.h>
#include <asys/string.h>
#include <asys/memory.h>
#include <asys/error.h>
#include <asys/log.h>

int asys_character_is_blank(int character) {
#ifdef ASYS_STDC
	return isspace(character);
#else
	return character == ' ' || character == '\r' || character == '\t' ||
			character == '\n';
#endif
}

int asys_character_is_letter(int character) {
#ifdef ASYS_STDC
	return isalpha(character);
#else
	return (character >= 'a' && character <= 'z') ||
			(character >= 'A' && character <= 'Z');
#endif
}

int asys_character_is_dec_digit(int character) {
#ifdef ASYS_STDC
	return isdigit(character);
#else
	return character >= '0' && character <= '9';
#endif
}

int asys_character_is_hex_digit(int character) {
#ifdef ASYS_STDC
	return isxdigit(character);
#else
	return asys_character_is_dec_digit(character) ||
			(asys_character_to_upper(character) > 'A' &&
			asys_character_to_upper(character) <= 'F');
#endif
}

int asys_character_is_oct_digit(int character) {
	return character >= '0' && character <= '8';
}

#ifndef ASYS_STDC
static const int case_difference = 'a' - 'A';
#endif

int asys_character_to_upper(int character) {
#ifdef ASYS_STDC
	return toupper(character);
#else
	return (character > 'A' && character < 'Z') ?
			(character - case_difference) : character;
#endif
}

int asys_character_to_lower(int character) {
#ifdef ASYS_STDC
	return tolower(character);
#else
	return (character > 'A' && character < 'Z') ?
			(character + case_difference) : character;
#endif
}

asys_size_t asys_string_length(const char* string) {
#ifdef ASYS_WIN32
	return (asys_size_t) lstrlen(string);
#elif defined(ASYS_STDC)
	return strlen(string);
#else
	asys_size_t length;

	for(length = 0; string[length]; ++length) continue;

	return length;
#endif
}

/* TODO: `lstrcmpi' exists for `strcasecmp'. */
asys_bool_t asys_string_equal(const char* a, const char* b) {
#ifdef ASYS_WIN32
	return !lstrcmp(a, b);
#elif defined(ASYS_STDC)
	return !strcmp(a, b);
#else
	asys_size_t i = 0;

	while(ASYS_TRUE) {
		if(a[i] != b[i]) return ASYS_FALSE;
		if(!a[i]) return ASYS_TRUE;

		++i;
	}
#endif
}

void asys_string_concatenate(char* to, const char* from) {
#ifdef ASYS_WIN32
	lstrcat(to, from);
#elif defined(ASYS_STDC)
	strcat(to, from);
#else
	(void) to;
	(void) from;
#endif
}

#ifndef ASYS_STDC
static asys_offset_t asys_string_find_base(
		const char* string, char character) {

	asys_size_t i;

	for(i = 0; string[i]; ++i) {
		if(string[i] == character) return (asys_offset_t) i;
	}

	if(character == '\0') return (asys_offset_t) i;

	return -1;
}

static asys_offset_t asys_string_reverse_find_base(
		const char* string, char character) {

	asys_size_t i;
	asys_offset_t last = -1;

	if(character == '\0') return (asys_offset_t) asys_string_length(string);

	for(i = 0; string[i]; ++i) {
		if(string[i] == character) last = i;
	}

	return (asys_offset_t) last;
}
#endif

char* asys_string_find(char* string, char character) {
#ifdef ASYS_STDC
	return strchr(string, character);
#else
	asys_offset_t offset;

	if((offset = asys_string_find_base(string, character)) == -1) return 0;

	return string + offset;
#endif
}

const char* asys_string_find_const(const char* string, char character) {
#ifdef ASYS_STDC
	return strchr(string, character);
#else
	asys_offset_t offset;

	if((offset = asys_string_find_base(string, character)) == -1) return 0;

	return string + offset;
#endif
}

char* asys_string_reverse_find(char* string, char character) {
#ifdef ASYS_STDC
	return strrchr(string, character);
#else
	asys_offset_t offset;

	offset = asys_string_reverse_find_base(string, character);
	if(offset == -1) return 0;

	return string + offset;
#endif
}

const char* asys_string_reverse_find_const(
		const char* string, char character) {

#ifdef ASYS_STDC
	return strrchr(string, character);
#else
	asys_offset_t offset;

	offset = asys_string_reverse_find_base(string, character);
	if(offset == -1) return 0;

	return string + offset;
#endif
}

static asys_offset_t asys_string_find_predicate_base(
		const char* string, asys_string_find_predicate_t predicate,
		asys_bool_t inverse) {

	asys_size_t i;

	for(i = 0; string[i]; ++i) {
		if(!!predicate(string[i]) != inverse) return (asys_offset_t) i;
	}

	if(!!predicate('\0') != inverse) return (asys_offset_t) i;

	return -1;
}

static asys_offset_t asys_string_reverse_find_predicate_base(
		const char* string, asys_string_find_predicate_t predicate,
		asys_bool_t inverse) {

	asys_size_t i;
	asys_offset_t last = -1;

	if(!!predicate('\0') != inverse) {
		return (asys_offset_t) asys_string_length(string);
	}

	for(i = 0; string[i]; ++i) {
		if(!!predicate(string[i]) != inverse) last = i;
	}

	return (asys_offset_t) last;
}

char* asys_string_find_predicate(
		char* string, asys_string_find_predicate_t predicate) {

	asys_offset_t offset;

	offset = asys_string_find_predicate_base(string, predicate, ASYS_FALSE);
	if(offset == -1) return 0;

	return string + offset;
}

const char* asys_string_find_const_predicate(
		const char* string, asys_string_find_predicate_t predicate) {

	asys_offset_t offset;

	offset = asys_string_find_predicate_base(string, predicate, ASYS_FALSE);
	if(offset == -1) return 0;

	return string + offset;
}

char* asys_string_reverse_find_predicate(
		char* string, asys_string_find_predicate_t predicate) {

	asys_offset_t offset;

	offset = asys_string_reverse_find_predicate_base(
				string, predicate, ASYS_FALSE);

	if(offset == -1) return 0;

	return string + offset;
}

const char* asys_string_reverse_find_const_predicate(
		const char* string, asys_string_find_predicate_t predicate) {

	asys_offset_t offset;

	offset = asys_string_reverse_find_predicate_base(
				string, predicate, ASYS_FALSE);

	if(offset == -1) return 0;

	return string + offset;
}

char* asys_string_find_predicate_inverse(
		char* string, asys_string_find_predicate_t predicate) {

	asys_offset_t offset;

	offset = asys_string_find_predicate_base(string, predicate, ASYS_TRUE);

	return string + offset;
}

const char* asys_string_find_const_predicate_inverse(
		const char* string, asys_string_find_predicate_t predicate) {

	asys_offset_t offset;

	offset = asys_string_find_predicate_base(string, predicate, ASYS_TRUE);
	if(offset == -1) return 0;

	return string + offset;
}

char* asys_string_reverse_find_predicate_inverse(
		char* string, asys_string_find_predicate_t predicate) {

	asys_offset_t offset;

	offset = asys_string_reverse_find_predicate_base(
			string, predicate, ASYS_TRUE);

	if(offset == -1) return 0;

	return string + offset;
}

const char* asys_string_reverse_find_const_predicate_inverse(
		const char* string, asys_string_find_predicate_t predicate) {

	asys_offset_t offset;

	offset = asys_string_reverse_find_predicate_base(
			string, predicate, ASYS_TRUE);

	if(offset == -1) return 0;

	return string + offset;
}

char* asys_string_duplicate(const char* string) {
	/* TODO: Detect impl. native `strdup'. */

	asys_size_t length = asys_string_length(string);
	char* new = asys_memory_allocate(length + 1);

	asys_memory_copy(new, string, length + 1);

	return new;
}

#ifndef ASYS_STDC
# include <glibc/stdlib/strtol.c>
#endif

asys_native_long_t asys_string_to_native_long(
		const char* string, char** end) {

#ifdef ASYS_STDC
# ifdef ASYS_LP32
	return strtoll(string, end, 0);
# else
	return strtol(string, end, 0);
# endif
#else
	return glibc_strtol(string, end, 0);
#endif
}

double asys_string_to_double(const char* string, char** end) {
#ifdef ASYS_STDC
	return strtod(string, end);
#elif defined(ASYS_WIN32)
	/* TODO: Temporary. */
	return strtod(string, end);
#else
	/* TODO: Use Glibc. */
	(void) string;
	(void) end;

	return 0.0;
#endif
}

enum asys_result asys_float_to_string(
		float value, asys_float_format_buffer_t* buffer) {

#ifdef ASYS_STDC
	int count = sprintf(*buffer, "%f", value);
	if(count < 0) return asys_result_errno(__FILE__, "sprintf");

	return ASYS_RESULT_OK;
#elif defined(ASYS_WIN32)
	/* TODO: Temporary. */
	extern int sprintf(char*, const char*, ...);
	int count = sprintf(*buffer, "%f", value);
	if(count < 0) return ASYS_RESULT_ERROR;

	return ASYS_RESULT_OK;
#else
	/* TODO: Use Glibc. */
	(void) value;
	(void) buffer;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}

const char* asys_bool_to_string(asys_bool_t value) {
	return value ? "true" : "false";
}

const char* asys_string_optional(const char* string) {
	return string ? string : "<null>";
}

enum asys_result asys_string_format(
		asys_fixed_buffer_t* buffer, asys_size_t* format_count,
		const char* format, ...) {

	enum asys_result result;
	va_list list;

	va_start(list, format);

	result = asys_string_format_variadic(buffer, format_count, format, list);

	va_end(list);

	return result;
}

enum asys_result asys_string_format_variadic(
		asys_fixed_buffer_t* buffer, asys_size_t* format_count,
		const char* format, va_list list) {

#ifdef ASYS_WIN32
	enum asys_result result;

	/* TODO: MSDN makes it unclear if this is the correct error condition. */
	int count = wvsprintf(*buffer, format, list);

	if(format_count) *format_count = count;

	if(count < 0) {
		result = ASYS_RESULT_ERROR;
		asys_log_result(__FILE__, "wvsprintf", result);
		return result;
	}

	return ASYS_RESULT_OK;
#elif defined(ASYS_STDC)
	int result = vsprintf(*buffer, format, list);

	if(format_count) *format_count = result;

	if(result < 0) return asys_result_errno(__FILE__, "vsprintf");

	return ASYS_RESULT_OK;
#else
	/* TODO: Roll our own format. */
	(void) buffer;
	(void) format_count;
	(void) format;
	(void) list;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}
