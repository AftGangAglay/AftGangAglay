/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_CONF_H
#define AGA_CONF_H

#include <afeirsa/aferr.h>

/*
 * NOTE: This is a pretty restrictive way to represent the quite versatile
 * 		 SGML data format (f.e. no attributes) but it'll do for our needs for
 * 		 Now.
 */

enum aga_conf_type {
	AGA_NONE,
	AGA_STRING,
	AGA_INTEGER
};

struct aga_conf_node;
struct aga_conf_node {
	char* name;

	enum aga_conf_type type;
	union aga_conf_node_data {
		char* string;
		long integer;
	} data;

	af_size_t scratch; /* Library internal - you peeked behind the curtain. */

	struct aga_conf_node* children;
	af_size_t len;
};

enum af_err aga_mkconf(const char* path, struct aga_conf_node* root);
enum af_err aga_killconf(struct aga_conf_node* root);

#endif
