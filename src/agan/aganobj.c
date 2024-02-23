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

/* TODO: Report object-related errors with path.  */

AGAN_SCRIPTPROC(mktrans) {
    aga_pyobject_t retval, list, f;
    aga_size_t i, j;

	if(!(retval = newdictobject())) return 0;

    for(i = 0; i < 3; ++i) {
		if(!(list = newlistobject(3))) return 0;

        for(j = 0; j < 3; ++j) {
			if(!(f = newfloatobject((i == 2 ? 1.0 : 0.0)))) return 0;
			if(aga_list_set(list, j, f)) return 0;
        }

        if(dictinsert(retval, (char*) agan_trans_components[i], list) == -1) {
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

    const char* path[2];

    aga_pyobject_t l, o;
    aga_size_t i, j;
    float f;

    for(i = 0; i < 3; ++i) {
        path[0] = agan_conf_components[i];

        l = dictlookup(obj->transform, (char*) agan_trans_components[i]);
        if(!l) return AGA_TRUE;

        for(j = 0; j < 3; ++j) {
            path[1] = agan_xyz[j];

            if(aga_conftree(conf, path, AGA_LEN(path), &f, AGA_FLOAT)) {
                f = 0.0f;
            }

			if(!(o = newfloatobject(f))) return 0;
			if(aga_list_set(l, j, o)) return 0;
        }
    }

	return AGA_FALSE;
}

static aga_bool_t agan_mkobj_model(
		struct agan_object* obj, struct aga_conf_node* conf,
		struct aga_respack* pack) {

	static const char* model = "Model";
	static const char* texture = "Texture";
    static const char* filter = "Filter";
    /* static const char* unlit = "Unlit"; */

	/* TODO: Opt to disable textures? */

	enum aga_result err;

	struct aga_res* res;
	unsigned mode = GL_COMPILE;
	const char* path;

	obj->drawlist = glGenLists(1);
	if(aga_script_glerr("glGenLists")) return 0;

#ifdef _DEBUG
    mode = GL_COMPILE_AND_EXECUTE;
#endif

	glNewList(obj->drawlist, mode);
	if(aga_script_glerr("glNewList")) return 0;

	/* TODO: Handle unlit. */

	{
		int f;

		if(aga_conftree(conf, &filter, 1, &f, AGA_INTEGER)) f = 1;
		f = f ? GL_LINEAR : GL_NEAREST;

		/* TODO: Specific error type for failure to find entry (?). */
		if(aga_conftree(conf, &texture, 1, &path, AGA_STRING)) {
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
             *       Proceedural resources?
             */
			err = aga_mkres(pack, path, &res);
			if(aga_script_err("aga_mkres", err)) return AGA_TRUE;

			err = aga_releaseres(res);
			if(aga_script_err("aga_releaseres", err)) return AGA_TRUE;

			err = aga_conftree_nonroot(res->conf, &width, 1, &w, AGA_INTEGER);
			if(err) {
				/* TODO: Defaultable conf values as part of the API. */
				aga_log(__FILE__, "warn: Texture `%s' is missing dimensions");
				w = 1;
			}

			h = (int) (res->size / (aga_size_t) (4 * w));

			/* TODO: Mipmapping/detail levels. */
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				res->data);
			/*
			 * TODO: Script land can probably handle lots of GL errors like
			 * 		 This relatively gracefully (i.e. allow the user code to go
			 * 		 Further without needing try-catch hell).
			 * 		 Especially in functions like this which aren't supposed to
			 * 		 Be run every frame.
			 */
			if(aga_script_glerr("glTexImage2D")) return AGA_TRUE;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
			if(aga_script_glerr("glTexParameteri")) return AGA_TRUE;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
			if(aga_script_glerr("glTexParameteri")) return AGA_TRUE;
		}

		if(aga_conftree(conf, &model, 1, &path, AGA_STRING)) {
			aga_log(__FILE__, "warn: Object `%s' is missing a model entry");
		}
		else {
			struct aga_vertex v;
			void* fp;
			aga_size_t i, len;

			err = aga_resfptr(pack, path, &fp, &len);
			if(aga_script_err("aga_resfptr", err)) return AGA_TRUE;

			glBegin(GL_TRIANGLES);
			/* if(aga_script_glerr("glBegin")) return 0; */

			for(i = 0; i < len; i += sizeof(v)) {
				err = aga_fread(&v, sizeof(v), pack->fp);
				if(aga_script_err("aga_fread", err)) return AGA_TRUE;

				glColor4fv(v.col);
				/* if(aga_script_glerr("glColor4fv")) return AGA_TRUE; */
				glTexCoord2fv(v.uv);
				/* if(aga_script_glerr("glTexCoord2fv")) return AGA_TRUE; */
				glNormal3fv(v.norm);
				/* if(aga_script_glerr("glNormal3fv")) return AGA_TRUE; */
				glVertex3fv(v.pos);
				/* if(aga_script_glerr("glVertex3fv")) return AGA_TRUE; */
			}

			glEnd();
			if(aga_script_glerr("glEnd")) return 0;
		}
	}

	glEndList();
	if(aga_script_glerr("glEndList")) return 0;

	return AGA_FALSE;
}

/*
 * TODO: Failure states here are super leaky - we can probably compartmentalise
 * 		 This function a lot more to help remedy this.
 */
AGAN_SCRIPTPROC(mkobj) {
	enum aga_result err;

	struct agan_nativeptr* nativeptr;
	struct agan_object* obj;
	aga_pyobject_t retval;
	struct aga_conf_node conf;

	struct aga_respack* pack;

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;

	if(!AGA_ARGLIST(string)) AGA_ARGERR("mkobj", "string");

	retval = agan_mknativeptr(0);
	nativeptr = (struct agan_nativeptr*) retval;

	if(!(nativeptr->ptr = calloc(1, sizeof(struct agan_object))))
		return err_nomem();

	obj = nativeptr->ptr;
	if(!(obj->transform = agan_mktrans(0, 0))) return 0;

	{
		char* path;
		void* fp;

		if(aga_script_string(arg, &path)) return 0;

		err = aga_searchres(pack, path, &obj->res);
		if(aga_script_err("aga_searchres", err)) return 0;

		err = aga_resseek(obj->res, &fp);
		if(aga_script_err("aga_resseek", err)) return 0;

		err = aga_mkconf(fp, obj->res->size, &conf);
		if(aga_script_err("aga_resfptr", err)) return 0;
	}

	if(agan_mkobj_trans(obj, &conf)) return 0;
	if(agan_mkobj_model(obj, &conf, pack)) return 0;

	return (aga_pyobject_t) retval;
}

AGAN_SCRIPTPROC(killobj) {
	struct agan_nativeptr* nativeptr;
	struct agan_object* obj;

	if(!AGA_ARGLIST(nativeptr)) AGA_ARGERR("killobj", "nativeptr");

	nativeptr = (struct agan_nativeptr*) arg;
	obj = nativeptr->ptr;

	glDeleteLists(obj->drawlist, 1);
	if(aga_script_glerr("glDeleteLists")) return 0;

	DECREF(obj->transform);

	return AGA_INCREF(None);
}

AGAN_SCRIPTPROC(inobj) {
	aga_pyobject_t retval = PyFalse;
	aga_pyobject_t o, j, dbg, point, flobj, scale, pos;
	float pt[3];
	float mins[3];
	float maxs[3];
	float f;
	aga_bool_t p, d;
	aga_size_t i;
	struct agan_object* obj;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(o, 0, nativeptr) ||
		!AGA_ARG(point, 1, list) || !AGA_ARG(j, 2, int) ||
		!AGA_ARG(dbg, 3, int)) {

		AGA_ARGERR("inobj", "nativeptr, list, int and int");
	}

	if(aga_script_bool(j, &p)) return 0;
	if(aga_script_bool(dbg, &d)) return 0;

	obj = ((struct agan_nativeptr*) o)->ptr;
	memcpy(mins, obj->min_extent, sizeof(mins));
	memcpy(maxs, obj->max_extent, sizeof(maxs));

	if(!(scale = dictlookup(obj->transform, "scale"))) return 0;
	if(!(pos = dictlookup(obj->transform, "pos"))) return 0;

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
			retval = PyTrue;
		}
	}

	(void) d;
	/*
	if(d) {
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
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
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
	}
	 */

	return AGA_INCREF(retval);
}

/* TODO: Avoid reloading conf for every call. */
AGAN_SCRIPTPROC(objconf) {
	enum aga_result result;

	void* fp;

	aga_pyobject_t o, l, retval;

	struct aga_conf_node conf;
	struct agan_object* obj;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(o, 0, nativeptr) ||
		!AGA_ARG(l, 1, list)) {

		AGA_ARGERR("objconf", "nativeptr and list");
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

AGAN_SCRIPTPROC(putobj) {
	struct agan_nativeptr* nativeptr;
	struct agan_object* obj;

	if(!AGA_ARGLIST(nativeptr)) AGA_ARGERR("putobj", "nativeptr");

	nativeptr = (struct agan_nativeptr*) arg;
	obj = nativeptr->ptr;

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_glerr("glMatrixMode")) return 0;
		glPushMatrix();
		if(aga_script_glerr("glPushMatrix")) return 0;
		if(agan_settransmat(obj->transform, AGA_FALSE)) return 0;

	glCallList(obj->drawlist);
	if(aga_script_glerr("glCallList")) return 0;

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_glerr("glMatrixMode")) return 0;
		glPopMatrix();
		if(aga_script_glerr("glPopMatrix")) return 0;

	return AGA_INCREF(None);
}

AGAN_SCRIPTPROC(objtrans) {
	struct agan_object* obj;

	if(!AGA_ARGLIST(nativeptr)) AGA_ARGERR("objtrans", "nativeptr");

	obj = ((struct agan_nativeptr*) arg)->ptr;

	return obj->transform;
}
