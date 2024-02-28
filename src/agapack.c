/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agapack.h>
#include <agaio.h>
#include <agaerr.h>
#include <agalog.h>
#include <agastd.h>
#include <agautil.h>
#include <agascript.h>

/* TODO: Checksum? */

#define AGA_PACK_MAGIC ((aga_uint32_t) 0xA6A)

struct aga_pack_header {
	aga_uint32_t size;
	aga_uint32_t magic;
};

struct aga_respack* aga_global_pack = 0;

enum aga_result aga_searchres(
		struct aga_respack* pack, const char* path, struct aga_res** out) {

	aga_size_t i;

	AGA_PARAM_CHK(pack);
	AGA_PARAM_CHK(path);
	AGA_PARAM_CHK(out);

	for(i = 0; i < pack->len; ++i) {
		if(aga_streql(pack->db[i].conf->name, path)) {
			*out = &pack->db[i];
			return AGA_RESULT_OK;
		}
	}

	return AGA_RESULT_BAD_PARAM;
}

enum aga_result aga_mkrespack(const char* path, struct aga_respack* pack) {
	enum aga_result result;
	aga_size_t i;
	struct aga_pack_header hdr;

	AGA_PARAM_CHK(path);
	AGA_PARAM_CHK(pack);

	aga_global_pack = pack;

	pack->db = 0;
	pack->len = 0;

	aga_log(__FILE__, "Loading resource pack `%s'...", path);

	/* TODO: Fix leaky error conditions. */

	/* TODO: Extract embedded resource file on Windows. */

	if(!(pack->fp = fopen(path, "rb"))) {
		return aga_patherrno(__FILE__, "fopen", path);
	}

	AGA_CHK(aga_fplen(pack->fp, &pack->size));

	AGA_CHK(aga_fread(&hdr, sizeof(hdr), pack->fp));

	AGA_VERIFY(hdr.magic == AGA_PACK_MAGIC, AGA_RESULT_BAD_PARAM);
	aga_conf_debug_file = path;
	AGA_CHK(aga_mkconf(pack->fp, hdr.size, &pack->root));

	pack->len = pack->root.children->len;
	pack->data_offset = hdr.size + sizeof(hdr);
	pack->db = calloc(pack->len, sizeof(struct aga_res));
	AGA_VERIFY(pack->db, AGA_RESULT_OOM);

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
		AGA_CHK(result);
		AGA_VERIFY(res->offset < pack->size, AGA_RESULT_BAD_PARAM);

		result = aga_conftree_nonroot(node, &size, 1, &res->size, AGA_INTEGER);
		AGA_CHK(result);
		AGA_VERIFY(res->offset + res->size < pack->size, AGA_RESULT_BAD_PARAM);
	}

	aga_log(__FILE__, "Loaded `%zu' resource entries", pack->len);

	return AGA_RESULT_OK;
}

enum aga_result aga_killrespack(struct aga_respack* pack) {
	AGA_PARAM_CHK(pack);

	free(pack->db);
	if(fclose(pack->fp) == EOF) return aga_errno(__FILE__, "fclose");
	AGA_CHK(aga_killconf(&pack->root));

	return AGA_RESULT_OK;
}

enum aga_result aga_sweeprespack(struct aga_respack* pack) {
	aga_size_t i;

	AGA_PARAM_CHK(pack);

	for(i = 0; i < pack->len; ++i) {
		struct aga_res* res = &pack->db[i];

		if(res->refcount || !res->data) continue;

		free(res->data);
		res->data = 0;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_mkres(
		struct aga_respack* pack, const char* path, struct aga_res** res) {

	AGA_PARAM_CHK(path);
	AGA_PARAM_CHK(pack);
	AGA_PARAM_CHK(res);

	AGA_CHK(aga_searchres(pack, path, res));

	if(!(*res)->data) {
		AGA_CHK(aga_resseek(*res, 0));

		/* TODO: Use mapping for large reads. */
		AGA_VERIFY((*res)->data = malloc((*res)->size), AGA_RESULT_OOM);

		AGA_CHK(aga_fread((*res)->data, (*res)->size, pack->fp));
	}

	++(*res)->refcount;

	return AGA_RESULT_OK;
}

enum aga_result aga_resfptr(
		struct aga_respack* pack, const char* path, void** fp,
		aga_size_t* size) {

	struct aga_res* res;

	AGA_PARAM_CHK(pack);
	AGA_PARAM_CHK(path);

	AGA_CHK(aga_searchres(pack, path, &res));

	AGA_CHK(aga_resseek(res, 0));

	*fp = pack->fp;
	*size = res->size;

	return AGA_RESULT_OK;
}

enum aga_result aga_resseek(struct aga_res* res, void** fp) {
	int result;
	aga_size_t offset;

	AGA_PARAM_CHK(res);

	offset = res->pack->data_offset + res->offset;
	result = fseek(res->pack->fp, (long) offset, SEEK_SET);
	if(result == -1) return aga_errno(__FILE__, "fseek");

	if(fp) *fp = res->pack->fp;

	return AGA_RESULT_OK;
}

enum aga_result aga_acquireres(struct aga_res* res) {
	AGA_PARAM_CHK(res);

	++res->refcount;

	return AGA_RESULT_OK;
}

enum aga_result aga_releaseres(struct aga_res* res) {
	AGA_PARAM_CHK(res);

	if(res->refcount) --res->refcount;

	return AGA_RESULT_OK;
}
