/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <apro.h>

#include <agan/object.h>
#include <agan/draw.h>

#include <aga/startup.h>
#include <aga/draw.h>
#include <aga/script.h>
#include <aga/pack.h>
#include <aga/diagnostic.h>

#include <asys/log.h>
#include <asys/memory.h>
#include <asys/string.h>
#include <asys/math.h>

#include <mil/system.h>

/* TODO: Some `aga_script_*err` disable with noverify. */

/* TODO: Report object-related errors with path. */

/*
 * TODO: Object init uses a lot of nonlinear conf lookup. If we just traversed
 * 		 The tree "as-is" it'd require the conf to be well-ordered (which we
 * 		 Don't want) - so we need a hybrid approach.
 */

/*
 * TODO: "Portal" object property using stencil buffers and camera state.
 */

#define AGA_SWAP_FLOAT(a, b) \
	do { \
		float scratch = b; \
		b = a; \
		a = scratch; \
	} while(0)

#define AGA_TRANSFORM_TOLERANCE (0.001)

enum asys_result agan_obj_register(struct py_env* env) {
	(void) env;
	return ASYS_RESULT_OK;
}

static asys_bool_t agan_mkobj_trans(
		struct agan_object* obj, struct aga_config_node* conf) {

	enum asys_result result;
	const char* path[2];

	struct py_object* l;
	struct py_object* o;
	unsigned i, j;
	double f;

	for(i = 0; i < 3; ++i) {
		path[0] = agan_conf_components[i];

		l = py_dict_lookup(obj->transform, agan_trans_components[i]);
		if(!l) {
			py_error_set_key();
			return ASYS_TRUE;
		}

		for(j = 0; j < 3; ++j) {
			path[1] = agan_xyz[j];

			result = aga_config_lookup(
					conf->children, path, ASYS_LENGTH(path), &f, AGA_FLOAT,
					ASYS_FALSE);

			if(result) f = 0.0f;

			if(!(o = py_float_new(f))) {
				py_error_set_nomem();
				return ASYS_TRUE;
			}

			py_list_set(l, j, o);
		}
	}

	return ASYS_FALSE;
}

static void agan_mkobj_extent(
		struct agan_object* obj, struct aga_config_node* conf) {

	static const char* min_attr[] = { "MinX", "MinY", "MinZ" };
	static const char* max_attr[] = { "MaxX", "MaxY", "MaxZ" };

	float (*min)[3] = &obj->min_extent;
	float (*max)[3] = &obj->max_extent;

	asys_size_t i;

	for(i = 0; i < 3; ++i) {
		double v;

		if(aga_config_lookup(conf, &min_attr[i], 1, &v, AGA_FLOAT, ASYS_FALSE)) {
			(*min)[i] = 0.0f;
		}
		else (*min)[i] = (float) v;

		if(aga_config_lookup(conf, &max_attr[i], 1, &v, AGA_FLOAT, ASYS_FALSE)) {
			(*max)[i] = 0.0f;
		}
		else (*max)[i] = (float) v;
	}
}

/*
 * TODO: Object models should be able to specify a billboard texture for auto
 * 		 LOD -- especially when we have our zoning/distance culling system.
 */
asys_bool_t agan_mkobj_model(
		struct py_env* env, struct agan_object* obj,
		struct aga_config_node* conf, struct aga_resource_pack* pack,
		const char* objpath) {

	static const char* model = "Model";
	static const char* texture = "Texture";
	static const char* filter = "Filter";
	static const char* mipmap = "Mipmap";

	struct aga_settings* settings = AGA_GET_USERDATA(env)->opts;

	enum asys_result result;

	struct aga_resource* res;
	unsigned mode = GL_COMPILE;
	char* model_path;
	char* texture_path;

	/* TODO: Delete lists in error conditions. */
	obj->drawlist = glGenLists(1);
	if(aga_script_gl_err(__FILE__, "glGenLists")) return ASYS_TRUE;

#ifndef NDEBUG
	mode = GL_COMPILE_AND_EXECUTE;
#endif

	/*
	 * TODO: We could batch chunks of static scene geometry together into one
	 * 		 Large list during scene build once we have a more cohesive system
	 * 		 In-place.
	 */
	/*
	 * TODO: Advice appears to be to keep a list per-model rather than
	 * 		 Per-object and to retexture/material as necessary for instances.
	 * 		 We can probably balance this between instanced and non-instanced
	 * 		 Draw.
	 */
	glNewList(obj->drawlist, mode);
	if(aga_script_gl_err(__FILE__, "glNewList")) return ASYS_TRUE;

	{
		asys_bool_t do_mips, tex_filter;
		aga_config_int_t v;

		result = aga_config_lookup(
				conf->children, &filter, 1, &v, AGA_INTEGER, ASYS_FALSE);

		if(result) v = 1;
		tex_filter = !!v;

		result = aga_config_lookup(
				conf->children, &mipmap, 1, &v, AGA_INTEGER, ASYS_FALSE);

		if(result) v = settings->mipmap_default;
		do_mips = !!v;

		result = aga_config_lookup(
				conf->children, &texture, 1, &texture_path, AGA_PATH,
				ASYS_FALSE);

		if(result) {
			/* TODO: Does this handle this case gracefully. */
			asys_log(
					__FILE__, "warn: Object `%s' is missing a texture entry",
					objpath);
		}
		else {
			static const char* width = "Width";

			aga_config_int_t w, h;

			/*
			 * TODO: "Just trusting" that this make/release pattern is safe to
			 * 		 Avoid leaks is unclear and unfriendly to non-maintainers.
			 * 		 We probably need better lifetime/cleanup etiquette and
			 * 		 Helper code.
			 */

			/*
			 * TODO: Handle missing textures etc. gracefully - default/
			 * 		 Procedural resources?
			 */
			/*
			 * TODO: Way to deliberately sweep resources once a certain
			 * 		 Resource type memory cap is reached? Super big RAM spike
			 * 		 When a scene loads all of these. We could make decref on
			 * 		 A resource trigger a sweep if the global resource ceiling
			 * 		 Is close to being reached and warn if we are still close
			 * 		 To it.
			 */
			result = aga_resource_new(pack, texture_path, &res);
			if(aga_script_err(__FILE__, "aga_resource_new", result)) {
				return ASYS_TRUE;
			}

			result = aga_resource_release(res);
			if(aga_script_err(__FILE__, "aga_resource_release", result)) {
				return ASYS_TRUE;
			}

			result = aga_config_lookup(
					res->config, &width, 1, &w, AGA_INTEGER, ASYS_FALSE);

			if(result) {
				/* TODO: Default conf values as part of the API. */
				asys_log(__FILE__, "warn: Texture `%s' is missing dimensions");
				w = 0;
				h = 0;
			}
			else h = (int) (res->size / (asys_size_t) (4 * w));

			/*
			 * TODO: Non-alpha textures for more effective use of GPU memory
			 * 		 And turning off transparency auto-disables alpha channel.
			 * 		 `glPolygonStipple' can be used for fake transparency.
			 */
			if(do_mips) {
				gluBuild2DMipmaps(
						GL_TEXTURE_2D, 4, (int) w, (int) h, GL_RGBA,
						GL_UNSIGNED_BYTE, res->data);

				/*
				 * TODO: Script land can probably handle lots of GL errors like
				 * 		 This relatively gracefully (i.e. allow the user code
				 * 		 To go further without needing try-catch hell).
				 * 		 Especially in functions like this which aren't
				 * 		 Supposed to be run every frame.
				 */
				if(aga_script_gl_err(__FILE__, "gluBuild2DMipmaps")) {
					return ASYS_TRUE;
				}
			}
			else {
				glTexImage2D(
						GL_TEXTURE_2D, 0, 4, (int) w, (int) h, 0, GL_RGBA,
						GL_UNSIGNED_BYTE, res->data);

				if(aga_script_gl_err(__FILE__, "glTexImage2D")) {
					return ASYS_TRUE;
				}
			}

			{
				/* TODO: `ScaleTex' for stretch vs. tile. */
				int mag = tex_filter ? GL_LINEAR : GL_NEAREST;
				int min;

				if(do_mips) {
					min = tex_filter ?
							GL_LINEAR_MIPMAP_LINEAR :
							GL_NEAREST_MIPMAP_NEAREST;
				}
				else min = mag;

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
				if(aga_script_gl_err(__FILE__, "glTexParameteri")) {
					return ASYS_TRUE;
				}

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
				if(aga_script_gl_err(__FILE__, "glTexParameteri")) {
					return ASYS_TRUE;
				}
			}

			asys_memory_free(texture_path);
		}

		result = aga_config_lookup(
				conf->children, &model, 1, &model_path, AGA_PATH, ASYS_FALSE);

		if(result) {
			asys_log(
					__FILE__, "warn: Object `%s' is missing a model entry",
					objpath);
		}
		else {
			static const char* version = "Version";

			struct aga_vertex vert;
			struct asys_stream* stream;
			asys_size_t i, len;
			aga_config_int_t ver;

#ifdef AGA_DEVBUILD
			asys_memory_free(obj->modelpath);
			if(!(obj->modelpath = asys_string_duplicate(model_path))) {
				py_error_set_nomem();
				return ASYS_TRUE;
			}

			asys_memory_free(obj->texpath);
			if(!(obj->texpath = asys_string_duplicate(texture_path))) {
				py_error_set_nomem();
				return ASYS_TRUE;
			}
#endif

			result = aga_resource_pack_lookup(pack, model_path, &res);
			if(aga_script_err(__FILE__, "aga_resource_pack_lookup", result)) {
				asys_log(
						__FILE__, "err: Failed to find resource `%s'",
						model_path);

				return ASYS_TRUE;
			}

			agan_mkobj_extent(obj, res->config);

			asys_memory_free(model_path);

			result = aga_config_lookup(
					res->config, &version, 1, &ver, AGA_INTEGER, ASYS_FALSE);
			if(result) ver = 1;

			result = aga_resource_seek(res, &stream);
			/* TODO: We can't return during list build! */
			if(aga_script_err(__FILE__, "aga_resource_stream", result)) {
				return ASYS_TRUE;
			}
			len = res->size;

			glBegin(GL_TRIANGLES);
			/* if(aga_script_gl_err("glBegin")) return 0; */

			for(i = 0; i < len; i += sizeof(vert)) {
				result = asys_stream_read(
						stream, 0, &vert, sizeof(struct aga_vertex));

				if(aga_script_err(__FILE__, "asys_stream_read", result)) {
					return ASYS_TRUE;
				}

				/*
				 * Models from v2.1.0 and below respected model vertex
				 * Colouration.
				 */
				if(ver == 2) {
					asys_uchar_t r = (obj->ind >> (2 * 8)) & 0xFF;
					asys_uchar_t g = (obj->ind >> (1 * 8)) & 0xFF;
					asys_uchar_t b = (obj->ind >> (0 * 8)) & 0xFF;
					glColor3ub(r, g, b);
				}
				else {
					AGA_DEPRECATED_IMPL(
							"Loading Version 1 model data is deprecated");

					glColor4fv(vert.col);
				}

				glTexCoord2fv(vert.uv);
				/* if(aga_script_gl_err("glTexCoord2fv")) return ASYS_TRUE; */
				glNormal3fv(vert.norm);
				/* if(aga_script_gl_err("glNormal3fv")) return ASYS_TRUE; */
				glVertex3fv(vert.pos);
				/* if(aga_script_gl_err("glVertex3fv")) return ASYS_TRUE; */
			}

			glEnd();
			if(aga_script_gl_err(__FILE__, "glEnd")) return 0;
		}
	}

	glEndList();
	if(aga_script_gl_err(__FILE__, "glEndList")) return 0;

	return ASYS_FALSE;
}

static asys_bool_t agan_mkobj_light(
		struct agan_object* obj, struct aga_config_node* conf) {

	static const char* light = "Light";

	enum asys_result result;

	struct aga_config_node* node = conf->children;
	struct agan_lightdata* data;

	aga_config_int_t scr;
	asys_size_t i, j;

	double v;

	if(aga_config_lookup_raw(node, &light, 1, &node)) return ASYS_FALSE;

	obj->light_data = asys_memory_allocate_zero(
			1, sizeof(struct agan_lightdata));

	if(!obj->light_data) return ASYS_TRUE;

	data = obj->light_data;

	for(i = 0; i < node->len; ++i) {
		aga_config_int_t index;
		struct aga_config_node* child = &node->children[i];

		if(aga_config_variable("Index", child, AGA_INTEGER, &index)) {
			/*
			 * TODO: Warn on light index reuse within a scene at draw time when
			 * 		 In dev builds.
			 */
			if(index > 7 || index < 0) {
				asys_log(
						__FILE__, "warn: Light index `%u' out of range 0-7",
						data->index);

				asys_memory_free(data);
				obj->light_data = 0;
				return ASYS_TRUE;
			}

			data->index = (asys_uchar_t) index;
		}
		else if(aga_config_variable("Directional", child, AGA_INTEGER, &scr)) {
			data->directional = !!scr;
		}
		else if(aga_config_variable("Exponent", child, AGA_FLOAT, &v)) {
			data->exponent = (float) v;
		}
		else if(aga_config_variable("Angle", child, AGA_FLOAT, &v)) {
			data->angle = (float) v;
		}
		else if(asys_string_equal("Direction", child->name)) {
			for(j = 0; j < 3; ++j) {
				const char* comp = agan_xyz[j];
				float (*dir)[3] = &data->direction;

				result = aga_config_lookup(
						child, &comp, 1, &v, AGA_FLOAT, ASYS_FALSE);
				(*dir)[j] = result ? 0.0f : (float) v;
			}
		}

		/* TODO: General API for getting multiple components from conf node. */
		else if(asys_string_equal("Ambient", child->name)) {
			float (*col)[4] = &data->ambient;

			for(j = 0; j < 3; ++j) {
				const char* comp = agan_rgb[j];

				result = aga_config_lookup(
						child, &comp, 1, &v, AGA_FLOAT, ASYS_FALSE);

				if(result) (*col)[j] = 1.0f;
				else (*col)[j] = (float) v;
			}

			(*col)[3] = 1.0f;
		}
		else if(asys_string_equal("Diffuse", child->name)) {
			float (*col)[4] = &data->diffuse;

			for(j = 0; j < 3; ++j) {
				const char* comp = agan_rgb[j];

				result = aga_config_lookup(
						child, &comp, 1, &v, AGA_FLOAT, ASYS_FALSE);

				(*col)[j] = result ? 1.0f : (float) v;
			}

			(*col)[3] = 1.0f;
		}
		else if(asys_string_equal("Specular", child->name)) {
			float (*col)[4] = &data->specular;

			for(j = 0; j < 3; ++j) {
				const char* comp = agan_rgb[j];

				result = aga_config_lookup(
						child, &comp, 1, &v, AGA_FLOAT, ASYS_FALSE);

				(*col)[j] = result ? 1.0f : (float) v;
			}

			(*col)[3] = 1.0f;
		}
		else if(asys_string_equal("Attenuation", child->name)) {
			static const char* attenuation[] = {
					"Constant", "Linear", "Quadratic"
			};

			float* at[ASYS_LENGTH(attenuation)];
			at[0] = &data->constant_attenuation;
			at[1] = &data->linear_attenuation;
			at[2] = &data->quadratic_attenuation;
			for(j = 0; j < ASYS_LENGTH(at); ++j) {
				const char* comp = attenuation[j];

				result = aga_config_lookup(
						child, &comp, 1, &v, AGA_FLOAT, ASYS_FALSE);
				*at[j] = result ? 0.0f : (float) v;
			}
		}
	}

	/*asys_log(
			__FILE__,
			"\nambient: [ %f, %f, %f, %f ]\n"
			"diffuse: [ %f, %f, %f, %f ]\n"
			"specular: [ %f, %f, %f, %f ]\n"
			"attenuation: {\n"
			"\tconstant: %f\n"
			"\tlinear: %f\n"
			"\tquadratic: %f\n"
			"}\n"
			"direction: [ %f, %f, %f ]\n"
			"exponent: %f\n"
			"angle: %f\n"
			"directional: %s\n"
			"index: %u",
			data->ambient[0], data->ambient[1], data->ambient[2],
				data->ambient[3],
			data->diffuse[0], data->diffuse[1], data->diffuse[2],
				data->diffuse[3],
			data->specular[0], data->specular[1], data->specular[2],
				data->specular[3],
			data->constant_attenuation, data->linear_attenuation,
				data->quadratic_attenuation,
			data->direction[0], data->direction[1], data->direction[2],
			data->exponent, data->angle, data->directional ? "true" : "false",
			(unsigned) data->index);
	 */

	return ASYS_FALSE;
}

struct py_object* agan_mkobj(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum asys_result result;

	struct agan_object* obj;
	struct py_int* v;
	struct py_object* retval;

	struct asys_stream* stream;

	const char* path;
	struct aga_resource_pack* pack = AGA_GET_USERDATA(env)->resource_pack;

	/*
	 * TODO: This is horrible (but only exists until we have an object registry
	 * 		 To get small unique handle IDs for colour picking.
	 */
	static asys_uint_t objn = 0;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_MKOBJ);

	if(!aga_arg_list(args, PY_TYPE_STRING)) {
		return aga_arg_error("mkobj", "string");
	}

	if(!(obj = asys_memory_allocate_zero(1, sizeof(struct agan_object)))) {
		return py_error_set_nomem();
	}

	retval = py_int_new((py_value_t) obj);
	v = (struct py_int*) retval;

	obj->ind = objn++;
	obj->light_data = 0;
	if(!(obj->transform = agan_mktrans(env, 0, 0))) goto cleanup;

	path = py_string_get(args);

	result = aga_resource_pack_lookup(pack, path, &obj->res);
	if(aga_script_err(__FILE__, "aga_resource_pack_lookup", result)) {
		goto cleanup;
	}

	result = aga_resource_seek(obj->res, &stream);
	if(aga_script_err(__FILE__, "aga_resource_seek", result)) goto cleanup;

	result = aga_config_new(stream, obj->res->size, &obj->config);
	if(aga_script_err(__FILE__, "aga_resource_stream", result)) goto cleanup;

	if(agan_mkobj_trans(obj, &obj->config)) goto cleanup;
	if(agan_mkobj_model(env, obj, &obj->config, pack, path)) goto cleanup;
	if(agan_mkobj_light(obj, &obj->config)) goto cleanup;

	apro_stamp_end(APRO_SCRIPTGLUE_MKOBJ);

	return retval;

	cleanup: {
		asys_log_result(
					__FILE__, "aga_config_delete",
					aga_config_delete(&obj->config));

		glDeleteLists(obj->drawlist, 1);
		(void) mil_gl_result(__FILE__, "glDeleteLists");

		asys_memory_free(obj->light_data);
		py_object_decref(obj->transform);
		asys_memory_free((void*) v->value);
		py_object_decref(retval);

		return 0;
	}
}

struct py_object* agan_killobj(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct agan_object* obj;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_KILLOBJ);

	if(!aga_arg_list(args, PY_TYPE_INT)) {
		return aga_arg_error("killobj", "int");
	}

	obj = (void*) py_int_get(args);

	glDeleteLists(obj->drawlist, 1);
	if(aga_script_gl_err(__FILE__, "glDeleteLists")) return 0;

	py_object_decref(obj->transform);

#ifdef AGA_DEVBUILD
	asys_memory_free(obj->modelpath);
	asys_memory_free(obj->texpath);
#endif

	asys_memory_free(obj);

	apro_stamp_end(APRO_SCRIPTGLUE_KILLOBJ);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_inobj(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* retval = PY_FALSE;

	struct py_object* objp;
	struct py_object* planarp;
	struct py_object* dbgp;
	struct py_object* pointp;
	struct py_object* tolerancep;

	struct py_object* pos;
	struct py_object* rot;
	struct py_object* scale;

	double point[3];
	float min[3];
	float max[3];
	double rotation[3];

	asys_bool_t planar;
	unsigned i;
	double abs_rot;
	struct agan_object* obj;
	double tolerance = AGA_TRANSFORM_TOLERANCE;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_INOBJ);

	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 4) ||
		!aga_arg(&objp, args, 0, PY_TYPE_INT) ||
		!aga_vararg_typed(&pointp, args, 1, PY_TYPE_LIST, 3, PY_TYPE_FLOAT) ||
		!aga_arg(&planarp, args, 2, PY_TYPE_INT) ||
		!aga_arg(&dbgp, args, 3, PY_TYPE_INT)) {

		if(!aga_vararg_list(args, PY_TYPE_TUPLE, 5) ||
			!aga_arg(&objp, args, 0, PY_TYPE_INT) ||
			!aga_vararg_typed(
					&pointp, args, 1, PY_TYPE_LIST, 3, PY_TYPE_FLOAT) ||
			!aga_arg(&planarp, args, 2, PY_TYPE_INT) ||
			!aga_arg(&dbgp, args, 3, PY_TYPE_INT) ||
			!aga_arg(&tolerancep, args, 4, PY_TYPE_FLOAT)) {

			return aga_arg_error(
					"inobj", "int, float[3], int, int [and float]");
		}

		tolerance = py_float_get(tolerancep);
	}

	planar = !!py_int_get(planarp);

	obj = (void*) py_int_get(objp);

	asys_memory_copy(min, obj->min_extent, sizeof(min));
	asys_memory_copy(max, obj->max_extent, sizeof(max));

	if(!(pos = py_dict_lookup(obj->transform, "pos"))) return 0;
	if(!(rot = py_dict_lookup(obj->transform, "rot"))) return 0;
	if(!(scale = py_dict_lookup(obj->transform, "scale"))) return 0;

	for(i = 0; i < ASYS_LENGTH(min); ++i) {
		float f;

		/* TODO: Static "get N items into buffer" to make noverify easier. */

		point[i] = py_float_get(py_list_get(pointp, i));

		f = (float) py_float_get(py_list_get(scale, i));

		min[i] *= f;
		max[i] *= f;

		rotation[i] = py_float_get(py_list_get(rot, i));
	}

	/* TODO: This only handles Y-plane rotations. */
	/* TODO: This doesn't handle lopsided objects correctly. */

	/* rot[Y] ~= 90 */
	/* rot[Y] ~= -90 */
	abs_rot = asys_math_fabs(asys_math_fabs(rotation[1]) - 90.0);
	if(abs_rot < AGA_TRANSFORM_TOLERANCE) {
		AGA_SWAP_FLOAT(min[0], min[2]);
		AGA_SWAP_FLOAT(max[0], max[2]);
	}

	for(i = 0; i < ASYS_LENGTH(min); ++i) {
		double f = py_float_get(py_list_get(pos, i));

		min[i] += (float) (f - tolerance);
		max[i] += (float) (f + tolerance);
	}

	/* TODO: Once cells are implemented check against current cell rad. */

	if(planar || point[1] > min[1]) {
		if(point[0] > min[0] && point[2] > min[2]) {

			if(planar || point[1] < max[1]) {
				if(point[0] < max[0] && point[2] < max[2]) {
					retval = PY_TRUE;
				}
			}
		}
	}

	/*
	 * TODO: `glPolygonMode' can provide wireframes if we want a model
	 * 		 Wireframe rather than a box outline.
	 */
	if(py_int_get(dbgp)) {
		enum aga_draw_flags fl = aga_draw_get();

		if(aga_script_err(
				__FILE__, "aga_draw_set", aga_draw_set(AGA_DRAW_NONE))) {

			return 0;
		}

		glLineWidth(1.0f);
		if(aga_script_gl_err(__FILE__, "glLineWidth")) return 0;

		glBegin(GL_LINE_STRIP);
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3f(min[0], max[1], max[2]);
			glVertex3f(max[0], max[1], max[2]);
			glVertex3f(min[0], min[1], max[2]);
			glVertex3f(max[0], min[1], max[2]);
			glVertex3f(max[0], min[1], min[2]);
			glVertex3f(max[0], max[1], max[2]);
			glVertex3f(max[0], max[1], min[2]);
			glVertex3f(min[0], max[1], max[2]);
			glVertex3f(min[0], max[1], min[2]);
			glVertex3f(min[0], min[1], max[2]);
			glVertex3f(min[0], min[1], min[2]);
			glVertex3f(max[0], min[1], min[2]);
			glVertex3f(min[0], max[1], min[2]);
			glVertex3f(max[0], max[1], min[2]);
		glEnd();
		if(aga_script_gl_err(__FILE__, "glEnd")) return 0;

		if(aga_script_err(__FILE__, "aga_draw_set", aga_draw_set(fl))) {
			return 0;
		}
	}

	apro_stamp_end(APRO_SCRIPTGLUE_INOBJ);

	return py_object_incref(retval);
}

struct py_object* agan_objconf(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* o;
	struct py_object* l;
	struct py_object* retval;

	struct agan_object* obj;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_OBJCONF);

	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
		!aga_arg(&o, args, 0, PY_TYPE_INT) ||
		!aga_arg(&l, args, 1, PY_TYPE_LIST)) {

		return aga_arg_error("objconf", "int and list");
	}

	obj = (void*) py_int_get(o);

	retval = agan_scriptconf(&obj->config, ASYS_TRUE, l);

	apro_stamp_end(APRO_SCRIPTGLUE_OBJCONF);

	return retval ? retval : py_object_incref(PY_NONE);
}

static asys_bool_t agan_putobj_light(struct agan_lightdata* data) {
	unsigned ind = GL_LIGHT0 + data->index;
	float pos[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pos[3] = data->directional ? 0.0f : 1.0f;

	glEnable(ind);
	if(aga_script_gl_err(__FILE__, "glEnable")) return ASYS_TRUE;

	glLightfv(ind, GL_POSITION, pos);
	if(aga_script_gl_err(__FILE__, "glLightfv")) return ASYS_TRUE;

	glLightfv(ind, GL_AMBIENT, data->ambient);
	if(aga_script_gl_err(__FILE__, "glLightfv")) return ASYS_TRUE;

	glLightfv(ind, GL_DIFFUSE, data->diffuse);
	if(aga_script_gl_err(__FILE__, "glLightfv")) return ASYS_TRUE;

	glLightfv(ind, GL_SPECULAR, data->specular);
	if(aga_script_gl_err(__FILE__, "glLightfv")) return ASYS_TRUE;

	glLightf(ind, GL_CONSTANT_ATTENUATION, data->constant_attenuation);
	if(aga_script_gl_err(__FILE__, "glLightf")) return ASYS_TRUE;

	glLightf(ind, GL_LINEAR_ATTENUATION, data->linear_attenuation);
	if(aga_script_gl_err(__FILE__, "glLightf")) return ASYS_TRUE;

	glLightf(ind, GL_QUADRATIC_ATTENUATION, data->quadratic_attenuation);
	if(aga_script_gl_err(__FILE__, "glLightf")) return ASYS_TRUE;

	glLightf(ind, GL_SPOT_EXPONENT, data->exponent);
	if(aga_script_gl_err(__FILE__, "glLightf")) return ASYS_TRUE;

	glLightf(ind, GL_SPOT_CUTOFF, data->angle);
	if(aga_script_gl_err(__FILE__, "glLightf")) return ASYS_TRUE;

	glLightfv(ind, GL_SPOT_DIRECTION, data->direction);
	if(aga_script_gl_err(__FILE__, "glLightfv")) return ASYS_TRUE;

	return ASYS_FALSE;
}

/* TODO: Add option to diagnose bloated objects with a time budget. */
struct py_object* agan_putobj(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct agan_object* obj;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_PUTOBJ);

	apro_stamp_start(APRO_PUTOBJ_RISING);

	if(!aga_arg_list(args, PY_TYPE_INT)) {
		return aga_arg_error("putobj", "int");
	}

	obj = (void*) py_int_get(args);

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_gl_err(__FILE__, "glMatrixMode")) return 0;
	glPushMatrix();
	if(aga_script_gl_err(__FILE__, "glPushMatrix")) return 0;
	if(agan_settransmat(obj->transform, ASYS_FALSE)) return 0;

	apro_stamp_end(APRO_PUTOBJ_RISING);

	apro_stamp_start(APRO_PUTOBJ_LIGHT);

	if(obj->light_data && agan_putobj_light(obj->light_data)) return 0;

	apro_stamp_end(APRO_PUTOBJ_LIGHT);

	apro_stamp_start(APRO_PUTOBJ_CALL);

	glCallList(obj->drawlist);
	if(aga_script_gl_err(__FILE__, "glCallList")) return 0;

	apro_stamp_end(APRO_PUTOBJ_CALL);

	apro_stamp_start(APRO_PUTOBJ_FALLING);

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_gl_err(__FILE__, "glMatrixMode")) return 0;
	glPopMatrix();
	if(aga_script_gl_err(__FILE__, "glPopMatrix")) return 0;

	apro_stamp_end(APRO_PUTOBJ_FALLING);

	apro_stamp_end(APRO_SCRIPTGLUE_PUTOBJ);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_objtrans(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct agan_object* obj;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_OBJTRANS);

	if(!aga_arg_list(args, PY_TYPE_INT)) {
		return aga_arg_error("objtrans", "int");
	}

	obj = (void*) py_int_get(args);

	apro_stamp_end(APRO_SCRIPTGLUE_OBJTRANS);

	return py_object_incref(obj->transform);
}

struct py_object* agan_objind(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct agan_object* obj;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_OBJTRANS);

	if(!aga_arg_list(args, PY_TYPE_INT)) {
		return aga_arg_error("agan_objind", "int");
	}

	obj = (void*) py_int_get(args);

	apro_stamp_end(APRO_SCRIPTGLUE_OBJTRANS);

	return py_int_new(obj->ind);
}
