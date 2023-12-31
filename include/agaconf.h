/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_CONF_H
#define AGA_CONF_H

#include <afeirsa/afeirsa.h>

/*
 * NOTE: This is a pretty restrictive way to represent the quite versatile
 * 		 SGML data format (f.e. no attributes) but it'll do for our needs for
 * 		 Now.
 */

enum aga_conf_type {
	AGA_NONE,
	AGA_STRING,
	AGA_INTEGER,
	AGA_FLOAT
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

	af_size_t scratch; /* Library internal - you peeked behind the curtain. */

	struct aga_conf_node* children;
	af_size_t len;
};

enum af_err aga_mkconf(const char* path, struct aga_conf_node* root);
enum af_err aga_killconf(struct aga_conf_node* root);

af_bool_t aga_confvar(
		const char* name, struct aga_conf_node* node, enum aga_conf_type type,
		void* value);

enum af_err aga_conftree_raw(
		struct aga_conf_node* root, const char** names, af_size_t count,
		struct aga_conf_node** out);

enum af_err aga_conftree(
		struct aga_conf_node* root, const char** names, af_size_t count,
		void* value, enum aga_conf_type type);

#endif
