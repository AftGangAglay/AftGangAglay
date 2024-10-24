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

/* TODO: For `struct vertex' definition -- move elsewhere. */
#include <agan/object.h>

#define AGA_RAWPATH (".raw")
#define AGA_PY_END ("\n\xFF")
/* TODO: Pass this through properly to `aga_build_X'. */
#define AGA_BUILD_FNAME ("<build>")

#ifdef AGA_DEVBUILD

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4668) /* Symbol not defined as macro. */
#endif

#include <glm.h>
#include <tiffio.h>

#ifdef _MSC_VER
# pragma warning(pop)
#endif

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

static enum aga_result aga_build_open_config(
		const char* path, struct aga_config_node* root) {

	enum aga_result result;

	void* fp;
	union aga_file_attribute attr;

	if(!(fp = fopen(path, "r"))) {
		return aga_error_system_path(__FILE__, "fopen", path);
	}

	if((result = aga_file_attribute(fp, AGA_FILE_LENGTH, &attr))) return result;

	aga_config_debug_file = path;

	result = aga_config_new(fp, attr.length, root);
	if(result) {
		if(fclose(fp) == EOF) (void) aga_error_system(__FILE__, "fclose");

		return result;
	}

	if(fclose(fp) == EOF) return aga_error_system(__FILE__, "fclose");

	return AGA_RESULT_OK;
}

static enum aga_result aga_build_open_output(
		struct aga_config_node* root, void** fp, const char** out_path) {

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

	*out_path = path;

	aga_log(__FILE__, "Writing output file `%s'...", path);

	if(!(*fp = fopen(path, "wb"))) {
		return aga_error_system_path(__FILE__, "fopen", path);
	}

	return AGA_RESULT_OK;
}

static enum aga_result aga_fprintf_add(
		void* fp, aga_size_t indent, const char* fmt, ...) {

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

static enum aga_result aga_build_obj(void* out, void* in) {
	enum aga_result result = AGA_RESULT_OK;

	GLMmodel* model;
	float extent[6];

	unsigned i, j;
	GLMgroup* group;

	if(!(model = glmReadOBJFile(AGA_BUILD_FNAME, in))) {
		/* TODO: Handle different EH. */
		return AGA_RESULT_OOM;
	}

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

			struct aga_vertex v;

			if(mat) aga_memcpy(v.col, mat->diffuse, sizeof(v.col));
			else aga_memset(v.col, 0, sizeof(v.col));

			for(j = 0; j < 3; ++j) {
				aga_memcpy(v.norm, &norms[3 * t->n_inds[j]], sizeof(v.norm));
				aga_memcpy(v.uv, &uvs[2 * t->t_inds[j]], sizeof(v.uv));
				aga_memcpy(v.pos, &verts[3 * t->v_inds[j]], sizeof(v.pos));

				if(fwrite(&v, sizeof(v), 1, out) < 1) {
					if(ferror(out)) {
						return aga_error_system(__FILE__, "fwrite");
					}

					return AGA_RESULT_EOF;
				}
			}
		}

		group = group->next;
	}

	if(fwrite(extent, sizeof(float), AGA_LEN(extent), out) < AGA_LEN(extent)) {
		if(ferror(out)) {
			result = aga_error_system(__FILE__, "fwrite");
			goto cleanup;
		}

		result = AGA_RESULT_EOF;
		goto cleanup;
	}

	cleanup: {
		glmDelete(model);
	}

	return result;

}

static enum aga_result aga_build_tiff(void* out, void* in) {
	/* NOTE: TIFF wants this -- this is kind of evil. */
	static char msg[1024];

	enum aga_result result = AGA_RESULT_OK;

	int fd;

	TIFF* tiff;
	TIFFRGBAImage img = { 0 };

	aga_size_t size, count;
	void* raster = 0;

	aga_uint_t width;

	if((fd = fileno(in)) == -1) return aga_error_system(__FILE__, "fileno");

	if(!(tiff = TIFFFdOpen(fd, AGA_BUILD_FNAME, "r"))) return AGA_RESULT_ERROR;

	if(!TIFFRGBAImageBegin(&img, tiff, 0, msg)) {
		aga_log(__FILE__, "err: Failed to read TIFF file: %s", msg);
		result = AGA_RESULT_ERROR;
		goto cleanup;
	}

	count = img.width * img.height;
	size = 4 * count;
	if(!(raster = aga_malloc(size))) {
		result = aga_error_system(__FILE__, "aga_malloc");
		goto cleanup;
	}

	if(!TIFFRGBAImageGet(&img, raster, img.width, img.height)) {
		result = AGA_RESULT_ERROR;
		goto cleanup;
	}

	if(fwrite(raster, 4, count, out) < count) {
		if(ferror(out)) {
			result = aga_error_system(__FILE__, "fwrite");
			goto cleanup;
		}

		result = AGA_RESULT_EOF;
		goto cleanup;
	}

	width = img.width;
	if(fwrite(&width, sizeof(width), 1, out) < 1) {
		if(ferror(out)) {
			result = aga_error_system(__FILE__, "fwrite");
			goto cleanup;
		}

		result = AGA_RESULT_EOF;
		goto cleanup;
	}

	cleanup: {
		aga_free(raster);
		TIFFRGBAImageEnd(&img);
		TIFFClose(tiff, 0);
	}

	return result;
}

static aga_bool_t aga_build_path_matches_kind(
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
	const char* ext = strrchr(path, '.');
	if(!ext || !aga_streql(ext, kind_exts[kind])) return AGA_FALSE;

	return AGA_TRUE;
}

static enum aga_result aga_build_input_file(
		const char* path, enum aga_file_kind kind) {

	static aga_fixed_buf_t outpath = { 0 };

	enum aga_result result = AGA_RESULT_OK;

	void* in;
	void* out;

	/*
	 * Input kinds which are handled as "raw" need a special case when
	 * Looking for the resultant artefact files -- we just redirect to the
	 * Original because there is no need to produce any output whatsoever.
	 */
	if(kind == AGA_KIND_SGML || kind == AGA_KIND_RAW) return AGA_RESULT_OK;

	/* Skip input files which don't match kind. */
	if(!aga_build_path_matches_kind(path, kind)) return AGA_RESULT_OK;

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

		case AGA_KIND_OBJ: result = aga_build_obj(out, in); break;
		case AGA_KIND_TIFF: result = aga_build_tiff(out, in); break;

/*
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
	aga_size_t offset;
};

static enum aga_result aga_build_conf_file(
		const char* path, enum aga_file_kind kind, void* fp,
		aga_size_t* offset) {

	aga_fixed_buf_t outpath = { 0 };

	enum aga_result result;

	union aga_file_attribute attr;

	/* Skip input files which don't match kind. */
	if(!aga_build_path_matches_kind(path, kind)) return AGA_RESULT_OK;

	strcpy(outpath, path);

	/* Use the base file as the input for SGML/RAW inputs. */
	if(kind != AGA_KIND_SGML && kind != AGA_KIND_RAW) {
		strcat(outpath, AGA_RAWPATH);
	}

	result = aga_fprintf_add(fp, 1, "<item name=\"%s\">\n", outpath); \
    if (result) return result;

	/* Unpleasant but neccesary. */
#define agab_(indent, name, type, fmt, param) \
        do { \
            result = aga_fprintf_add( \
                    fp, indent, \
                    "<item name=\"%s\" type=\"%s\">\n", name, type); \
            if(result) return result; \
            \
            result = aga_fprintf_add(fp, indent + 1, fmt "\n", param); \
            if(result) return result; \
            \
            result = aga_fprintf_add(fp, indent, "</item>\n"); \
            if(result) return result; \
		} while(0)
	{
		/*
		 * TODO: Sort out:
		 *		  - `%zu' did not exist until C99.
		 *		  - `%llu' should not be used on non "modern Windows" machines.
		 *				- Same goes for `long long' in general.
		 *
		 *		 Might need to make our own equivalent of `inttypes.h'.
		 */
		agab_(2, "Offset", "Integer", "%zu", *offset);

		result = aga_file_attribute_path(outpath, AGA_FILE_LENGTH, &attr);
		if(result) return result;

		*offset += attr.length;
		agab_(2, "Size", "Integer", "%zu", attr.length);

		switch(kind) {
			default: break;

			/*
			 * TODO: More formally document these "tails" in a comment at the
			 * 		 Top of this file.
			 */
			case AGA_KIND_TIFF: {
				aga_uint_t width;

				result = aga_path_tail(outpath, sizeof(width), &width);
				if(result) return result;

				*offset -= sizeof(width);
				agab_(2, "Width", "Integer", "%u", width);

				break;
			}

			case AGA_KIND_OBJ: {
				float extents[6];

				result = aga_path_tail(outpath, sizeof(extents), extents);
				if(result) return result;

				*offset -= sizeof(extents);
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

	result = aga_fprintf_add(fp, 1, "</item>\n"); \
	if(result) return result;

	return AGA_RESULT_OK;
}

static enum aga_result aga_build_conf_dir(const char* path, void* pass) {
	struct aga_build_conf_pass* conf_pass = pass;

	return aga_build_conf_file(
			path, conf_pass->kind, conf_pass->fp, &conf_pass->offset);
}

static enum aga_result aga_build_conf(
		const char* path, enum aga_file_kind kind, aga_bool_t recurse,
		void* pass) {

	enum aga_result result;
	union aga_file_attribute attr;

	struct aga_build_conf_pass* conf_pass = pass;

	if((result = aga_file_attribute_path(path, AGA_FILE_TYPE, &attr))) {
		return result;
	}

	if(attr.type == AGA_FILE_DIRECTORY) {
		conf_pass->kind = kind;

		return aga_directory_iterate(
				path, aga_build_conf_dir, recurse, conf_pass, AGA_TRUE);
	}
	else {
		return aga_build_conf_file(
			path, kind, conf_pass->fp, &conf_pass->offset);
	}
}

static enum aga_result aga_build_pack_file(
		const char* path, enum aga_file_kind kind, void* fp) {

	aga_fixed_buf_t outpath = { 0 };

	enum aga_result result;

	union aga_file_attribute attr;

	void* in;
	aga_size_t size;

	/* Skip input files which don't match kind. */
	if(!aga_build_path_matches_kind(path, kind)) return AGA_RESULT_OK;

	strcpy(outpath, path);

	/* Use the base file as the input for SGML/RAW inputs. */
	if(kind != AGA_KIND_SGML && kind != AGA_KIND_RAW) {
		strcat(outpath, AGA_RAWPATH);
	}

	if((result = aga_file_attribute_path(outpath, AGA_FILE_LENGTH, &attr))) {
		return result;
	}

	size = attr.length;

	switch(kind) {
		default: break;

		/* TODO: Structurize file tails. */
		case AGA_KIND_TIFF: size -= sizeof(aga_uint_t); break;
		case AGA_KIND_OBJ: size -= sizeof(float[6]); break;
	}

	if(!(in = fopen(outpath, "rb"))) {
		return aga_error_system_path(__FILE__, "fopen", outpath);
	}

	result = aga_file_copy(fp, in, size);
	if(result) goto cleanup;

	if(fclose(in) == EOF) return aga_error_system(__FILE__, "fclose");

	return AGA_RESULT_OK;

	cleanup: {
		if(fclose(in) == EOF) (void) aga_error_system(__FILE__, "fclose");

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
			aga_error_check_soft(
					__FILE__, "aga_build_iter::<callback>", result);

			held_result = result;
		}
	}

	return held_result;
}

static void aga_tiff_handler(
		aga_bool_t warning, const char* module, const char* fmt, va_list ap) {

	static aga_fixed_buf_t buf = { 0 };
	int ret;
	char* p = buf;

	if((ret = sprintf(p, "%s: ", warning ? "warn" : "err")) < 0) {
		(void) aga_error_system(__FILE__, "sprintf");
		return;
	}
	p += ret;

	if(module) {
		if((ret = sprintf(p, "%s: ", module)) < 0) {
			(void) aga_error_system(__FILE__, "sprintf");
			return;
		}
		p += ret;
	}
	if(vsprintf(p, fmt, ap) < 0) {
		(void) aga_error_system(__FILE__, "vsprintf");
		return;
	}

	aga_log(__FILE__, buf);
}

static void aga_tiff_error(const char* module, const char* fmt, va_list ap) {
	aga_tiff_handler(AGA_FALSE, module, fmt, ap);
}

static void aga_tiff_warning(const char* module, const char* fmt, va_list ap) {
	aga_tiff_handler(AGA_TRUE, module, fmt, ap);
}

/* TODO: Lots of leaky error states in here and the above statics. */
/* TODO: Extra verbose per-file output if set to verbose output. */
enum aga_result aga_build(struct aga_settings* opts) {
	static const char* input = "Input";

	enum aga_result result;

	struct aga_config_node root;
	struct aga_config_node* input_root;

	void* fp = 0;
	const char* out_path = 0;

	aga_log(__FILE__, "Compiling project `%s'...", opts->build_file);

	TIFFSetErrorHandler(aga_tiff_error);
	TIFFSetWarningHandler(aga_tiff_warning);

	if((result = aga_build_open_config(opts->build_file, &root))) {
		return result;
	}

	result = aga_config_lookup_check(root.children, &input, 1, &input_root);
	if(result) {
		aga_log(__FILE__, "err: No input files specified");
		goto cleanup;
	}

	if((result = aga_build_iter(input_root, AGA_TRUE, aga_build_input, 0))) {
		goto cleanup;
	}

	if((result = aga_build_open_output(&root, &fp, &out_path))) goto cleanup;

	aga_log(__FILE__, "Building pack directory...");

	{
		struct aga_build_conf_pass conf_pass;

		struct aga_resource_pack_header hdr = { 0, AGA_PACK_MAGIC };
		long off;
		fpos_t mark;

		conf_pass.fp = fp;
		conf_pass.offset = 0;

		if(fwrite(&hdr, sizeof(hdr), 1, fp) < 1) {
			if(ferror(fp)) result = aga_error_system(__FILE__, "fwrite");
			else result = AGA_RESULT_EOF;

			goto cleanup;
		}
		if(fputs("<root>\n", fp) == EOF) {
			result = aga_error_system(__FILE__, "fputs");
			goto cleanup;
		}

		result = aga_build_iter(
				input_root, AGA_FALSE, aga_build_conf, &conf_pass);

		if(result) goto cleanup;

		if(fputs("</root>\n", fp) == EOF) {
			result = aga_error_system(__FILE__, "fputs");
			goto cleanup;
		}

		if((off = ftell(fp)) == -1) {
			result = aga_error_system(__FILE__, "ftell");
			goto cleanup;
		}

		hdr.size = off - sizeof(hdr);

		if(fgetpos(fp, &mark)) {
			result = aga_error_system(__FILE__, "fgetpos");
			goto cleanup;
		}

		rewind(fp);

		if(fwrite(&hdr, sizeof(hdr), 1, fp) < 1) {
			if(ferror(fp)) result = aga_error_system(__FILE__, "fwrite");
			else result = AGA_RESULT_EOF;

			goto cleanup;
		}

		if(fsetpos(fp, &mark)) {
			result = aga_error_system(__FILE__, "fsetpos");
			goto cleanup;
		}
	}

	aga_log(__FILE__, "Inserting file data...");

	if((result = aga_build_iter(input_root, AGA_FALSE, aga_build_pack, fp))) {
		goto cleanup;
	}

	if(fclose(fp) == EOF) goto cleanup;

	if((result = aga_config_delete(&root))) return result;

	aga_log(__FILE__, "Done!");

	return AGA_RESULT_OK;

	cleanup: {
		/* TODO: Destroy failed intermediate files in error cases aswell. */
		/*
		 * NOTE: `out_path' may reside in `root' so this needs to be before
		 * 		 `aga_config_delete'.
		 */
		if(out_path) {
			aga_error_check_soft(
					__FILE__, "aga_path_delete",
					aga_path_delete(out_path));
		}

		aga_error_check_soft(
				__FILE__, "aga_config_delete", aga_config_delete(&root));

		if(fp && fclose(fp) == EOF) {
			(void) aga_error_system(__FILE__, "fclose");
		}

		aga_log(__FILE__, "err: Build failed");

		return result;
	};
}

#else

enum aga_result aga_build(struct aga_settings* opts) {
	(void) opts;

	aga_log(__FILE__, "err: Project building is only supported in dev builds");

	return AGA_RESULT_ERROR;
}

#endif
