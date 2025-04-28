/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/pack.h>
#include <aga/startup.h>

#include <asys/log.h>
#include <asys/memory.h>
#include <asys/string.h>

/*
 * TODO: Allow inplace use of UNIX `compress'/`uncompress' utilities on pack
 * 		 In distribution.
 * 		 Windows apparently had `COMPRESS.EXE' and could use `LZRead' etc. for
 * 		 In-place compressed reads (!).
 */

enum asys_result aga_resource_pack_lookup(
		struct aga_resource_pack* pack, const char* path,
		struct aga_resource** out) {

	asys_size_t i;

	if(!pack) return ASYS_RESULT_BAD_PARAM;
	if(!path) return ASYS_RESULT_BAD_PARAM;
	if(!out) return ASYS_RESULT_BAD_PARAM;

	for(i = 0; i < pack->count; ++i) {
		struct aga_resource* resource = &pack->resources[i];
		const char* name;

		if(!resource->config) continue;

		name = resource->config->name;

		if(asys_string_equal(name, path)) {
			*out = &pack->resources[i];
			return ASYS_RESULT_OK;
		}
#ifdef ASYS_WIN32
		/* TODO: Should pack paths be pre-transformed? */
		else {
			asys_size_t j;

			for(j = 0; path[j] && name[j]; ++j) {
				if(path[j] == name[j]) continue;
				else if(path[j] == '\\' && name[j] == '/') continue;
				else break;
			}

			if(path[j] == name[j]) {
				*out = &pack->resources[i];
				return ASYS_RESULT_OK;
			}
		}
#endif
	}

	asys_log(__FILE__, "err: Path `%s' not found in resource pack", path);

	return ASYS_RESULT_MISSING_KEY;
}

enum asys_result aga_resource_pack_new(
		const char* path, struct aga_resource_pack* pack, struct aga_settings* opts) {

	enum asys_result result;

	asys_size_t i;
	struct aga_resource_pack_header header;

	if(!path) return ASYS_RESULT_BAD_PARAM;
	if(!pack) return ASYS_RESULT_BAD_PARAM;

	asys_memory_zero(pack, sizeof(struct aga_resource_pack));
	asys_memory_zero(&pack->root, sizeof(struct aga_config_node));

#ifdef AGA_PACK_DEBUG
	pack->outstanding_refs = 0;
#endif

	asys_log(__FILE__, "Loading resource pack `%s'...", path);

	if((result = asys_stream_new(&pack->stream, path))) return result;

	result = asys_stream_read(
			&pack->stream, 0, &header,
			sizeof(struct aga_resource_pack_header));

	if(result) goto cleanup;

	if(header.magic != AGA_PACK_MAGIC) {
		asys_log(
				__FILE__,
				"Invalid resource pack magic. Expected `%x', got `%x'",
				AGA_PACK_MAGIC, header.magic);

		result = ASYS_RESULT_BAD_PARAM;
		goto cleanup;
	}

	/*
	 * TODO: Needing to keep the entire resource pack header config loaded
	 * 		 Is not a terribly efficient use of our memory.
	 * 		 Can we avoid/reduce this?
	 */
	result = aga_config_new(&pack->stream, header.size, &pack->root);
	if(result) goto cleanup;

	if(pack->root.len) pack->count = pack->root.children->len;
	else asys_log(__FILE__, "warn: Resource pack appears to be empty");

	pack->data_offset = header.size + sizeof(header);

	pack->resources = asys_memory_allocate_zero(
			pack->count, sizeof(struct aga_resource));

	if(!pack->resources) {
		result = ASYS_RESULT_OOM;
		goto cleanup;
	}

	for(i = 0; i < pack->count; ++i) {
		static const char* offset_name = "Offset";
		static const char* size_name = "Size";

		struct aga_resource* resource = &pack->resources[i];
		struct aga_config_node* node = &pack->root.children->children[i];

		aga_config_int_t v;

		result = aga_config_lookup(
				node, &offset_name, 1, &v, AGA_INTEGER, ASYS_TRUE);

		if(result) continue;
		resource->offset = (asys_offset_t) v;

		result = aga_config_lookup(
				node, &size_name, 1, &v, AGA_INTEGER, ASYS_TRUE);

		if(result) continue;
		resource->size = (asys_size_t) v;

		/* Only make a valid resource entry once all checks have passed. */
		resource->config = node;
		resource->pack = pack;

		/*
		 * TODO: Should we automatically disable trace log output based on
		 * 		 Verbosity from log?
		 */
		if(opts->verbose) {
			asys_log(
					__FILE__,
					"trace: Added resource entry `%s` (@"
					ASYS_NATIVE_LONG_FORMAT ", " ASYS_NATIVE_ULONG_FORMAT ")",
					asys_string_optional(node->name),
					resource->offset, resource->size);
		}
	}

	pack->opts = opts;

	asys_log(
			__FILE__,
			"Processed `" ASYS_NATIVE_ULONG_FORMAT "' resource entries",
			pack->count);

	return ASYS_RESULT_OK;

	cleanup: {
		asys_memory_free(pack->resources);

		asys_log_result(
				__FILE__, "asys_stream_delete",
				asys_stream_delete(&pack->stream));

		asys_log_result(
				__FILE__, "aga_config_delete",
				aga_config_delete(&pack->root));

		return result;
	}
}

enum asys_result aga_resource_pack_delete(struct aga_resource_pack* pack) {
	enum asys_result result;

	if(!pack) return ASYS_RESULT_BAD_PARAM;

	if((result = aga_resource_pack_sweep(pack))) return result;

#ifdef AGA_PACK_DEBUG
	if(pack->outstanding_refs) {
		asys_log(
				__FILE__,
				"warn: `" ASYS_NATIVE_ULONG_FORMAT "' outstanding refs held in"
				" freed respack", pack->outstanding_refs);
	}
#endif

	asys_memory_free(pack->resources);

	result = asys_stream_delete(&pack->stream);
	if(result) {
		asys_log_result(
				__FILE__, "aga_config_delete", aga_config_delete(&pack->root));

		return result;
	}

	return aga_config_delete(&pack->root);
}

enum asys_result aga_resource_pack_sweep(struct aga_resource_pack* pack) {
	asys_size_t i;
	asys_size_t cleared = 0;

	if(!pack) return ASYS_RESULT_BAD_PARAM;

	for(i = 0; i < pack->count; ++i) {
		struct aga_resource* resource = &pack->resources[i];

		if(resource->refcount || !resource->data) continue;

		cleared++;

#ifdef AGA_PACK_DEBUG
		pack->outstanding_refs--;
#endif

		asys_memory_free(resource->data);
		resource->data = 0;

		if(pack->opts->verbose) {
			asys_log(
				__FILE__,
				"trace: Cleared resource entry `%s'", resource->config->name);
		}
	}

	if(cleared) {
		asys_log(
			__FILE__,
			"Sweep cleared `" ASYS_NATIVE_ULONG_FORMAT "' resources...",
			cleared);
	}

	return ASYS_RESULT_OK;
}

enum asys_result aga_resource_new(
		struct aga_resource_pack* pack, const char* path,
		struct aga_resource** resource) {

	enum asys_result result;

	if(!path) return ASYS_RESULT_BAD_PARAM;
	if(!pack) return ASYS_RESULT_BAD_PARAM;
	if(!resource) return ASYS_RESULT_BAD_PARAM;

	result = aga_resource_pack_lookup(pack, path, resource);
	if(result) {
		asys_log(__FILE__, "err: Failed to find resource `%s'", path);
		return result;
	}

	if(!(*resource)->data) {
		result = aga_resource_seek(*resource, 0);
		if(result) return result;

#ifdef AGA_PACK_DEBUG
		pack->outstanding_refs++;
#endif

		/*
		 * TODO: Use mapping for large reads (user configurable as mapping is
		 * 		 Suboptimal for linear media). */
		(*resource)->data = asys_memory_allocate((*resource)->size);
		if(!(*resource)->data) return ASYS_RESULT_OOM;

		result = asys_stream_read(
				&pack->stream, 0, (*resource)->data, (*resource)->size);

		if(result) return result;
	}

	++(*resource)->refcount;

	return ASYS_RESULT_OK;
}

enum asys_result aga_resource_stream(
		struct aga_resource_pack* pack, const char* path,
		struct asys_stream** stream, asys_size_t* size) {

	enum asys_result result;
	struct aga_resource* resource;

	if(!pack) return ASYS_RESULT_BAD_PARAM;
	if(!path) return ASYS_RESULT_BAD_PARAM;

	result = aga_resource_pack_lookup(pack, path, &resource);
	if(result) return result;

	result = aga_resource_seek(resource, 0);
	if(result) return result;

	*stream = &pack->stream;
	*size = resource->size;

	return ASYS_RESULT_OK;
}

enum asys_result aga_resource_seek(
		struct aga_resource* resource, struct asys_stream** stream) {

	int result;
	asys_offset_t offset;

	if(!resource) return ASYS_RESULT_BAD_PARAM;

	offset = (asys_offset_t) (resource->pack->data_offset + resource->offset);

	result = asys_stream_seek(&resource->pack->stream, ASYS_SEEK_SET, offset);
	if(result) return result;

	if(stream) *stream = &resource->pack->stream;

	return ASYS_RESULT_OK;
}

enum asys_result aga_resource_aquire(struct aga_resource* res) {
	if(!res) return ASYS_RESULT_BAD_PARAM;

	++res->refcount;

	return ASYS_RESULT_OK;
}

enum asys_result aga_resource_release(struct aga_resource* res) {
	if(!res) return ASYS_RESULT_BAD_PARAM;

	if(res->refcount) --res->refcount;

	return ASYS_RESULT_OK;
}
