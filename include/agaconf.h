/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_CONF_H
#define AGA_CONF_H

#include <agaresult.h>

/*
 * NOTE: This is a pretty restrictive way to represent the quite versatile
 * 		 SGML data format (f.e. no attributes) but it'll do for our needs for
 * 		 Now.
 */

enum aga_conf_type {
	AGA_NONE, AGA_STRING, AGA_INTEGER, AGA_FLOAT
};

struct aga_conf_node;
struct aga_conf_node {
	char* name;

	enum aga_conf_type type;
	union aga_conf_node_data {
		char* string;
		long integer;
		double flt;
	} data;

	aga_size_t scratch; /* Library internal - you peeked behind the curtain. */

	struct aga_conf_node* children;
	aga_size_t len;
};

/* Specify the filename of the config file being parsed for debug purposes. */
extern const char* aga_conf_debug_file;

enum aga_result aga_mkconf(void*, aga_size_t, struct aga_conf_node*);
enum aga_result aga_killconf(struct aga_conf_node*);

aga_bool_t aga_confvar(
		const char*, struct aga_conf_node*, enum aga_conf_type, void*);

enum aga_result aga_conftree_raw(
		struct aga_conf_node*, const char**, aga_size_t,
		struct aga_conf_node**);

enum aga_result aga_conftree_nonroot(
		struct aga_conf_node*, const char**, aga_size_t, void*,
		enum aga_conf_type);

enum aga_result aga_conftree(
		struct aga_conf_node*, const char**, aga_size_t, void*,
		enum aga_conf_type);

#endif
