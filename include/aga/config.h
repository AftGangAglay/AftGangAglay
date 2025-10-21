/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_CONFIG_H
#define AGA_CONFIG_H

#include <asys/base.h>
#include <asys/result.h>

/*
 * NOTE: This is a pretty restrictive way to represent the quite versatile
 * 		 SGML data format (f.e. no attributes) but it'll do for our needs for
 * 		 Now.
 */

struct asys_stream;

/*
 * Read config until EOF instead of by length. Only available in dev builds.
 */
#define AGA_CONFIG_EOF ((asys_size_t) -1)

enum aga_config_node_type {
	AGA_NONE,
	AGA_STRING,
	AGA_INTEGER,
	AGA_FLOAT,

	/* Convert `/' a string node into `\' when on Win32 platform. */
	AGA_PATH
};

typedef asys_native_long_t aga_config_int_t;

struct aga_config_node;
struct aga_config_node {
	char* name;

	enum aga_config_node_type type;
	union aga_config_node_data {
		char* string;
		aga_config_int_t integer;
		double flt;
	} data;

	asys_size_t scratch;

	struct aga_config_node* children;
	asys_size_t len;
};

enum asys_result aga_config_new(
		struct asys_stream*, asys_size_t, struct aga_config_node*);

enum asys_result aga_config_delete(struct aga_config_node*);

/*
 * NOTE: `aga_config_variable' and callers (all lookup functions) output a
 *		 Heap allocated string for `type == AGA_PATH'. This may change once
 *		 Path type nodes are natively implemented.
 */

asys_bool_t aga_config_variable(
		const char*, struct aga_config_node*, enum aga_config_node_type,
		void*);

enum asys_result aga_config_lookup_raw(
		struct aga_config_node*, const char**, asys_size_t,
		struct aga_config_node**);

/* Just wraps `aga_config_lookup_raw' with verbose EH. */
enum asys_result aga_config_lookup_check(
		struct aga_config_node*, const char**, asys_size_t,
		struct aga_config_node**);

enum asys_result aga_config_lookup(
		struct aga_config_node*, const char**, asys_size_t, void*,
		enum aga_config_node_type, asys_bool_t);

enum asys_result aga_config_dump(struct aga_config_node*, struct asys_stream*);

#endif
