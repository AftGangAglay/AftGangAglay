/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef ASYS_STRING_H
#define ASYS_STRING_H

#include <asys/base.h>
#include <asys/result.h>

/* TODO: This is overkill. */
typedef char asys_float_format_buffer_t[128 + 1];
typedef int (*asys_string_find_predicate_t)(int);

int asys_character_is_blank(int);
int asys_character_is_letter(int);
int asys_character_is_dec_digit(int);
int asys_character_is_hex_digit(int);
int asys_character_is_oct_digit(int);

int asys_character_to_upper(int);
int asys_character_to_lower(int);

asys_size_t asys_string_length(const char*);

asys_bool_t asys_string_equal(const char*, const char*);
void asys_string_concatenate(char*, const char*);

char* asys_string_find(char*, char);
const char* asys_string_find_const(const char*, char);

char* asys_string_reverse_find(char*, char);
const char* asys_string_reverse_find_const(const char*, char);

char* asys_string_find_predicate(char*, asys_string_find_predicate_t);
const char* asys_string_find_const_predicate(
		const char*, asys_string_find_predicate_t);

char* asys_string_reverse_find_predicate(char*, asys_string_find_predicate_t);
const char* asys_string_reverse_find_const_predicate(
		const char*, asys_string_find_predicate_t);

char* asys_string_find_predicate_inverse(
		char*, asys_string_find_predicate_t);

const char* asys_string_find_const_predicate_inverse(
		const char*, asys_string_find_predicate_t);

char* asys_string_reverse_find_predicate_inverse(
		char*, asys_string_find_predicate_t);

const char* asys_string_reverse_find_const_predicate_inverse(
		const char*, asys_string_find_predicate_t);

/* NOTE: In case of null return, assume `ASYS_RESULT_OOM' error value. */
char* asys_string_duplicate(const char*);

asys_native_long_t asys_string_to_native_long(const char*, char**);
double asys_string_to_double(const char*, char**);

enum asys_result asys_float_to_string(float, asys_float_format_buffer_t*);
const char* asys_bool_to_string(asys_bool_t);

/*
 * If the string is null, returns "<null>" -- otherwise returns the input
 * String.
 */
const char* asys_string_optional(const char*);

enum asys_result asys_string_format(
		asys_fixed_buffer_t*, asys_size_t*, const char*, ...);

#ifdef ASYS_VARARGS_H
enum asys_result asys_string_format_variadic(
		asys_fixed_buffer_t*, asys_size_t*, const char*, va_list);
#endif

#endif
