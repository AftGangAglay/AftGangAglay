/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/pack.h>
#include <aga/io.h>
#include <aga/error.h>
#include <aga/log.h>
#include <aga/std.h>
#include <aga/utility.h>
#include <aga/script.h>

/*
 * TODO: Allow inplace use of UNIX `compress'/`uncompress' utilities on pack
 * 		 In distribution.
 */

/*
 * TODO: Allow pack input as a "raw" argument and make space for a shebang so
 * 		 Packs can be executed directly by the shell if we're installed on the
 * 		 System.
 */

struct aga_resource_pack* aga_global_pack = 0;

enum aga_result aga_resource_pack_lookup(
		struct aga_resource_pack* pack, const char* path,
		struct aga_resource** out) {

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

enum aga_result aga_resource_pack_new(
		const char* path, struct aga_resource_pack* pack) {

	enum aga_result result;

	aga_size_t i;
	struct aga_resource_pack_header hdr;
	aga_bool_t c = AGA_FALSE;

	union aga_file_attribute attr;

	if(!path) return AGA_RESULT_BAD_PARAM;
	if(!pack) return AGA_RESULT_BAD_PARAM;

	aga_global_pack = pack;

	pack->fp = 0;
	pack->db = 0;
	pack->len = 0;

	aga_memset(&pack->root, 0, sizeof(struct aga_config_node));

#ifndef NDEBUG
	pack->outstanding_refs = 0;
#endif

	aga_log(__FILE__, "Loading resource pack `%s'...", path);

	if(!(pack->fp = fopen(path, "rb"))) {
		return aga_error_system_path(__FILE__, "fopen", path);
	}

	result = aga_file_attribute(pack->fp, AGA_FILE_LENGTH, &attr);
	if(result) goto cleanup;
	pack->size = attr.length;

	result = aga_file_read(&hdr, sizeof(hdr), pack->fp);
	if(result) goto cleanup;

	if(hdr.magic != AGA_PACK_MAGIC) {
		result = AGA_RESULT_BAD_PARAM;
		goto cleanup;
	}

	aga_config_debug_file = path;

	result = aga_config_new(pack->fp, hdr.size, &pack->root);
	if(result) goto cleanup;

	c = AGA_TRUE;

	pack->len = pack->root.children->len;
	pack->data_offset = hdr.size + sizeof(hdr);

	pack->db = aga_calloc(pack->len, sizeof(struct aga_resource));
	if(!pack->db) {
		result = AGA_RESULT_OOM;
		goto cleanup;
	}

	for(i = 0; i < pack->len; ++i) {
		static const char* off = "Offset";
		static const char* sz = "Size";

		struct aga_resource* res = &pack->db[i];
		struct aga_config_node* node = &pack->root.children->children[i];

		aga_slong_t offset;
		aga_slong_t size;

		res->conf = node;
		res->pack = pack;

		result = aga_config_lookup(
				node, &off, 1, &offset, AGA_INTEGER, AGA_FALSE);
		if(result) {
			aga_error_check_soft(__FILE__, "aga_config_lookup", result);
			aga_log(
					__FILE__, "Resource #%zu appears to be missing an offset "
							  "entry", i);
			continue;
		}
		res->offset = offset;

		if(res->offset >= pack->size) {
			aga_log(
					__FILE__, "Resource #%zu appears to be beyond resource "
							  "pack bounds (`%zu >= %zu')", i, res->offset,
					pack->size);
			result = AGA_RESULT_BAD_PARAM;
			goto cleanup;
		}

		result = aga_config_lookup(
				node, &sz, 1, &size, AGA_INTEGER, AGA_FALSE);
		if(result) {
			aga_error_check_soft(__FILE__, "aga_config_lookup", result);
			aga_log(
					__FILE__, "Resource #%zu appears to be missing a size "
							  "entry", i);
			continue;
		}
		res->size = size;

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
			return aga_error_system(__FILE__, "fclose");
		}
		pack->fp = 0;

		if(c) {
			enum aga_result res2 = aga_config_delete(&pack->root);
			if(res2) return res2;
		}

		return result;
	}
}

enum aga_result aga_resource_pack_delete(struct aga_resource_pack* pack) {
	enum aga_result result;

	if(!pack) return AGA_RESULT_BAD_PARAM;

	if((result = aga_resource_pack_sweep(pack))) return result;

#ifndef NDEBUG
	if(pack->outstanding_refs) {
		aga_log(
				__FILE__, "warn: `%zu' outstanding refs held in freed respack",
				pack->outstanding_refs);
	}
#endif

	aga_free(pack->db);
	if(pack->fp && fclose(pack->fp) == EOF) {
		return aga_error_system(__FILE__, "fclose");
	}

	return aga_config_delete(&pack->root);
}

enum aga_result aga_resource_pack_sweep(struct aga_resource_pack* pack) {
	aga_size_t i;

	if(!pack) return AGA_RESULT_BAD_PARAM;

	for(i = 0; i < pack->len; ++i) {
		struct aga_resource* res = &pack->db[i];

		if(res->refcount || !res->data) continue;

#ifndef NDEBUG
		pack->outstanding_refs--;
#endif

		aga_free(res->data);
		res->data = 0;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_resource_new(
		struct aga_resource_pack* pack, const char* path,
		struct aga_resource** res) {

	enum aga_result result;

	if(!path) return AGA_RESULT_BAD_PARAM;
	if(!pack) return AGA_RESULT_BAD_PARAM;
	if(!res) return AGA_RESULT_BAD_PARAM;

	result = aga_resource_pack_lookup(pack, path, res);
	if(result) {
		aga_log(__FILE__, "err: Failed to find resource `%s'", path);
		return result;
	}

	if(!(*res)->data) {
		result = aga_resource_seek(*res, 0);
		if(result) return result;

#ifndef NDEBUG
		pack->outstanding_refs++;
#endif

		/* TODO: Use mapping for large reads. */
		if(!((*res)->data = aga_malloc((*res)->size))) return AGA_RESULT_OOM;

		result = aga_file_read((*res)->data, (*res)->size, pack->fp);
		if(result) return result;
	}

	++(*res)->refcount;

	return AGA_RESULT_OK;
}

enum aga_result aga_resource_stream(
		struct aga_resource_pack* pack, const char* path, void** fp,
		aga_size_t* size) {

	enum aga_result result;
	struct aga_resource* res;

	if(!pack) return AGA_RESULT_BAD_PARAM;
	if(!path) return AGA_RESULT_BAD_PARAM;

	result = aga_resource_pack_lookup(pack, path, &res);
	if(result) return result;

	result = aga_resource_seek(res, 0);
	if(result) return result;

	*fp = pack->fp;
	*size = res->size;

	return AGA_RESULT_OK;
}

enum aga_result aga_resource_seek(struct aga_resource* res, void** fp) {
	int result;
	aga_size_t offset;

	if(!res) return AGA_RESULT_BAD_PARAM;

	offset = res->pack->data_offset + res->offset;

	result = fseek(res->pack->fp, (long) offset, SEEK_SET);
	if(result == -1) return aga_error_system(__FILE__, "fseek");

	if(fp) *fp = res->pack->fp;

	return AGA_RESULT_OK;
}

enum aga_result aga_resource_aquire(struct aga_resource* res) {
	if(!res) return AGA_RESULT_BAD_PARAM;

	++res->refcount;

	return AGA_RESULT_OK;
}

enum aga_result aga_resource_release(struct aga_resource* res) {
	if(!res) return AGA_RESULT_BAD_PARAM;

	if(res->refcount) --res->refcount;

	return AGA_RESULT_OK;
}
