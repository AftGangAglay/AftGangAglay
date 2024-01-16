/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PACK_H
#define AGA_PACK_H

#include <afeirsa/afeirsa.h>

struct aga_respack;
struct aga_res {
	af_size_t refcount;
	af_size_t offset; /* Offset into main buffer, not `data' member. */

	void* data;
	af_size_t size;

	struct aga_respack* pack;

	struct aga_conf_node* conf;
};

struct aga_respack {
	char dummy;
};

enum af_err aga_mkrespack(const char* path, struct aga_respack* pack);
enum af_err aga_killrespack(struct aga_respack* pack);
enum af_err aga_sweeprespack(struct aga_respack* pack);

/* Also counts as an acquire - i.e. initial refcount is 1. */
enum af_err aga_mkres(
		struct aga_respack* pack, const char* path, struct aga_res** res);

/*
 * NOTE: You should ensure that you acquire after any potential error
 * 		 Conditions during object init, and before any potential error
 * 		 Conditions during object destroy in order to avoid holding onto refs
 * 		 For invalid objects.
 */
enum af_err aga_acquireres(struct aga_res* res);
enum af_err aga_releaseres(struct aga_res* res);

#endif
