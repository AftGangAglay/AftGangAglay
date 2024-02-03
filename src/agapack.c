/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agapack.h>
#include <agaio.h>
#include <agaerr.h>
#include <agalog.h>
#include <agastd.h>
#include <agascript.h>

#define AGA_PACK_MAGIC ((af_uint32_t) 0xA6A)

struct aga_pack_header {
	af_uint32_t size;
	af_uint32_t magic;
};

struct aga_respack* aga_global_pack = 0;

enum af_err aga_searchres(
		struct aga_respack* pack, const char* path, struct aga_res** out) {

	af_size_t i;

	AF_PARAM_CHK(pack);
	AF_PARAM_CHK(path);
	AF_PARAM_CHK(out);

	for(i = 0; i < pack->len; ++i) {
		if(af_streql(pack->db[i].conf->name, path)) {
			*out = &pack->db[i];
			return AF_ERR_NONE;
		}
	}

	return AF_ERR_BAD_PARAM;
}

enum af_err aga_mkrespack(const char* path, struct aga_respack* pack) {
	enum af_err result;
	af_size_t i;
	struct aga_pack_header hdr;

	AF_PARAM_CHK(path);
	AF_PARAM_CHK(pack);

    aga_global_pack = pack;

	pack->db = 0;
	pack->len = 0;

	aga_log(__FILE__, "Loading resource pack `%s'...", path);

	/* TODO: Fix leaky error conditions. */

	/* TODO: Extract embedded resource file on Windows. */

	if(!(pack->fp = fopen(path, "rb"))) {
		return aga_af_patherrno(__FILE__, "fopen", path);
	}

	AF_CHK(aga_fplen(pack->fp, &pack->size));

	if(fread(&hdr, 1, sizeof(hdr), pack->fp) != sizeof(hdr)) {
		if(ferror(pack->fp)) return aga_af_patherrno(__FILE__, "fread", path);
	}

	AF_VERIFY(hdr.magic == AGA_PACK_MAGIC, AF_ERR_BAD_PARAM);
	aga_conf_debug_file = path;
	AF_CHK(aga_mkconf(pack->fp, hdr.size, &pack->root));

	pack->len = pack->root.children->len;
	pack->data_offset = hdr.size + sizeof(hdr);
	pack->db = calloc(pack->len, sizeof(struct aga_res));
	AF_VERIFY(pack->db, AF_ERR_MEM);

	for(i = 0; i < pack->len; ++i) {
		/* TODO: Non-fatally skip bad entries. */
		static const char* off = "Offset";
		static const char* size = "Size";

		struct aga_res* res = &pack->db[i];
		struct aga_conf_node* node = &pack->root.children->children[i];

		res->conf = node;
		res->pack = pack;

		result = aga_conftree_nonroot(
			node, &off, 1, &res->offset, AGA_INTEGER);
		AF_CHK(result);
		AF_VERIFY(res->offset < pack->size, AF_ERR_BAD_PARAM);

		result = aga_conftree_nonroot(node, &size, 1, &res->size, AGA_INTEGER);
		AF_CHK(result);
		AF_VERIFY(res->offset + res->size < pack->size, AF_ERR_BAD_PARAM);
	}

	aga_log(__FILE__, "Loaded `%zu' resource entries", pack->len);

	return AF_ERR_NONE;
}

enum af_err aga_killrespack(struct aga_respack* pack) {
	AF_PARAM_CHK(pack);

	free(pack->db);
	if(fclose(pack->fp) == EOF) return aga_af_errno(__FILE__, "fclose");
	AF_CHK(aga_killconf(&pack->root));

	return AF_ERR_NONE;
}

enum af_err aga_sweeprespack(struct aga_respack* pack) {
	af_size_t i;

	AF_PARAM_CHK(pack);

	for(i = 0; i < pack->len; ++i) {
		struct aga_res* res = &pack->db[i];

		if(res->refcount || !res->data) continue;

		free(res->data);
		res->data = 0;
	}

	return AF_ERR_NONE;
}

enum af_err aga_mkres(
		struct aga_respack* pack, const char* path, struct aga_res** res) {

	AF_PARAM_CHK(path);
	AF_PARAM_CHK(pack);
	AF_PARAM_CHK(res);

	AF_CHK(aga_searchres(pack, path, res));

	if(!(*res)->data) {
		AF_CHK(aga_resseek(*res, 0));

		/* TODO: Use mapping for large reads. */
		AF_VERIFY((*res)->data = malloc((*res)->size), AF_ERR_MEM);

		if(fread((*res)->data, 1, (*res)->size, pack->fp) != (*res)->size) {
			return aga_af_errno(__FILE__, "fread");
		}
	}

	++(*res)->refcount;

	return AF_ERR_NONE;
}

enum af_err aga_resfptr(
		struct aga_respack* pack, const char* path, void** fp,
		af_size_t* size) {

	struct aga_res* res;

	AF_PARAM_CHK(pack);
	AF_PARAM_CHK(path);

	AF_CHK(aga_searchres(pack, path, &res));

	AF_CHK(aga_resseek(res, 0));

	*fp = pack->fp;
	*size = res->size;

	return AF_ERR_NONE;
}

enum af_err aga_resseek(struct aga_res* res, void** fp) {
	int result;
	af_size_t offset;

	AF_PARAM_CHK(res);

	offset = res->pack->data_offset + res->offset;
	result = fseek(res->pack->fp, (long) offset, SEEK_SET);
	if(result == -1) return aga_af_errno(__FILE__, "fseek");

	if(fp) *fp = res->pack->fp;

	return AF_ERR_NONE;
}

enum af_err aga_acquireres(struct aga_res* res) {
	AF_PARAM_CHK(res);

	++res->refcount;

	return AF_ERR_NONE;
}

enum af_err aga_releaseres(struct aga_res* res) {
	AF_PARAM_CHK(res);

	if(res->refcount) --res->refcount;

	return AF_ERR_NONE;
}
