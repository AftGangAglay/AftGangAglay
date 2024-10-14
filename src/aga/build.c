/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/build.h>
#include <aga/startup.h>
#include <aga/log.h>
#include <aga/pack.h>
#include <aga/config.h>
#include <aga/std.h>
#include <aga/error.h>
#include <aga/io.h>
#include <aga/utility.h>

#define AGA_RAWPATH (".raw")
#define AGA_PY_END ("\n\xFF")

#ifdef AGA_DEVBUILD

enum aga_file_kind {
	AGA_KIND_NONE = 0,

	AGA_KIND_RAW,

	AGA_KIND_TIFF,
	AGA_KIND_OBJ,
	AGA_KIND_SGML,
	AGA_KIND_PY,
	AGA_KIND_WAV,
	AGA_KIND_MIDI
};

typedef enum aga_result (*aga_input_iterfn_t)(
		const char*, enum aga_file_kind, aga_bool_t, void*);

static enum aga_result aga_openconf(
		const char* path, struct aga_config_node* root) {

	enum aga_result result;

	void* fp;
	union aga_file_attribute attr;

	if(!(fp = fopen(path, "r"))) {
		return aga_error_system_path(__FILE__, "fopen", path);
	}

	if((result = aga_file_attribute(fp, AGA_FILE_LENGTH, &attr))) return result;

	aga_config_debug_file = path;
	return aga_config_new(fp, attr.length, root);
}

static enum aga_result aga_openout(struct aga_config_node* root, void** fp) {
	static const char* output = "Output";

	enum aga_result result;
	const char* path;

	result = aga_config_lookup(
			root->children, &output, 1, &path, AGA_STRING, AGA_FALSE);
	if(result) {
		aga_log(
				__FILE__,
				"warn: No output file specified, defaulting to `agapack.raw'");

		path = "agapack.raw";
	}

	aga_log(__FILE__, "Writing output file `%s'...", path);

	if(!(*fp = fopen(path, "wb"))) {
		return aga_error_system_path(__FILE__, "fopen", path);
	}

	return AGA_RESULT_OK;
}

static enum aga_result aga_fprintf_add(
		void* fp, aga_size_t* total, aga_size_t indent, const char* fmt, ...) {

	int written;
	aga_size_t i;
	va_list ap;

	va_start(ap, fmt);

	for(i = 0; i < indent; ++i) {
		if(fputc('\t', fp) == EOF) {
			va_end(ap);
			return aga_error_system(__FILE__, "fputc");
		}
	}

	if((written = vfprintf(fp, fmt, ap)) < 0) {
		va_end(ap);
		return aga_error_system(__FILE__, "vfprintf");
	}

	*total += written + indent;

	va_end(ap);
	return AGA_RESULT_OK;
}

/*
static enum aga_result aga_build_input_file(
		void* fp, const char* path, enum aga_file_kind kind) {

	enum aga_result result;

	result = aga_fprintf_add(fp, &conf_size, "\t<item name=\"%s\">");
	if(result) return result;

	return AGA_RESULT_OK;
}
 */

static enum aga_result aga_build_python(void* out, void* in) {
	enum aga_result result;

	size_t written;

	if((result = aga_file_copy(out, in, AGA_COPY_ALL))) return result;

	written = fwrite(AGA_PY_END, 1, sizeof(AGA_PY_END) - 1, out);
	if(written < sizeof(AGA_PY_END) - 1) {
		return aga_error_system(__FILE__, "fwrite");
	}

	return AGA_RESULT_OK;
}

static enum aga_result aga_build_input_file(
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

	static aga_fixed_buf_t outpath = { 0 };

	enum aga_result result = AGA_RESULT_OK;

	void* in;
	void* out;

	const char* ext;

	/*
	 * Input kinds which are handled as "raw" need a special case when
	 * Looking for the resultant artefact files -- we just redirect to the
	 * Original because there is no need to produce any output whatsoever.
	 */
	if(kind == AGA_KIND_SGML || kind == AGA_KIND_RAW) return AGA_RESULT_OK;

	/* Skip input files which don't match kind. */
	ext = strchr(path, '.');
	if(!ext || !aga_streql(ext, kind_exts[kind])) return AGA_RESULT_OK;

	strcpy(outpath, path);
	strcat(outpath, AGA_RAWPATH);

	if(!(in = fopen(path, "rb"))) {
		return aga_error_system_path(__FILE__, "fopen", path);
	}

	if(!(out = fopen(outpath, "wb"))) {
		return aga_error_system_path(__FILE__, "fopen", path);
	}

	/* TODO: Leaky error states. */
	switch(kind) {
		default: {
			aga_log(
					__FILE__,
					"err: Unknown or unimplemented input kind for `%s'", path);
			break;
		}

		case AGA_KIND_PY: result = aga_build_python(out, in); break;

/*
		case AGA_KIND_TIFF: break;
		case AGA_KIND_OBJ: break;

		case AGA_KIND_WAV: break;
		case AGA_KIND_MIDI: break;
 */
	}

	if(result) return result;

	if(fclose(in) == EOF) return aga_error_system(__FILE__, "fclose");

	if(fclose(out) == EOF) return aga_error_system(__FILE__, "fclose");

	return AGA_RESULT_OK;
}

static enum aga_result aga_build_input_dir(const char* path, void* pass) {
	enum aga_file_kind* kind = pass;

	return aga_build_input_file(path, *kind);
}

static enum aga_result aga_build_input(
		const char* path, enum aga_file_kind kind, aga_bool_t recurse,
		void* pass) {

	enum aga_result result;
	union aga_file_attribute attr;

	(void) pass;

	if((result = aga_file_attribute_path(path, AGA_FILE_TYPE, &attr))) {
		return result;
	}

	if(attr.type == AGA_FILE_DIRECTORY) {
		return aga_directory_iterate(
				path, aga_build_input_dir, recurse, &kind, AGA_TRUE);
	}
	else return aga_build_input_file(path, kind);
}

struct aga_build_conf_pass {
	void* fp;
	enum aga_file_kind kind;
};

static enum aga_result aga_build_conf_file(
		const char* path, enum aga_file_kind kind, void* fp) {

	enum aga_result result;

	aga_size_t total = 0;

	union aga_file_attribute attr;

	result = aga_fprintf_add(fp, &total, 1, "<item name=\"%s\">\n", path); \
	if(result) return result;

	/* Unpleasant but neccesary. */
#define agab_(indent, name, type, fmt, param) \
		result = aga_fprintf_add( \
				fp, &total, indent, \
				"<item name=\"%s\" type=\"%s\">\n", name, type); \
		if(result) return result; \
		\
		result = aga_fprintf_add(fp, &total, indent + 1, fmt "\n", param); \
		if(result) return result; \
		\
		result = aga_fprintf_add(fp, &total, indent, "</item>\n"); \
		if(result) return result;
	{
		/*
		 * TODO: Sort out:
		 *		  - `%zu' did not exist until C89.
		 *		  - `%llu' should not be used on non "modern Windows" machines.
		 *				- Same goes for `long long' in general.
		 *
		 *		 Might need to make our own equivalent of `inttypes.h'.
		 */
		agab_(2, "Offset", "Integer", "%zu", total);

		if((result = aga_file_attribute_path(path, AGA_FILE_LENGTH, &attr))) {
			return result;
		}
		agab_(2, "Size", "Integer", "%zu", attr.length);

		switch(kind) {
			default: break;

			/*
			 * TODO: More formally document these "tails" in a comment at the
			 * 		 Top of this file.
			 */
			case AGA_KIND_TIFF: {
				aga_uint_t width;

				result = aga_path_tail(path, sizeof(width), &width);
				if(result) return result;

				agab_(2, "Width", "Integer", "%u", width);

				break;
			}

			case AGA_KIND_OBJ: {
				float extents[6];

				result = aga_path_tail(path, sizeof(extents), extents);
				if(result) return result;

				agab_(2, "MinX", "Float", "%f", extents[0]);
				agab_(2, "MinY", "Float", "%f", extents[1]);
				agab_(2, "MinZ", "Float", "%f", extents[2]);
				agab_(2, "MaxX", "Float", "%f", extents[3]);
				agab_(2, "MaxY", "Float", "%f", extents[4]);
				agab_(2, "MaxZ", "Float", "%f", extents[5]);

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

	result = aga_fprintf_add(fp, &total, 1, "</item>\n"); \
	if(result) return result;

	return AGA_RESULT_OK;
}

static enum aga_result aga_build_conf_dir(const char* path, void* pass) {
	struct aga_build_conf_pass* conf_pass = pass;

	return aga_build_conf_file(path, conf_pass->kind, conf_pass->fp);
}

static enum aga_result aga_build_conf(
		const char* path, enum aga_file_kind kind, aga_bool_t recurse,
		void* pass) {

	enum aga_result result;
	union aga_file_attribute attr;

	if((result = aga_file_attribute_path(path, AGA_FILE_TYPE, &attr))) {
		return result;
	}

	if(attr.type == AGA_FILE_DIRECTORY) {
		struct aga_build_conf_pass conf_pass;

		conf_pass.fp = pass;
		conf_pass.kind = kind;

		return aga_directory_iterate(
				path, aga_build_conf_dir, recurse, &conf_pass, AGA_TRUE);
	}
	else return aga_build_conf_file(path, kind, pass);
}

static enum aga_result aga_build_pack_file(
		const char* path, enum aga_file_kind kind, void* fp) {

	enum aga_result result;

	union aga_file_attribute attr;

	void* in;
	aga_size_t size;

	if((result = aga_file_attribute_path(path, AGA_FILE_LENGTH, &attr))) {
		return result;
	}

	size = attr.length;

	switch(kind) {
		default: break;

		case AGA_KIND_TIFF: size -= sizeof(aga_uint_t); break;
		case AGA_KIND_OBJ: size -= sizeof(float[6]); break;
	}

	if(!(in = fopen(path, "rb"))) {
		return aga_error_system_path(__FILE__, "fopen", path);
	}

	result = aga_file_copy(fp, in, size);
	if(result) goto cleanup;

	if(fclose(in) == EOF) return aga_error_system(__FILE__, "fclose");

	return AGA_RESULT_OK;

	cleanup: {
		if(fclose(in) == EOF) aga_error_system(__FILE__, "fclose");

		return result;
	}
}

static enum aga_result aga_build_pack_dir(const char* path, void* pass) {
	struct aga_build_conf_pass* conf_pass = pass;

	return aga_build_pack_file(path, conf_pass->kind, conf_pass->fp);
}

static enum aga_result aga_build_pack(
		const char* path, enum aga_file_kind kind, aga_bool_t recurse,
		void* pass) {

	enum aga_result result;
	union aga_file_attribute attr;

	if((result = aga_file_attribute_path(path, AGA_FILE_TYPE, &attr))) {
		return result;
	}

	if(attr.type == AGA_FILE_DIRECTORY) {
		struct aga_build_conf_pass conf_pass;

		conf_pass.fp = pass;
		conf_pass.kind = kind;

		return aga_directory_iterate(
				path, aga_build_pack_dir, recurse, &conf_pass, AGA_TRUE);
	}
	else return aga_build_pack_file(path, kind, pass);
}

static enum aga_result aga_build_iter(
		struct aga_config_node* input_root, aga_bool_t log,
		aga_input_iterfn_t fn, void* pass) {

	enum aga_result result;
	enum aga_result held_result = AGA_RESULT_OK;

	aga_size_t i, j;

	for(i = 0; i < input_root->len; ++i) {
		struct aga_config_node* node = &input_root->children[i];

		aga_slong_t v;
		const char* str = 0;

		enum aga_file_kind kind = AGA_KIND_NONE;
		const char* path = 0;
		aga_bool_t recurse = AGA_FALSE;

		for(j = 0; j < node->len; ++j) {
			struct aga_config_node* child = &node->children[j];

			if(aga_config_variable("Kind", child, AGA_STRING, &str)) {
				if(aga_streql(str, "RAW")) kind = AGA_KIND_RAW;
				else if(aga_streql(str, "TIFF")) kind = AGA_KIND_TIFF;
				else if(aga_streql(str, "OBJ")) kind = AGA_KIND_OBJ;
				else if(aga_streql(str, "SGML")) kind = AGA_KIND_SGML;
				else if(aga_streql(str, "PY")) kind = AGA_KIND_PY;
				else if(aga_streql(str, "WAV")) kind = AGA_KIND_WAV;
				else if(aga_streql(str, "MIDI")) kind = AGA_KIND_MIDI;
				else {
					aga_log(
							__FILE__,
							"warn: Unknown input kind `%s' -- Assuming `RAW'",
							str);

					kind = AGA_KIND_RAW;
				}

				continue;
			}
			else if(aga_config_variable("Path", child, AGA_STRING, &path)) {
				continue;
			}
			else if(aga_config_variable("Recurse", child, AGA_INTEGER, &v)) {
				recurse = !!v;
				continue;
			}
		}

		if(log) {
			aga_log(
					__FILE__,
					"Build Input: Path=\"%s\" Kind=%s Recurse=%s",
					path, str, recurse ? "True" : "False");
		}

		if((result = fn(path, kind, recurse, pass))) {
			aga_error_check_soft(__FILE__, "aga_build_input", result);
			held_result = result;
		}
	}

	return held_result;
}

/* TODO: Lots of leaky error states in here and the above statics. */
enum aga_result aga_build(struct aga_settings* opts) {
	static const char* input = "Input";

	enum aga_result result;
	enum aga_result held_result = AGA_RESULT_OK;

	struct aga_config_node root;
	struct aga_config_node* input_root;

	void* fp;

	aga_log(__FILE__, "Compiling project `%s'...", opts->build_file);

	if((result = aga_openconf(opts->build_file, &root))) return result;

	result = aga_config_lookup_check(root.children, &input, 1, &input_root);
	if(result) {
		aga_log(__FILE__, "err: No input files specified");
		return result;
	}

	if((result = aga_build_iter(input_root, AGA_TRUE, aga_build_input, 0))) {
		return result;
	}

	if((result = aga_openout(&root, &fp))) return result;

	if((result = aga_build_iter(input_root, AGA_FALSE, aga_build_conf, fp))) {
		return result;
	}

	if((result = aga_build_iter(input_root, AGA_FALSE, aga_build_pack, fp))) {
		return result;
	}

	if(fclose(fp) == EOF) return aga_error_system(__FILE__, "fclose");

	aga_log(__FILE__, "Done!");

	return held_result;
}

#else

enum aga_result aga_build(struct aga_settings* opts) {
	(void) opts;

	aga_log(__FILE__, "err: Project building is only supported in dev builds");

	return AGA_RESULT_ERROR;
}

#endif
