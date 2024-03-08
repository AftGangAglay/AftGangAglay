/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PACK_H
#define AGA_PACK_H

#include <agaconf.h>

#include <agaresult.h>

struct aga_respack;
struct aga_res {
	aga_size_t refcount;
	aga_size_t offset; /* Offset into pack data fields, not `data' member. */

	void* data;
	aga_size_t size;

	struct aga_respack* pack;

	struct aga_conf_node* conf;
};

struct aga_respack {
	void* fp;
	aga_size_t size;
	aga_size_t data_offset;

	/* TODO: This should eventually be a hashmap. */
	struct aga_res* db;
	aga_size_t len; /* Alias for `pack->root.children->len'. */

	struct aga_conf_node root;
};

/*
 * NOTE: This is only for situations where we can't get the context through
 *		 Non-global data flow (i.e. filesystem intercepts).
 */
extern struct aga_respack* aga_global_pack;

enum aga_result aga_searchres(
		struct aga_respack*, const char*, struct aga_res**);

enum aga_result aga_mkrespack(const char*, struct aga_respack*);

enum aga_result aga_killrespack(struct aga_respack*);

enum aga_result aga_sweeprespack(struct aga_respack*);

/* Also counts as an acquire - i.e. initial refcount is 1. */
enum aga_result aga_mkres(struct aga_respack*, const char*, struct aga_res**);

enum aga_result aga_resfptr(
		struct aga_respack*, const char*, void**, aga_size_t*);

enum aga_result aga_resseek(struct aga_res*, void**);

/*
 * NOTE: You should ensure that you acquire after any potential error
 * 		 Conditions during object init, and before any potential error
 * 		 Conditions during object destroy in order to avoid holding onto refs
 * 		 For invalid objects.
 */
enum aga_result aga_acquireres(struct aga_res*);
enum aga_result aga_releaseres(struct aga_res*);

#endif
