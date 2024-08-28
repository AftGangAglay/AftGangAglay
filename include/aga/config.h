/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_CONFIG_H
#define AGA_CONFIG_H

#include <aga/environment.h>
#include <aga/result.h>

/*
 * NOTE: This is a pretty restrictive way to represent the quite versatile
 * 		 SGML data format (f.e. no attributes) but it'll do for our needs for
 * 		 Now.
 */

enum aga_config_node_type {
	AGA_NONE, AGA_STRING, AGA_INTEGER, AGA_FLOAT
};

struct aga_config_node;
struct aga_config_node {
	char* name;

	enum aga_config_node_type type;
	union aga_config_node_data {
		char* string;
		aga_slong_t integer;
		double flt;
	} data;

	aga_size_t scratch; /* Library internal - you peeked behind the curtain. */

	struct aga_config_node* children;
	aga_size_t len;
};

/* Specify the filename of the config file being parsed for debug purposes. */
extern const char* aga_config_debug_file;

enum aga_result aga_config_new(void*, aga_size_t, struct aga_config_node*);
enum aga_result aga_config_delete(struct aga_config_node*);

aga_bool_t aga_config_variable(
		const char*, struct aga_config_node*, enum aga_config_node_type, void*);

enum aga_result aga_config_lookup_raw(
		struct aga_config_node*, const char**, aga_size_t,
		struct aga_config_node**);

/* Just wraps `aga_config_lookup_raw' with verbose EH. */
enum aga_result aga_config_lookup_check(
		struct aga_config_node*, const char**, aga_size_t,
		struct aga_config_node**);

enum aga_result aga_config_lookup(
		struct aga_config_node*, const char**, aga_size_t, void*,
		enum aga_config_node_type, aga_bool_t);

enum aga_result aga_config_dump(struct aga_config_node*, void*);

#endif
