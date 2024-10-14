/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PACK_H
#define AGA_PACK_H

#include <aga/config.h>

#include <aga/result.h>

#define AGA_PACK_MAGIC (0xA6AU)

struct aga_resource_pack;

struct aga_resource_pack_header {
	aga_uint_t size;
	aga_uint_t magic;
};

struct aga_resource {
	aga_size_t refcount;
	aga_size_t offset; /* Offset into pack data fields, not `data' member. */

	void* data;
	aga_size_t size;

	struct aga_resource_pack* pack;

	struct aga_config_node* conf;
};

struct aga_resource_pack {
	void* fp;
	aga_size_t size;
	aga_size_t data_offset;

	/* TODO: This should eventually be a hashmap. */
	struct aga_resource* db;
	aga_size_t len; /* Alias for `pack->root.children->len'. */

#ifndef NDEBUG
	aga_size_t outstanding_refs;
#endif

	struct aga_config_node root;
};

/*
 * TODO: This is only for situations where we can't get the context through
 *		 Non-global data flow (i.e. filesystem intercepts). Once we have a
 *		 More congruent state model for Python etc. we can
 */
extern struct aga_resource_pack* aga_global_pack;

enum aga_result aga_resource_pack_new(const char*, struct aga_resource_pack*);
enum aga_result aga_resource_pack_delete(struct aga_resource_pack*);

enum aga_result aga_resource_pack_lookup(
		struct aga_resource_pack*, const char*, struct aga_resource**);

enum aga_result aga_resource_pack_sweep(struct aga_resource_pack*);

/* Also counts as an acquire - i.e. initial refcount is 1. */
enum aga_result aga_resource_new(
		struct aga_resource_pack*, const char*, struct aga_resource**);

enum aga_result aga_resource_stream(
		struct aga_resource_pack*, const char*, void**, aga_size_t*);

enum aga_result aga_resource_seek(struct aga_resource*, void**);

/*
 * NOTE: You should ensure that you acquire after any potential error
 * 		 Conditions during object init, and before any potential error
 * 		 Conditions during object destroy in order to avoid holding onto refs
 * 		 For invalid objects.
 */
enum aga_result aga_resource_aquire(struct aga_resource*);
enum aga_result aga_resource_release(struct aga_resource*);

#endif
