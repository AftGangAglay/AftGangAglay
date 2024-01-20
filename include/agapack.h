/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PACK_H
#define AGA_PACK_H

#include <agaconf.h>

#include <afeirsa/afeirsa.h>

struct aga_respack;
struct aga_res {
	af_size_t refcount;
	af_size_t offset; /* Offset into pack data fields, not `data' member. */

	void* data;
	af_size_t size;

	struct aga_respack* pack;

	struct aga_conf_node* conf;
};

struct aga_respack {
	void* fp;
	af_size_t size;
	af_size_t data_offset;

	/* TODO: This should eventually be a hashmap. */
	struct aga_res* db;
	af_size_t len; /* Alias for `pack->root.children->len'. */

	struct aga_conf_node root;
};

enum af_err aga_mkrespack(const char* path, struct aga_respack* pack);
enum af_err aga_killrespack(struct aga_respack* pack);
enum af_err aga_sweeprespack(struct aga_respack* pack);

/* Also counts as an acquire - i.e. initial refcount is 1. */
enum af_err aga_mkres(
		struct aga_respack* pack, const char* path, struct aga_res** res);

enum af_err aga_resfptr(
		struct aga_respack* pack, const char* path, void** fp,
		af_size_t* size);
enum af_err aga_resseek(struct aga_res* res, void** fp);

/*
 * NOTE: You should ensure that you acquire after any potential error
 * 		 Conditions during object init, and before any potential error
 * 		 Conditions during object destroy in order to avoid holding onto refs
 * 		 For invalid objects.
 */
enum af_err aga_acquireres(struct aga_res* res);
enum af_err aga_releaseres(struct aga_res* res);

#endif
