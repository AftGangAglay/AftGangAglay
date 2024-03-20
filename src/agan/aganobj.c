/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/aganobj.h>

#include <agagl.h>
#include <agautil.h>
#include <agaresult.h>
#include <agalog.h>
#include <agascript.h>
#include <agapack.h>
#include <agaio.h>
#include <agapyinc.h>
#include <agascripthelp.h>

#include <apro.h>

/* TODO: Some `aga_script_*err` disable with noverify. */

/* TODO: Report object-related errors with path. */

struct py_object* agan_mktrans(struct py_object* self, struct py_object* arg) {
	struct py_object* retval;
	struct py_object* list;
	struct py_object* f;
	aga_size_t i, j;

	(void) self;
	(void) arg;

	apro_stamp_start(APRO_SCRIPTGLUE_MKTRANS);

	if(!(retval = py_dict_new())) return 0;

	for(i = 0; i < 3; ++i) {
		if(!(list = py_list_new(3))) return 0;

		for(j = 0; j < 3; ++j) {
			if(!(f = py_float_new((i == 2 ? 1.0 : 0.0)))) return 0;
			if(aga_list_set(list, j, f)) return 0;
		}

		if(py_dict_insert(retval, agan_trans_components[i], list) == -1) {
			return 0;
		}
	}

	apro_stamp_end(APRO_SCRIPTGLUE_MKTRANS);

	return retval;
}

/*
 * TODO: Object init uses a lot of nonlinear conf lookup. If we just traversed
 * 		 The tree "as-is" it'd require the conf to be well-ordered (which we
 * 		 Don't want) - so we need a hybrid approach.
 */

static aga_bool_t agan_mkobj_trans(
		struct agan_object* obj, struct aga_conf_node* conf) {

	enum aga_result result;
	const char* path[2];

	struct py_object* l;
	struct py_object* o;
	aga_size_t i, j;
	float f;

	for(i = 0; i < 3; ++i) {
		path[0] = agan_conf_components[i];

		l = py_dict_lookup(obj->transform, agan_trans_components[i]);
		if(!l) return AGA_TRUE;

		for(j = 0; j < 3; ++j) {
			path[1] = agan_xyz[j];

			result = aga_conftree(
					conf->children, path, AGA_LEN(path), &f, AGA_FLOAT);
			if(result) f = 0.0f;

			if(!(o = py_float_new(f))) return 0;
			if(aga_list_set(l, j, o)) return 0;
		}
	}

	return AGA_FALSE;
}

static void agan_mkobj_extent(
		struct agan_object* obj, struct aga_conf_node* conf) {

	static const char* min_attr[] = { "MinX", "MinY", "MinZ" };
	static const char* max_attr[] = { "MaxX", "MaxY", "MaxZ" };

	float (* min)[3] = &obj->min_extent;
	float (* max)[3] = &obj->max_extent;

	aga_size_t i;

	for(i = 0; i < 3; ++i) {
		if(aga_conftree(conf, &min_attr[i], 1, &(*min)[i], AGA_FLOAT)) {
			(*min)[i] = 0.0f;
		}
	}

	for(i = 0; i < 3; ++i) {
		if(aga_conftree(conf, &max_attr[i], 1, &(*max)[i], AGA_FLOAT)) {
			(*max)[i] = 0.0f;
		}
	}
}

static aga_bool_t agan_mkobj_model(
		struct agan_object* obj, struct aga_conf_node* conf,
		struct aga_respack* pack, const char* objpath) {

	static const char* model = "Model";
	static const char* texture = "Texture";
	static const char* filter = "Filter";
	/* static const char* unlit = "Unlit"; */

	/* TODO: Opt to disable textures? */

	enum aga_result result;

	struct aga_res* res;
	unsigned mode = GL_COMPILE;
	const char* path;

	/* TODO: Delete lists in error conds. */
	obj->drawlist = glGenLists(1);
	if(aga_script_gl_err("glGenLists")) return 0;

#ifdef _DEBUG
	mode = GL_COMPILE_AND_EXECUTE;
#endif

	glNewList(obj->drawlist, mode);
	if(aga_script_gl_err("glNewList")) return 0;

	/* TODO: Handle unlit. */

	{
		int f;

		if(aga_conftree(conf->children, &filter, 1, &f, AGA_INTEGER)) {
			f = 1;
		}

		f = f ? GL_LINEAR : GL_NEAREST;

		/* TODO: Specific error type for failure to find entry (?). */
		if(aga_conftree(conf->children, &texture, 1, &path, AGA_STRING)) {
			aga_log(
					__FILE__, "warn: Object `%s' is missing a texture entry",
					objpath);
		}
		else {
			static const char* width = "Width";

			int w, h;

			/*
			 * TODO: "Just trusting" that this make/release pattern is safe to
			 * 		 Avoid leaks is unclear and unfriendly to non-maintainers.
			 * 		 We probably need better lifetime/cleanup etiquette and
			 * 		 Helper code.
			 */

			/*
			 * TODO: Handle missing textures etc. gracefully - default/
			 *       Procedural resources?
			 */
			result = aga_mkres(pack, path, &res);
			if(aga_script_err("aga_mkres", result)) return AGA_TRUE;

			result = aga_releaseres(res);
			if(aga_script_err("aga_releaseres", result)) return AGA_TRUE;

			result = aga_conftree(
					res->conf, &width, 1, &w, AGA_INTEGER);
			if(result) {
				/* TODO: Defaultable conf values as part of the API. */
				aga_log(__FILE__, "warn: Texture `%s' is missing dimensions");
				w = 1;
			}

			h = (int) (res->size / (aga_size_t) (4 * w));

			/* TODO: Mipmapping/detail levels. */
			glTexImage2D(
					GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
					GL_UNSIGNED_BYTE, res->data);
			/*
			 * TODO: Script land can probably handle lots of GL errors like
			 * 		 This relatively gracefully (i.e. allow the user code to go
			 * 		 Further without needing try-catch hell).
			 * 		 Especially in functions like this which aren't supposed to
			 * 		 Be run every frame.
			 */
			if(aga_script_gl_err("glTexImage2D")) return AGA_TRUE;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
			if(aga_script_gl_err("glTexParameteri")) return AGA_TRUE;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
			if(aga_script_gl_err("glTexParameteri")) return AGA_TRUE;
		}

		result = aga_conftree(
				conf->children, &model, 1, &path, AGA_STRING);
		if(result) {
			aga_log(
					__FILE__, "warn: Object `%s' is missing a model entry",
					objpath);
		}
		else {
			struct aga_vertex v;
			void* fp;
			aga_size_t i, len;

			result = aga_searchres(pack, path, &res);
			if(aga_script_err("aga_mkres", result)) return AGA_TRUE;

			agan_mkobj_extent(obj, res->conf);

			result = aga_resseek(res, &fp);
			/* TODO: We can't return during list build! */
			if(aga_script_err("aga_resfptr", result)) return AGA_TRUE;
			len = res->size;

			glBegin(GL_TRIANGLES);
			/* if(aga_script_gl_err("glBegin")) return 0; */

			for(i = 0; i < len; i += sizeof(v)) {
				result = aga_fread(&v, sizeof(v), pack->fp);
				if(aga_script_err("aga_fread", result)) return AGA_TRUE;

				glColor4fv(v.col);
				/* if(aga_script_gl_err("glColor4fv")) return AGA_TRUE; */
				glTexCoord2fv(v.uv);
				/* if(aga_script_gl_err("glTexCoord2fv")) return AGA_TRUE; */
				glNormal3fv(v.norm);
				/* if(aga_script_gl_err("glNormal3fv")) return AGA_TRUE; */
				glVertex3fv(v.pos);
				/* if(aga_script_gl_err("glVertex3fv")) return AGA_TRUE; */
			}

			glEnd();
			if(aga_script_gl_err("glEnd")) return 0;
		}
	}

	glEndList();
	if(aga_script_gl_err("glEndList")) return 0;

	return AGA_FALSE;
}

static aga_bool_t agan_mkobj_light(
		struct agan_object* obj, struct aga_conf_node* conf) {

	static const char* light = "Light";

	struct aga_conf_node* node = conf->children;
	struct agan_lightdata* data;

	int scr;
	aga_size_t i, j;

	if(aga_conftree_raw(node, &light, 1, &node)) return AGA_FALSE;

	if(!(obj->light_data = calloc(1, sizeof(struct agan_lightdata)))) {
		return AGA_TRUE;
	}
	data = obj->light_data;

	for(i = 0; i < node->len; ++i) {
		struct aga_conf_node* child = &node->children[i];

		if(aga_confvar("Index", child, AGA_INTEGER, &data->index)) {
			if(data->index > 7) {
				aga_log(
						__FILE__, "warn: Light index `%u' exceeds max of 7",
						data->index);
				free(data);
				obj->light_data = 0;
				return AGA_TRUE;
			}
		}
		else if(aga_confvar("Directional", child, AGA_INTEGER, &scr)) {
			data->directional = !!scr;
		}
		else if(aga_confvar("Exponent", child, AGA_FLOAT, &data->exponent)) {
			continue;
		}
		else if(aga_confvar("Angle", child, AGA_FLOAT, &data->angle)) {
			continue;
		}
		else if(aga_streql("Direction", child->name)) {
			for(j = 0; j < 3; ++j) {
				const char* comp = agan_xyz[j];
				float (*dir)[3] = &data->direction;

				if(aga_conftree(child, &comp, 1, &(*dir)[j], AGA_FLOAT)) {
					(*dir)[j] = 0.0f;
				}
			}
		}
		/* TODO: General API for getting multiple components from conf node. */
		else if(aga_streql("Ambient", child->name)) {
			float (*col)[4] = &data->ambient;

			for(j = 0; j < 3; ++j) {
				const char* comp = agan_rgb[j];

				if(aga_conftree(child, &comp, 1, &(*col)[j], AGA_FLOAT)) {
					(*col)[j] = 1.0f;
				}
			}

			(*col)[3] = 1.0f;
		}
		else if(aga_streql("Diffuse", child->name)) {
			float (*col)[4] = &data->diffuse;

			for(j = 0; j < 3; ++j) {
				const char* comp = agan_rgb[j];

				if(aga_conftree(child, &comp, 1, &(*col)[j], AGA_FLOAT)) {
					(*col)[j] = 1.0f;
				}
			}

			(*col)[3] = 1.0f;
		}
		else if(aga_streql("Specular", child->name)) {
			float (*col)[4] = &data->specular;

			for(j = 0; j < 3; ++j) {
				const char* comp = agan_rgb[j];

				if(aga_conftree(child, &comp, 1, &(*col)[j], AGA_FLOAT)) {
					(*col)[j] = 1.0f;
				}
			}

			(*col)[3] = 1.0f;
		}
		else if(aga_streql("Attenuation", child->name)) {
			static const char* atten[] = { "Constant", "Linear", "Quadratic" };
			float* at[3];
			at[0] = &data->constant_attenuation;
			at[1] = &data->linear_attenuation;
			at[2] = &data->quadratic_attenuation;
			for(j = 0; j < 3; ++j) {
				const char* comp = atten[j];

				if(aga_conftree(child, &comp, 1, at[j], AGA_FLOAT)) {
					*at[j] = 0.0f;
				}
			}
		}
	}

	/*
	aga_log(
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

	return AGA_FALSE;
}

/*
 * TODO: Failure states here are super leaky - we can probably compartmentalise
 * 		 This function a lot more to help remedy this.
 */
struct py_object* agan_mkobj(struct py_object* self, struct py_object* arg) {
	enum aga_result result;

	struct agan_object* obj;
	struct py_int* v;
	struct py_object* retval;
	struct aga_conf_node conf;
	aga_bool_t c = AGA_FALSE;
	aga_bool_t m = AGA_FALSE;

	const char* path;
	struct aga_respack* pack;

	(void) self;
	(void) arg;

	apro_stamp_start(APRO_SCRIPTGLUE_MKOBJ);

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;

	if(!aga_arg_list(arg, PY_TYPE_STRING)) {
		return aga_arg_error("mkobj", "string");
	}

	retval = py_int_new(0);
	v = (struct py_int*) retval;

	if(!(v->value = (py_value_t) calloc(1, sizeof(struct agan_object)))) {
		return py_error_set_nomem();
	}

	obj = (void*) v->value;
	obj->light_data = 0;
	if(!(obj->transform = agan_mktrans(0, 0))) goto cleanup;

	{
		void* fp;

		if(aga_script_string(arg, &path)) goto cleanup;

		result = aga_searchres(pack, path, &obj->res);
		if(aga_script_err("aga_searchres", result)) goto cleanup;

		result = aga_resseek(obj->res, &fp);
		if(aga_script_err("aga_resseek", result)) goto cleanup;

		result = aga_mkconf(fp, obj->res->size, &conf);
		if(aga_script_err("aga_resfptr", result)) goto cleanup;

		c = AGA_TRUE;
	}

	if(agan_mkobj_trans(obj, &conf)) goto cleanup;
	if(agan_mkobj_model(obj, &conf, pack, path)) goto cleanup;
	if(agan_mkobj_light(obj, &conf)) goto cleanup;

	m = AGA_TRUE;

	result = aga_killconf(&conf);
	if(aga_script_err("aga_killconf", result)) goto cleanup;

	apro_stamp_end(APRO_SCRIPTGLUE_MKOBJ);

	return (struct py_object*) retval;

	cleanup:
	{
		if(c) {
			result = aga_killconf(&conf);
			if(aga_script_err("aga_killconf", result)) return 0;
		}

		if(m) {
			glDeleteLists(obj->drawlist, 1);
			if(aga_script_gl_err("glDeleteLists")) return 0;
		}

		free(obj->light_data);
		py_object_decref(obj->transform);
		free((void*) v->value);
		py_object_decref(retval);

		return 0;
	};
}

struct py_object* agan_killobj(struct py_object* self, struct py_object* arg) {
	struct agan_object* obj;
	struct py_int* v;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_KILLOBJ);

	if(!aga_arg_list(arg, PY_TYPE_INT)) {
		return aga_arg_error("killobj", "int");
	}

	v = (struct py_int*) arg;
	obj = (void*) v->value;

	glDeleteLists(obj->drawlist, 1);
	if(aga_script_gl_err("glDeleteLists")) return 0;

	py_object_decref(obj->transform);

	apro_stamp_end(APRO_SCRIPTGLUE_KILLOBJ);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_inobj(struct py_object* self, struct py_object* arg) {
	struct py_object* retval = PY_FALSE;
	struct py_object* o;
	struct py_object* j;
	struct py_object* dbg;
	struct py_object* point;
	struct py_object* flobj;
	struct py_object* scale;
	struct py_object* pos;

	float pt[3];
	float mins[3];
	float maxs[3];
	float f;
	aga_bool_t p, d;
	aga_size_t i;
	struct agan_object* obj;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_INOBJ);

	/* TODO: Handle 90 degree rotations as a special case. */

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&o, arg, 0, PY_TYPE_INT) ||
	   !aga_arg(&point, arg, 1, PY_TYPE_LIST) ||
	   !aga_arg(&j, arg, 2, PY_TYPE_INT) ||
	   !aga_arg(&dbg, arg, 3, PY_TYPE_INT)) {

		return aga_arg_error("inobj", "int, list, int and int");
	}

	if(aga_script_bool(j, &p)) return 0;
	if(aga_script_bool(dbg, &d)) return 0;

	obj = (void*) ((struct py_int*) o)->value;
	memcpy(mins, obj->min_extent, sizeof(mins));
	memcpy(maxs, obj->max_extent, sizeof(maxs));

	if(!(scale = py_dict_lookup(obj->transform, "scale"))) return 0;
	if(!(pos = py_dict_lookup(obj->transform, "pos"))) return 0;

	for(i = 0; i < 3; ++i) {
		/* TODO: Static "get N items into buffer" to make noverify easier. */

		if(aga_list_get(point, i, &flobj)) return 0;
		if(aga_script_float(flobj, &pt[i])) return 0;

		if(aga_list_get(scale, i, &flobj)) return 0;
		if(aga_script_float(flobj, &f)) return 0;

		mins[i] *= f;
		maxs[i] *= f;
	}

	for(i = 0; i < 3; ++i) {
		if(aga_list_get(pos, i, &flobj)) return 0;
		if(aga_script_float(flobj, &f)) return 0;

		mins[i] += f;
		maxs[i] += f;
	}

	/* TODO: Once cells are implemented check against current cell rad. */

	if(pt[0] > mins[0] && (p || pt[1] > mins[1]) && pt[2] > mins[2]) {
		if(pt[0] < maxs[0] && (p || pt[1] < maxs[1]) && pt[2] < maxs[2]) {
			retval = PY_TRUE;
		}
	}

	if(d) {
		glDisable(GL_TEXTURE_2D);
		if(aga_script_gl_err("glDisable")) return 0;
		glDisable(GL_DEPTH_TEST);
		if(aga_script_gl_err("glDisable")) return 0;
		/*glDisable(GL_LIGHTING);
		if(aga_script_gl_err("glDisable")) return 0;*/

		glBegin(GL_LINE_STRIP);
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3f(mins[0], maxs[1], maxs[2]);
			glVertex3f(maxs[0], maxs[1], maxs[2]);
			glVertex3f(mins[0], mins[1], maxs[2]);
			glVertex3f(maxs[0], mins[1], maxs[2]);
			glVertex3f(maxs[0], mins[1], mins[2]);
			glVertex3f(maxs[0], maxs[1], maxs[2]);
			glVertex3f(maxs[0], maxs[1], mins[2]);
			glVertex3f(mins[0], maxs[1], maxs[2]);
			glVertex3f(mins[0], maxs[1], mins[2]);
			glVertex3f(mins[0], mins[1], maxs[2]);
			glVertex3f(mins[0], mins[1], mins[2]);
			glVertex3f(maxs[0], mins[1], mins[2]);
			glVertex3f(mins[0], maxs[1], mins[2]);
			glVertex3f(maxs[0], maxs[1], mins[2]);
		glEnd();
		if(aga_script_gl_err("glEnd")) return 0;

		glEnable(GL_TEXTURE_2D);
		if(aga_script_gl_err("glEnable")) return 0;
		glEnable(GL_DEPTH_TEST);
		if(aga_script_gl_err("glEnable")) return 0;
		/*glEnable(GL_LIGHTING);
		if(aga_script_gl_err("glEnable")) return 0;*/
	}

	apro_stamp_end(APRO_SCRIPTGLUE_INOBJ);

	return py_object_incref(retval);
}

/* TODO: Avoid reloading conf for every call. */
struct py_object* agan_objconf(struct py_object* self, struct py_object* arg) {
	enum aga_result result;

	void* fp;

	struct py_object* o;
	struct py_object* l;
	struct py_object* retval;

	struct aga_conf_node conf;
	struct agan_object* obj;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_OBJCONF);

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&o, arg, 0, PY_TYPE_INT) ||
	   !aga_arg(&l, arg, 1, PY_TYPE_LIST)) {

		return aga_arg_error("objconf", "int and list");
	}

	obj = (void*) ((struct py_int*) o)->value;

	result = aga_resseek(obj->res, &fp);
	if(aga_script_err("aga_resseek", result)) return 0;

	/* TODO: Set aga_conf_debug_file = obj->res. whatever. */
	result = aga_mkconf(fp, obj->res->size, &conf);
	if(aga_script_err("aga_mkconf", result)) return 0;

	retval = agan_scriptconf(&conf, AGA_TRUE, l);
	if(!retval) return 0;

	result = aga_killconf(&conf);
	if(aga_script_err("aga_killconf", result)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_OBJCONF);

	return retval;
}

static aga_bool_t agan_putobj_light(struct agan_lightdata* data) {
	unsigned ind = GL_LIGHT0 + data->index;
	float pos[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pos[3] = data->directional ? 0.0f : 1.0f;

	glEnable(ind);
	if(aga_script_gl_err("glEnable")) return AGA_TRUE;

	glLightfv(ind, GL_POSITION, pos);
	if(aga_script_gl_err("glLightfv")) return AGA_TRUE;

	glLightfv(ind, GL_AMBIENT, data->ambient);
	if(aga_script_gl_err("glLightfv")) return AGA_TRUE;

	glLightfv(ind, GL_DIFFUSE, data->ambient);
	if(aga_script_gl_err("glLightfv")) return AGA_TRUE;

	glLightfv(ind, GL_SPECULAR, data->specular);
	if(aga_script_gl_err("glLightfv")) return AGA_TRUE;

	glLightf(ind, GL_CONSTANT_ATTENUATION, data->constant_attenuation);
	if(aga_script_gl_err("glLightf")) return AGA_TRUE;

	glLightf(ind, GL_LINEAR_ATTENUATION, data->linear_attenuation);
	if(aga_script_gl_err("glLightf")) return AGA_TRUE;

	glLightf(ind, GL_SPOT_EXPONENT, data->exponent);
	if(aga_script_gl_err("glLightf")) return AGA_TRUE;

	glLightf(ind, GL_SPOT_CUTOFF, data->angle);
	if(aga_script_gl_err("glLightf")) return AGA_TRUE;

	glLightf(ind, GL_QUADRATIC_ATTENUATION,data->quadratic_attenuation);
	if(aga_script_gl_err("glLightf")) return AGA_TRUE;

	glLightfv(ind, GL_SPOT_DIRECTION, data->direction);
	if(aga_script_gl_err("glLightfv")) return AGA_TRUE;

	return AGA_FALSE;
}

struct py_object* agan_putobj(struct py_object* self, struct py_object* arg) {
	struct py_int* v;
	struct agan_object* obj;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_PUTOBJ);

	apro_stamp_start(APRO_PUTOBJ_RISING);

	if(!aga_arg_list(arg, PY_TYPE_INT)) {
		return aga_arg_error("putobj", "int");
	}

	v = (struct py_int*) arg;
	obj = (void*) v->value;

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_gl_err("glMatrixMode")) return 0;
	glPushMatrix();
	if(aga_script_gl_err("glPushMatrix")) return 0;
	if(agan_settransmat(obj->transform, AGA_FALSE)) return 0;

	apro_stamp_end(APRO_PUTOBJ_RISING);

	apro_stamp_start(APRO_PUTOBJ_LIGHT);

	if(obj->light_data && agan_putobj_light(obj->light_data)) return 0;

	apro_stamp_end(APRO_PUTOBJ_LIGHT);

	apro_stamp_start(APRO_PUTOBJ_CALL);

	glCallList(obj->drawlist);
	if(aga_script_gl_err("glCallList")) return 0;

	apro_stamp_end(APRO_PUTOBJ_CALL);

	apro_stamp_start(APRO_PUTOBJ_FALLING);

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_gl_err("glMatrixMode")) return 0;
	glPopMatrix();
	if(aga_script_gl_err("glPopMatrix")) return 0;

	apro_stamp_end(APRO_PUTOBJ_FALLING);

	apro_stamp_end(APRO_SCRIPTGLUE_PUTOBJ);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_objtrans(struct py_object* self, struct py_object* arg) {
	struct agan_object* obj;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_OBJTRANS);

	if(!aga_arg_list(arg, PY_TYPE_INT)) {
		return aga_arg_error(
				"objtrans", "int");
	}

	obj = (void*) ((struct py_int*) arg)->value;

	apro_stamp_end(APRO_SCRIPTGLUE_OBJTRANS);

	return obj->transform;
}
