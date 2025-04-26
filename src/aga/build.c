/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#define AGA_RAW_SUFFIX (".raw")

#define AGA_PY_TAIL ("\n\xFF")

/* TODO: Pass this through properly to `aga_build_X'. */
#define AGA_BUILD_FNAME ("<build>")

#ifdef AGA_DEVBUILD
/* We need the `va_list' stream procs. */
# include <asys/varargs.h>

# include <aga/build.h>
# include <aga/startup.h>
# include <aga/pack.h>

# include <asys/log.h>
# include <asys/stream.h>
# include <asys/file.h>
# include <asys/memory.h>
# include <asys/string.h>

/* TODO: For `struct vertex' definition -- move elsewhere. */
# include <agan/object.h>

# include <glm.h>
# include <tiffio.h>

enum aga_file_kind {
	AGA_KIND_NONE = 0,

	AGA_KIND_RAW,

	AGA_KIND_TIFF,
	AGA_KIND_OBJ,
	/*
	 * TODO: We can precompile a more efficiently serialised conftree in
	 * 		 Non-Debug/Dev builds.
	 */
	AGA_KIND_SGML,
	AGA_KIND_PY,
	AGA_KIND_WAV,
	AGA_KIND_MIDI
};

struct aga_build_conf_pass {
	struct asys_stream* stream;
	enum aga_file_kind kind;
	asys_size_t offset;
};

typedef enum asys_result (*aga_build_input_fn_t)(
		struct asys_stream*, struct asys_stream*);

typedef enum asys_result (*aga_input_iterfn_t)(
		const char*, enum aga_file_kind, asys_bool_t, void*);

static enum asys_result aga_build_open_config(
		const char* path, struct aga_config_node* root) {

	enum asys_result result;

	struct asys_stream stream;

	result = asys_stream_new(&stream, path);
	if(result) return result;

	result = aga_config_new(&stream, AGA_CONFIG_EOF, root);
	if(result) goto cleanup;

	result = asys_stream_delete(&stream);
	if(result) goto cleanup;

	return ASYS_RESULT_OK;

	cleanup: {
		asys_log_result(
				__FILE__, "asys_stream_delete", asys_stream_delete(&stream));

		asys_log_result(
				__FILE__, "aga_config_delete", aga_config_delete(root));

		return result;
	}
}

static enum asys_result aga_build_open_output(
		struct aga_config_node* root, struct asys_stream* stream,
		char** out_path) {

	static const char* output = "Output";

	enum asys_result result;
	char* path;

	result = aga_config_lookup(
			root->children, &output, 1, &path, AGA_PATH, ASYS_FALSE);

	if(result) {
		asys_log(
				__FILE__,
				"warn: No output file specified, defaulting to `agapack.raw'");

		path = "agapack.raw";
	}

	*out_path = path;

	asys_log(__FILE__, "Writing output file `%s'...", path);

	result = asys_stream_new_write(stream, path);
	if(result) return result;

	return ASYS_RESULT_OK;
}

static asys_bool_t aga_build_path_matches_kind(
		const char* path, enum aga_file_kind kind) {

	/*
	 * TODO: Reduce all paths to within basic FAT restrictions -- including our
	 * 		 Source tree!
	 */
	static const char* kind_exts[] = {
			"", /* AGA_KIND_NONE */
			".raw", /* AGA_KIND_RAW */
			".tiff", /* AGA_KIND_TIFF */
			".obj", /* AGA_KIND_OBJ */
			".sgml", /* AGA_KIND_SGML */
			".py", /* AGA_KIND_PY */
			".wav", /* AGA_KIND_WAV */
			".mid" /* AGA_KIND_MIDI */
	};

	/* TODO: `strcasecmp'? */
	const char* ext = asys_string_find_const(path, '.');
	if(!ext || !asys_string_equal(ext, kind_exts[kind])) return ASYS_FALSE;

	return ASYS_TRUE;
}

static enum asys_result aga_fprintf_add(
		struct asys_stream* stream, asys_size_t indent,
		const char* format, ...) {

	enum asys_result result;

	va_list list;

	result = asys_stream_write_characters(stream, '\t', indent);
	if(result) return result;

	va_start(list, format);

	result = asys_stream_write_format_variadic(stream, format, list);

	va_end(list);

	return result;
}

static enum asys_result aga_build_python(
		struct asys_stream* out, struct asys_stream* in) {

	enum asys_result result;

	if((result = asys_stream_splice(out, in, ASYS_COPY_ALL))) return result;

	return asys_stream_write(out, AGA_PY_TAIL, sizeof(AGA_PY_TAIL) - 1);
}

static enum asys_result aga_build_obj(
		struct asys_stream* out, struct asys_stream* in) {

	enum asys_result result;

	GLMmodel* model;
	float extent[6];

	unsigned i, j;
	GLMgroup* group;

	void* stdc_handle;

	/* TODO: This needs to be closed. */
	stdc_handle = asys_stream_stdc(in);
	if(!stdc_handle) return ASYS_RESULT_NOT_IMPLEMENTED;

	/* TODO: Handle different EH. */
	model = glmReadOBJFile(AGA_BUILD_FNAME, asys_stream_stdc(in));
	if(!model) return ASYS_RESULT_OOM;

	/* TODO: Put this epsilon somewhere configurable. */
	/* TODO: This hangs? (Or takes a *really* long time on sponza or smth.) */
	/* glmWeld(model, 0.0001f); */
	glmExtent(model, extent);

	group = model->groups;
	while(group) {
		const GLMtriangle* tris = model->tris;
		const GLMmaterial* mats = model->mats;
		const float* norms = model->norms;
		const float* uvs = model->uvs;
		const float* verts = model->verts;

		for(i = 0; i < group->ntris; i++) {
			const GLMtriangle* t = &tris[group->tris[i]];
			const GLMmaterial* mat = &mats[group->material];

			struct aga_vertex v = { 0 };

			if(mat) asys_memory_copy(v.col, mat->diffuse, sizeof(float[4]));

			for(j = 0; j < 3; ++j) {
				asys_memory_copy(
						v.uv, &uvs[2 * t->t_inds[j]], sizeof(float[2]));

				asys_memory_copy(
						v.norm, &norms[3 * t->n_inds[j]], sizeof(float[3]));

				asys_memory_copy(
						v.pos, &verts[3 * t->v_inds[j]], sizeof(float[3]));

				result = asys_stream_write(out, &v, sizeof(struct aga_vertex));
				if(result) goto cleanup;
			}
		}

		group = group->next;
	}

	result = asys_stream_write(out, &extent, sizeof(float[6]));

	cleanup: glmDelete(model);

	return result;

}

static enum asys_result aga_build_tiff(
		struct asys_stream* out, struct asys_stream* in) {

	/* NOTE: TIFF wants this -- this is kind of evil. */
	static char msg[1024];

	enum asys_result result = ASYS_RESULT_OK;

	asys_stream_native_t native = asys_stream_native(in);

	TIFF* tiff;
	TIFFRGBAImage img = { 0 };

	asys_size_t size;
	void* raster = 0;

	if(!(tiff = TIFFFdOpen(native, AGA_BUILD_FNAME, "r"))) {
		return ASYS_RESULT_ERROR;
	}

	if(!TIFFRGBAImageBegin(&img, tiff, 0, msg)) {
		asys_log(__FILE__, "err: Failed to read TIFF file: %s", msg);
		result = ASYS_RESULT_ERROR;
		goto cleanup;
	}

	size = 4 * img.width * img.height;

	if(!(raster = asys_memory_allocate(size))) {
		result = ASYS_RESULT_OOM;
		goto cleanup;
	}

	if(!TIFFRGBAImageGet(&img, raster, img.width, img.height)) {
		result = ASYS_RESULT_ERROR;
		goto cleanup;
	}

	result = asys_stream_write(out, raster, size);
	if(result) goto cleanup;

	result = asys_stream_write(out, &img.width, sizeof(uint32));
	if(result) goto cleanup;

	cleanup: {
		asys_memory_free(raster);
		TIFFRGBAImageEnd(&img);
		TIFFClose(tiff, 0);
	}

	return result;
}

static enum asys_result aga_build_input_file(
		const char* path, enum aga_file_kind kind) {

	static asys_fixed_buffer_t buffer = { 0 };

	enum asys_result result;

	aga_build_input_fn_t fn;
	struct asys_stream in, out;

	asys_bool_t older;

	/*
	 * Input kinds which are handled as "raw" need a special case when
	 * Looking for the resultant artefact files -- we just redirect to the
	 * Original because there is no need to produce any output whatsoever.
	 */
	if(kind == AGA_KIND_SGML || kind == AGA_KIND_RAW) return ASYS_RESULT_OK;

	/* Skip input files which don't match kind. */
	if(!aga_build_path_matches_kind(path, kind)) return ASYS_RESULT_OK;

	buffer[0] = 0;

	asys_string_concatenate(buffer, path);
	asys_string_concatenate(buffer, AGA_RAW_SUFFIX);

	/*
	 * TODO: On Windows this requires opening streams -- should we have a way
	 * 		 To keep streams alive for input assets in general?
	 */
	result = asys_path_older(path, buffer, &older);
	/* If we can't determine the age of the files -- rebuild anyway. */
	asys_log_result(__FILE__, "asys_path_older", result);

	/* TODO: Flag to force rebuilds. */
	if(older) return ASYS_RESULT_OK;

	result = asys_stream_new(&in, path);
	if(result) return result;

	result = asys_stream_new_write(&out, buffer);
	if(result) goto cleanup;

	switch(kind) {
		default: {
			asys_log(
					__FILE__,
					"err: Unknown or unimplemented input kind for `%s'", path);

			goto cleanup;
		}

		case AGA_KIND_PY: fn = aga_build_python; break;
		case AGA_KIND_OBJ: fn = aga_build_obj; break;
		case AGA_KIND_TIFF: fn = aga_build_tiff; break;

/*
		case AGA_KIND_WAV: break;
		case AGA_KIND_MIDI: break;
 */
	}

	result = fn(&out, &in);
	if(result) goto cleanup;

	result = asys_stream_delete(&in);
	if(result) goto cleanup;

	return asys_stream_delete(&out);

	cleanup: {
		asys_log_result(
				__FILE__, "asys_stream_delete", asys_stream_delete(&in));

		asys_log_result(
				__FILE__, "asys_stream_delete", asys_stream_delete(&out));

		return result;
	}
}

static enum asys_result aga_build_input_dir(const char* path, void* pass) {
	enum aga_file_kind* kind = pass;

	return aga_build_input_file(path, *kind);
}

static enum asys_result aga_build_input(
		const char* path, enum aga_file_kind kind, asys_bool_t recurse,
		void* pass) {

	enum asys_result result;
	union asys_file_attribute attribute;

	(void) pass;

	result = asys_path_attribute(path, ASYS_FILE_TYPE, &attribute);
	if(result) return result;

	if(attribute.type == ASYS_FILE_DIRECTORY) {
		return asys_path_iterate(
				path, aga_build_input_dir, recurse, &kind, ASYS_TRUE);
	}
	else return aga_build_input_file(path, kind);
}

static enum asys_result aga_build_conf_file(
		const char* path, enum aga_file_kind kind, struct asys_stream* stream,
		asys_size_t* offset) {

	static asys_fixed_buffer_t buffer;
	static asys_float_format_buffer_t double_format;
#ifdef ASYS_WIN32
	static asys_fixed_buffer_t standard_path;
	asys_size_t i;
#endif

	enum asys_result result;

	union asys_file_attribute attr;

	/* Skip input files which don't match kind. */
	if(!aga_build_path_matches_kind(path, kind)) return ASYS_RESULT_OK;

	buffer[0] = 0;
	asys_string_concatenate(buffer, path);

	/* Use the base file as the input for SGML/RAW inputs. */
	if(kind != AGA_KIND_SGML && kind != AGA_KIND_RAW) {
		asys_string_concatenate(buffer, AGA_RAW_SUFFIX);
	}

#ifdef ASYS_WIN32
	/* TODO: Make a function for this. */
	for(i = 0; buffer[i]; ++i) {
		if(buffer[i] == '\\') standard_path[i] = '/';
		else standard_path[i] = buffer[i];
	}

	standard_path[i] = 0;

	result = aga_fprintf_add(stream, 1, "<item name=\"%s\">\n", standard_path);
	if(result) return result;
#else
	result = aga_fprintf_add(stream, 1, "<item name=\"%s\">\n", buffer);
	if(result) return result;
#endif

	/* Unpleasant but neccesary. */
#define agab_(indent, name, type, format, parameter) \
		do { \
			result = aga_fprintf_add( \
					stream, indent, \
					"<item name=\"%s\" type=\"%s\">\n", name, type); \
			if(result) return result; \
			\
			result = aga_fprintf_add( \
					stream, indent + 1, format "\n", parameter); \
			if(result) return result; \
			\
			result = aga_fprintf_add(stream, indent, "</item>\n"); \
			if(result) return result; \
		} while(0)

#define agaf_(indent, name, value) \
		do { \
			result = aga_fprintf_add( \
					stream, indent, \
					"<item name=\"%s\" type=\"Float\">\n", name); \
			if(result) return result; \
			\
			result = asys_float_to_string(value, &double_format); \
			if(result) return result; \
			\
			result = aga_fprintf_add( \
					stream, indent + 1, "%s", double_format); \
			if(result) return result; \
			\
			result = aga_fprintf_add(stream, indent, "</item>\n"); \
			if(result) return result; \
		} while(0)

	{
		agab_(2, "Offset", "Integer", ASYS_NATIVE_ULONG_FORMAT, *offset);

		/*
		 * TODO: If we build as we go we can avoid loads of file open/close
		 * 		 Operations and entirely eliminate file length gets.
		 */
		result = asys_path_attribute(buffer, ASYS_FILE_LENGTH, &attr);
		if(result) return result;

		*offset += attr.length;
		agab_(2, "Size", "Integer", ASYS_NATIVE_ULONG_FORMAT, attr.length);

		switch(kind) {
			default: break;

			/*
			 * TODO: More formally document these "tails" in a comment at the
			 * 		 Top of this file.
			 */
			case AGA_KIND_TIFF: {
				aga_image_tail_t tail;

				result = asys_path_tail(buffer, &tail, sizeof(tail));
				if(result) return result;

				agab_(2, "Width", "Integer", "%u", tail);

				break;
			}

			case AGA_KIND_OBJ: {
				aga_model_tail_t tail;

				result = asys_path_tail(buffer, tail, sizeof(tail));
				if(result) return result;

				agaf_(2, "MinX", tail[0]);
				agaf_(2, "MinY", tail[1]);
				agaf_(2, "MinZ", tail[2]);
				agaf_(2, "MaxX", tail[3]);
				agaf_(2, "MaxY", tail[4]);
				agaf_(2, "MaxZ", tail[5]);

				/*
				 * Mark model as version 2 -- we started discarding model
				 * vertex colouration.
				 */
				agab_(2, "Version", "Integer", "%u", 2);

				break;
			}
		}
	}
#undef agab_

	result = aga_fprintf_add(stream, 1, "</item>\n"); \
	if(result) return result;

	return ASYS_RESULT_OK;
}

static enum asys_result aga_build_conf_dir(const char* path, void* pass) {
	struct aga_build_conf_pass* conf_pass = pass;

	return aga_build_conf_file(
			path, conf_pass->kind, conf_pass->stream, &conf_pass->offset);
}

static enum asys_result aga_build_conf(
		const char* path, enum aga_file_kind kind, asys_bool_t recurse,
		void* pass) {

	enum asys_result result;
	union asys_file_attribute attr;

	struct aga_build_conf_pass* conf_pass = pass;

	if((result = asys_path_attribute(path, ASYS_FILE_TYPE, &attr))) {
		return result;
	}

	if(attr.type == ASYS_FILE_DIRECTORY) {
		conf_pass->kind = kind;

		return asys_path_iterate(
				path, aga_build_conf_dir, recurse, conf_pass, ASYS_TRUE);
	}
	else {
		return aga_build_conf_file(
			path, kind, conf_pass->stream, &conf_pass->offset);
	}
}

static enum asys_result aga_build_pack_file(
		const char* path, enum aga_file_kind kind,
		struct asys_stream* stream) {

	static asys_fixed_buffer_t buffer = { 0 };

	enum asys_result result;

	struct asys_stream in;

	/* Skip input files which don't match kind. */
	if(!aga_build_path_matches_kind(path, kind)) return ASYS_RESULT_OK;

	buffer[0] = 0;

	asys_string_concatenate(buffer, path);

	/* Use the base file as the input for SGML/RAW inputs. */
	if(kind != AGA_KIND_SGML && kind != AGA_KIND_RAW) {
		asys_string_concatenate(buffer, AGA_RAW_SUFFIX);
	}

	/* TODO: Should we have a path-wise copy? */
	result = asys_stream_new(&in, buffer);
	if(result) return result;

	result = asys_stream_splice(stream, &in, ASYS_COPY_ALL);
	if(result) goto cleanup;

	result = asys_stream_delete(&in);
	if(result) return result;

	return ASYS_RESULT_OK;

	cleanup: {
		asys_log_result(
				__FILE__, "asys_stream_delete", asys_stream_delete(&in));

		return result;
	}
}

static enum asys_result aga_build_pack_dir(const char* path, void* pass) {
	struct aga_build_conf_pass* conf_pass = pass;

	return aga_build_pack_file(path, conf_pass->kind, conf_pass->stream);
}

static enum asys_result aga_build_pack(
		const char* path, enum aga_file_kind kind, asys_bool_t recurse,
		void* pass) {

	enum asys_result result;
	union asys_file_attribute attr;

	if((result = asys_path_attribute(path, ASYS_FILE_TYPE, &attr))) {
		return result;
	}

	if(attr.type == ASYS_FILE_DIRECTORY) {
		struct aga_build_conf_pass conf_pass;

		conf_pass.stream = pass;
		conf_pass.kind = kind;

		return asys_path_iterate(
				path, aga_build_pack_dir, recurse, &conf_pass, ASYS_TRUE);
	}
	else return aga_build_pack_file(path, kind, pass);
}

static enum asys_result aga_build_iter(
		struct aga_config_node* input_root, asys_bool_t log,
		aga_input_iterfn_t fn, void* pass) {

	enum asys_result result;
	enum asys_result held_result = ASYS_RESULT_OK;

	asys_size_t i, j;

	for(i = 0; i < input_root->len; ++i) {
		struct aga_config_node* node = &input_root->children[i];

		aga_config_int_t v;
		const char* str = 0;

		enum aga_file_kind kind = AGA_KIND_NONE;
		char* path = 0;
		asys_bool_t recurse = ASYS_FALSE;

		for(j = 0; j < node->len; ++j) {
			struct aga_config_node* child = &node->children[j];

			if(aga_config_variable("Kind", child, AGA_STRING, &str)) {
				if(asys_string_equal(str, "RAW")) kind = AGA_KIND_RAW;
				else if(asys_string_equal(str, "TIFF")) kind = AGA_KIND_TIFF;
				else if(asys_string_equal(str, "OBJ")) kind = AGA_KIND_OBJ;
				else if(asys_string_equal(str, "SGML")) kind = AGA_KIND_SGML;
				else if(asys_string_equal(str, "PY")) kind = AGA_KIND_PY;
				else if(asys_string_equal(str, "WAV")) kind = AGA_KIND_WAV;
				else if(asys_string_equal(str, "MIDI")) kind = AGA_KIND_MIDI;
				else {
					asys_log(
							__FILE__,
							"warn: Unknown input kind `%s' -- Assuming `RAW'",
							str);

					kind = AGA_KIND_RAW;
				}

				continue;
			}
			else if(aga_config_variable("Path", child, AGA_PATH, &path)) {
				continue;
			}
			else if(aga_config_variable("Recurse", child, AGA_INTEGER, &v)) {
				recurse = !!v;
				continue;
			}
		}

		if(log) {
			asys_log(
					__FILE__,
					"Build Input: Path=\"%s\" Kind=%s Recurse=%s",
					path, str, recurse ? "True" : "False");
		}

		if((result = fn(path, kind, recurse, pass))) {
			asys_log_result(
					__FILE__, "aga_build_iter::<callback>", result);

			held_result = result;
		}

		asys_memory_free(path);
	}

	return held_result;
}

static void aga_tiff_handler(
		asys_bool_t warning, const char* module, const char* format,
		va_list list) {

	static asys_fixed_buffer_t buffer = { 0 };

	enum asys_result result;

	const char* prefix = warning ? "warn" : "err";
	const char* module_tag = module ? module : "<none>";

	result = asys_string_format_variadic(&buffer, 0, format, list);
	if(result) {
		asys_log_result(
				__FILE__, "asys_string_format_variadic", result);
	}

	asys_log(__FILE__, "%s: %s: %s", prefix, module_tag, buffer);
}

static void aga_tiff_error(
		const char* module, const char* format, va_list list) {

	aga_tiff_handler(ASYS_FALSE, module, format, list);
}

static void aga_tiff_warning(
		const char* module, const char* format, va_list list) {

	aga_tiff_handler(ASYS_TRUE, module, format, list);
}

/* TODO: Lots of leaky error states in here and the above statics. */
/*
 * TODO: Extra verbose per-file output if set to verbose output. Make
 * 		 Verbose outputs use a `verb:' logging tag so the logger can filter
 * 		 Based on the `-v' flag instead of needing to check everywhere.
 */
enum asys_result aga_build(struct aga_settings* opts) {
	static const char* input = "Input";

	enum asys_result result;

	struct aga_config_node root;
	struct aga_config_node* input_root;

	struct asys_stream stream = { 0 };
	char* out_path = 0;

	asys_log(__FILE__, "Compiling project `%s'...", opts->build_file);

	TIFFSetErrorHandler(aga_tiff_error);
	TIFFSetWarningHandler(aga_tiff_warning);

	if((result = aga_build_open_config(opts->build_file, &root))) {
		return result;
	}

	result = aga_config_lookup_check(root.children, &input, 1, &input_root);
	if(result) {
		asys_log(__FILE__, "err: No input files specified");
		goto cleanup;
	}

	if((result = aga_build_iter(input_root, ASYS_TRUE, aga_build_input, 0))) {
		goto cleanup;
	}

	result = aga_build_open_output(&root, &stream, &out_path);
	if(result) goto cleanup;

	asys_log(__FILE__, "Building pack directory...");

	{
		struct aga_build_conf_pass conf_pass;

		struct aga_resource_pack_header header = { 0, AGA_PACK_MAGIC };
		asys_offset_t offset;

		conf_pass.stream = &stream;
		conf_pass.offset = 0;

		result = asys_stream_write(
				&stream, &header, sizeof(struct aga_resource_pack_header));

		if(result) goto cleanup;

		/* TODO: CLI option to emit this separately for inspection. */
		result = asys_stream_write_format(&stream, "<root>\n");
		if(result) goto cleanup;

		result = aga_build_iter(
				input_root, ASYS_FALSE, aga_build_conf, &conf_pass);

		if(result) goto cleanup;

		result = asys_stream_write_format(&stream, "</root>\n");
		if(result) goto cleanup;

		result = asys_stream_tell(&stream, &offset);
		if(result) goto cleanup;

		header.size =
			(asys_uint_t) offset - sizeof(struct aga_resource_pack_header);

		result = asys_stream_seek(&stream, ASYS_SEEK_SET, 0);
		if(result) goto cleanup;

		result = asys_stream_write(
				&stream, &header, sizeof(struct aga_resource_pack_header));

		if(result) goto cleanup;

		result = asys_stream_seek(&stream, ASYS_SEEK_SET, offset);
		if(result) goto cleanup;
	}

	asys_log(__FILE__, "Inserting file data...");

	result = aga_build_iter(input_root, ASYS_FALSE, aga_build_pack, &stream);
	if(result) goto cleanup;

	result = asys_stream_delete(&stream);
	if(result) goto cleanup;

	if((result = aga_config_delete(&root))) return result;

	asys_memory_free(out_path);

	asys_log(__FILE__, "Done!");

	return ASYS_RESULT_OK;

	cleanup: {
		/* TODO: Add option to clean built files. */
		/* TODO: Destroy failed intermediate files in error cases as well. */

		/*
		 * NOTE: `out_path' may reside in `root' so this needs to be before
		 * 		 `aga_config_delete'.
		 */
		if(out_path) {
			asys_log_result(
					__FILE__, "asys_path_remove", asys_path_remove(out_path));
		}

		asys_log_result(
				__FILE__, "aga_config_delete", aga_config_delete(&root));

		asys_log_result(
				__FILE__, "asys_stream_delete", asys_stream_delete(&stream));

		asys_memory_free(out_path);

		asys_log(__FILE__, "err: Build failed");

		return result;
	}
}

#else
# include <aga/build.h>

# include <asys/log.h>

enum asys_result aga_build(struct aga_settings* opts) {
	(void) opts;

	asys_log(
		__FILE__, "err: Project building is only supported in dev builds");

	return ASYS_RESULT_ERROR;
}

#endif
