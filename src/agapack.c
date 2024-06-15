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

/*
 * TODO: Allow pack input as a "raw" argument and make space for a shebang so
 * 		 Packs can be executed directly by the shell if we're installed on the
 * 		 System.
 */

#define AGA_PACK_MAGIC ((aga_uint32_t) 0xA6A)

struct aga_pack_header {
	aga_uint32_t size;
	aga_uint32_t magic;
};

struct aga_respack* aga_global_pack = 0;

enum aga_result aga_searchres(
		struct aga_respack* pack, const char* path, struct aga_res** out) {

	aga_size_t i;

	if(!pack) return AGA_RESULT_BAD_PARAM;
	if(!path) return AGA_RESULT_BAD_PARAM;
	if(!out) return AGA_RESULT_BAD_PARAM;

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
	aga_bool_t c = AGA_FALSE;

	if(!path) return AGA_RESULT_BAD_PARAM;
	if(!pack) return AGA_RESULT_BAD_PARAM;

	aga_global_pack = pack;

	pack->fp = 0;
	pack->db = 0;
	pack->len = 0;

#ifdef _DEBUG
	pack->outstanding_refs = 0;
#endif

	aga_log(__FILE__, "Loading resource pack `%s'...", path);

	if(!(pack->fp = fopen(path, "rb"))) {
		return aga_errno_path(__FILE__, "fopen", path);
	}

	result = aga_fplen(pack->fp, &pack->size);
	if(result) goto cleanup;

	result = aga_fread(&hdr, sizeof(hdr), pack->fp);
	if(result) goto cleanup;

	if(hdr.magic != AGA_PACK_MAGIC) {
		result = AGA_RESULT_BAD_PARAM;
		goto cleanup;
	}

	aga_conf_debug_file = path;

	result = aga_mkconf(pack->fp, hdr.size, &pack->root);
	if(result) goto cleanup;

	c = AGA_TRUE;

	pack->len = pack->root.children->len;
	pack->data_offset = hdr.size + sizeof(hdr);

	pack->db = aga_calloc(pack->len, sizeof(struct aga_res));
	if(!pack->db) {
		result = AGA_RESULT_OOM;
		goto cleanup;
	}

	for(i = 0; i < pack->len; ++i) {
		static const char* off = "Offset";
		static const char* size = "Size";

		struct aga_res* res = &pack->db[i];
		struct aga_conf_node* node = &pack->root.children->children[i];

		res->conf = node;
		res->pack = pack;

		result = aga_conftree(
				node, &off, 1, &res->offset, AGA_INTEGER);
		if(result) {
			aga_soft(__FILE__, "aga_conftree", result);
			aga_log(
					__FILE__, "Resource #%zu appears to be missing an offset "
							  "entry", i);
			continue;
		}

		if(res->offset >= pack->size) {
			aga_log(
					__FILE__, "Resource #%zu appears to be beyond resource "
							  "pack bounds (`%zu >= %zu')", i, res->offset,
					pack->size);
			result = AGA_RESULT_BAD_PARAM;
			goto cleanup;
		}

		result = aga_conftree(node, &size, 1, &res->size, AGA_INTEGER);
		if(result) {
			aga_soft(__FILE__, "aga_conftree", result);
			aga_log(
					__FILE__, "Resource #%zu appears to be missing a size "
							  "entry", i);
			continue;
		}

		if(res->offset + res->size >= pack->size) {
			aga_log(
					__FILE__, "Resource #%zu appears to be beyond resource "
							  "pack bounds (`%zu + %zu >= %zu')", i,
					res->offset, res->size, pack->size);
			result = AGA_RESULT_BAD_PARAM;
			goto cleanup;
		}
	}

	aga_log(__FILE__, "Loaded `%zu' resource entries", pack->len);

	return AGA_RESULT_OK;

	cleanup: {
		aga_free(pack->db);

		if(pack->fp && fclose(pack->fp) == EOF) {
			return aga_errno(__FILE__, "fclose");
		}

		if(c) {
			enum aga_result res2 = aga_killconf(&pack->root);
			if(res2) return res2;
		}

		return result;
	}
}

enum aga_result aga_killrespack(struct aga_respack* pack) {
	enum aga_result result;

	if(!pack) return AGA_RESULT_BAD_PARAM;

	if((result = aga_sweeprespack(pack))) return result;

#ifdef _DEBUG
	if(pack->outstanding_refs) {
		aga_log(
				__FILE__, "warn: `%zu' outstanding refs held in freed respack",
				pack->outstanding_refs);
	}
#endif

	aga_free(pack->db);
	if(fclose(pack->fp) == EOF) return aga_errno(__FILE__, "fclose");

	return aga_killconf(&pack->root);
}

enum aga_result aga_sweeprespack(struct aga_respack* pack) {
	aga_size_t i;

	if(!pack) return AGA_RESULT_BAD_PARAM;

	for(i = 0; i < pack->len; ++i) {
		struct aga_res* res = &pack->db[i];

		if(res->refcount || !res->data) continue;

#ifdef _DEBUG
		pack->outstanding_refs--;
#endif

		aga_free(res->data);
		res->data = 0;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_mkres(
		struct aga_respack* pack, const char* path, struct aga_res** res) {

	enum aga_result result;

	if(!path) return AGA_RESULT_BAD_PARAM;
	if(!pack) return AGA_RESULT_BAD_PARAM;
	if(!res) return AGA_RESULT_BAD_PARAM;

	result = aga_searchres(pack, path, res);
	if(result) return result;

	if(!(*res)->data) {
		result = aga_resseek(*res, 0);
		if(result) return result;

#ifdef _DEBUG
		pack->outstanding_refs++;
#endif

		/* TODO: Use mapping for large reads. */
		if(!((*res)->data = aga_malloc((*res)->size))) return AGA_RESULT_OOM;

		result = aga_fread((*res)->data, (*res)->size, pack->fp);
		if(result) return result;
	}

	++(*res)->refcount;

	return AGA_RESULT_OK;
}

enum aga_result aga_resfptr(
		struct aga_respack* pack, const char* path, void** fp,
		aga_size_t* size) {

	enum aga_result result;
	struct aga_res* res;

	if(!pack) return AGA_RESULT_BAD_PARAM;
	if(!path) return AGA_RESULT_BAD_PARAM;

	result = aga_searchres(pack, path, &res);
	if(result) return result;

	result = aga_resseek(res, 0);
	if(result) return result;

	*fp = pack->fp;
	*size = res->size;

	return AGA_RESULT_OK;
}

enum aga_result aga_resseek(struct aga_res* res, void** fp) {
	int result;
	aga_size_t offset;

	if(!res) return AGA_RESULT_BAD_PARAM;

	offset = res->pack->data_offset + res->offset;

	result = fseek(res->pack->fp, (long) offset, SEEK_SET);
	if(result == -1) return aga_errno(__FILE__, "fseek");

	if(fp) *fp = res->pack->fp;

	return AGA_RESULT_OK;
}

enum aga_result aga_acquireres(struct aga_res* res) {
	if(!res) return AGA_RESULT_BAD_PARAM;

	++res->refcount;

	return AGA_RESULT_OK;
}

enum aga_result aga_releaseres(struct aga_res* res) {
	if(!res) return AGA_RESULT_BAD_PARAM;

	if(res->refcount) --res->refcount;

	return AGA_RESULT_OK;
}
