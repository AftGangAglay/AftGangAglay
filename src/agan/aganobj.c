/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/aganobj.h>

#include <agagl.h>
#include <agaresult.h>
#include <agalog.h>
#include <agascript.h>
#include <agapack.h>
#include <agaio.h>
#include <agapyinc.h>
#include <agascripthelp.h>

/* TODO: Some `aga_script_*err` disable with noverify. */

/* TODO: Report object-related errors with path. */

struct py_object* agan_mktrans(struct py_object* self, struct py_object* arg) {
	struct py_object* retval;
	struct py_object* list;
	struct py_object* f;
	aga_size_t i, j;

	(void) self;
	(void) arg;

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
		struct aga_respack* pack) {

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
			aga_log(__FILE__, "warn: Object `%s' is missing a texture entry");
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
			aga_log(__FILE__, "warn: Object `%s' is missing a model entry");
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

/*
 * TODO: Failure states here are super leaky - we can probably compartmentalise
 * 		 This function a lot more to help remedy this.
 */
struct py_object* agan_mkobj(struct py_object* self, struct py_object* arg) {
	enum aga_result result;

	struct agan_nativeptr* nativeptr;
	struct agan_object* obj;
	struct py_object* retval;
	struct aga_conf_node conf;
	aga_bool_t c = AGA_FALSE;
	aga_bool_t m = AGA_FALSE;

	struct aga_respack* pack;

	(void) self;
	(void) arg;

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;

	if(!aga_arg_list(arg, &py_string_type)) {
		return aga_arg_error("mkobj", "string");
	}

	retval = agan_mknativeptr(0);
	nativeptr = (struct agan_nativeptr*) retval;

	if(!(nativeptr->ptr = calloc(1, sizeof(struct agan_object)))) {
		return py_error_set_nomem();
	}

	obj = nativeptr->ptr;
	if(!(obj->transform = agan_mktrans(0, 0))) goto cleanup;

	{
		const char* path;
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
	if(agan_mkobj_model(obj, &conf, pack)) goto cleanup;

	m = AGA_TRUE;

	result = aga_killconf(&conf);
	if(aga_script_err("aga_killconf", result)) goto cleanup;

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

		py_object_decref(obj->transform);
		free(nativeptr->ptr);
		py_object_decref(retval);

		return 0;
	};
}

struct py_object* agan_killobj(struct py_object* self, struct py_object* arg) {
	struct agan_nativeptr* nativeptr;
	struct agan_object* obj;

	(void) self;

	if(!aga_arg_list(arg, &agan_nativeptr_type)) {
		return aga_arg_error(
				"killobj", "nativeptr");
	}

	nativeptr = (struct agan_nativeptr*) arg;
	obj = nativeptr->ptr;

	glDeleteLists(obj->drawlist, 1);
	if(aga_script_gl_err("glDeleteLists")) return 0;

	py_object_decref(obj->transform);

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

	/* TODO: Handle 90 degree rotations as a special case. */

	if(!aga_arg_list(arg, &py_tuple_type) ||
	   !aga_arg(&o, arg, 0, &agan_nativeptr_type) ||
	   !aga_arg(&point, arg, 1, &py_list_type) ||
	   !aga_arg(&j, arg, 2, &py_int_type) ||
	   !aga_arg(&dbg, arg, 3, &py_int_type)) {

		return aga_arg_error("inobj", "nativeptr, list, int and int");
	}

	if(aga_script_bool(j, &p)) return 0;
	if(aga_script_bool(dbg, &d)) return 0;

	obj = ((struct agan_nativeptr*) o)->ptr;
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
	}

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

	if(!aga_arg_list(arg, &py_tuple_type) ||
	   !aga_arg(&o, arg, 0, &agan_nativeptr_type) ||
	   !aga_arg(&l, arg, 1, &py_list_type)) {

		return aga_arg_error("objconf", "nativeptr and list");
	}

	obj = ((struct agan_nativeptr*) o)->ptr;

	result = aga_resseek(obj->res, &fp);
	if(aga_script_err("aga_resseek", result)) return 0;

	result = aga_mkconf(fp, obj->res->size, &conf);
	if(aga_script_err("aga_mkconf", result)) return 0;

	retval = agan_scriptconf(&conf, AGA_TRUE, l);
	if(!retval) return 0;

	result = aga_killconf(&conf);
	if(aga_script_err("aga_killconf", result)) return 0;

	return retval;
}

struct py_object* agan_putobj(struct py_object* self, struct py_object* arg) {
	struct agan_nativeptr* nativeptr;
	struct agan_object* obj;

	(void) self;

	if(!aga_arg_list(arg, &agan_nativeptr_type)) {
		return aga_arg_error(
				"putobj", "nativeptr");
	}

	nativeptr = (struct agan_nativeptr*) arg;
	obj = nativeptr->ptr;

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_gl_err("glMatrixMode")) return 0;
	glPushMatrix();
	if(aga_script_gl_err("glPushMatrix")) return 0;
	if(agan_settransmat(obj->transform, AGA_FALSE)) return 0;

	glCallList(obj->drawlist);
	if(aga_script_gl_err("glCallList")) return 0;

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_gl_err("glMatrixMode")) return 0;
	glPopMatrix();
	if(aga_script_gl_err("glPopMatrix")) return 0;

	return py_object_incref(PY_NONE);
}

struct py_object* agan_objtrans(struct py_object* self, struct py_object* arg) {
	struct agan_object* obj;

	(void) self;

	if(!aga_arg_list(arg, &agan_nativeptr_type)) {
		return aga_arg_error(
				"objtrans", "nativeptr");
	}

	obj = ((struct agan_nativeptr*) arg)->ptr;

	return obj->transform;
}
