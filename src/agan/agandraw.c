/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/agandraw.h>

#include <agascript.h>
#include <agascripthelp.h>
#include <agastartup.h>
#include <agawin.h>
#include <agagl.h>
#include <agadraw.h>
#include <agalog.h>

#include <apro.h>

enum aga_result agan_draw_register(void) {
	enum aga_result result;

	if((result = aga_insertint("BACKFACE", AGA_DRAW_BACKFACE))) return result;
	if((result = aga_insertint("BLEND", AGA_DRAW_BLEND))) return result;
	if((result = aga_insertint("FOG", AGA_DRAW_FOG))) return result;
	if((result = aga_insertint("TEXTURE", AGA_DRAW_TEXTURE))) return result;
	if((result = aga_insertint("LIGHTING", AGA_DRAW_LIGHTING))) return result;
	if((result = aga_insertint("DEPTH", AGA_DRAW_DEPTH))) return result;
	if((result = aga_insertint("SHADEFLAT", AGA_DRAW_FLAT))) return result;

	if((result = aga_insertint("FRONT", AGAN_SURFACE_FRONT))) return result;
	if((result = aga_insertint("BACK", AGAN_SURFACE_BACK))) return result;
	if((result = aga_insertint("STENCIL", AGAN_SURFACE_STENCIL))) {
		return result;
	}
	if((result = aga_insertint("DEPTHBUF", AGAN_SURFACE_DEPTH))) return result;

	return AGA_RESULT_OK;
}

struct py_object* agan_setcam(struct py_object* self, struct py_object* args) {
	struct py_object* t;
	struct py_object* mode;
	aga_bool_t b;
	double ar;

	struct aga_opts* opts;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_SETCAM);

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
	   !aga_arg(&t, args, 0, PY_TYPE_DICT) ||
	   !aga_arg(&mode, args, 1, PY_TYPE_INT)) {

		return aga_arg_error("setcam", "dict and int");
	}

	if(aga_script_bool(mode, &b)) return 0;

	ar = (double) opts->height / (double) opts->width;

	glMatrixMode(GL_PROJECTION);
	if(aga_script_gl_err("glMatrixMode")) return 0;
	glLoadIdentity();
	if(aga_script_gl_err("glLoadIdentity")) return 0;

	if(b) {
		gluPerspective(opts->fov, 1.0 / ar, 0.1, 10000.0);
		if(aga_script_gl_err("gluPerspective")) return 0;
	}
	else {
		glOrtho(-1.0, 1.0, -ar, ar, 0.001, 1.0);
		if(aga_script_gl_err("glOrtho")) return 0;
	}

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_gl_err("glMatrixMode")) return 0;
	glLoadIdentity();
	if(aga_script_gl_err("glLoadIdentity")) return 0;
	if(agan_settransmat(t, AGA_TRUE)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_SETCAM);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_text(struct py_object* self, struct py_object* args) {
	const char* text;
	struct py_object* str;
	struct py_object* t;
	struct py_object* f;
	double x, y;

	AGAN_DEPRCALL("agan.text", "agan.text2d");

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_TEXT);

	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
	   !aga_arg(&str, args, 0, PY_TYPE_STRING) ||
	   !aga_arg(&t, args, 1, PY_TYPE_LIST)) {

		return aga_arg_error("text", "string and list");
	}

	if(aga_script_string(str, &text)) return 0;

	/* TODO: Use general list get N items API here. */
	if(aga_list_get(t, 0, &f)) return 0;
	if(aga_script_float(f, &x)) return 0;

	if(aga_list_get(t, 1, &f)) return 0;
	if(aga_script_float(f, &y)) return 0;

	if(aga_script_err("aga_puttext", aga_puttext((float) x, (float) y, text))) {
		return 0;
	}

	apro_stamp_end(APRO_SCRIPTGLUE_TEXT);

	return py_object_incref(PY_NONE);
}

/*
 * NOTE: Fog params follow the following format:
 * 		 [ density(norm), start, end ]
 */
struct py_object* agan_fogparam(
		struct py_object* self, struct py_object* args) {

	struct py_object* v;
	double f;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_FOGPARAM);

	if(!aga_arg_list(args, PY_TYPE_LIST)) {
		return aga_arg_error("fogparam", "list");
	}

	glFogi(GL_FOG_MODE, GL_EXP);
	if(aga_script_gl_err("glFogi")) return 0;

	if(aga_list_get(args, 0, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_DENSITY, (float) f);
	if(aga_script_gl_err("glFogf")) return 0;

	if(aga_list_get(args, 1, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_START, (float) f);
	if(aga_script_gl_err("glFogf")) return 0;

	if(aga_list_get(args, 2, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_END, (float) f);
	if(aga_script_gl_err("glFogf")) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_FOGPARAM);

	return py_object_incref(PY_NONE);
}

/*
 * TODO: We should ideally have a consolidated way to get vector-y types
 * 		 Out of scriptland.
 */
struct py_object* agan_fogcol(struct py_object* self, struct py_object* args) {
	struct py_object* v;
	aga_size_t i;
	double f;
	float col[3];

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_FOGCOL);

	if(!aga_arg_list(args, PY_TYPE_LIST)) {
		return aga_arg_error("fogcol", "list");
	}

	for(i = 0; i < 3; ++i) {
		if(aga_list_get(args, i, &v)) return 0;
		if(aga_script_float(v, &f)) return 0;
		col[i] = (float) f;
	}

	glFogfv(GL_FOG_COLOR, col);
	if(aga_script_gl_err("glFogfv")) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_FOGCOL);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_clear(struct py_object* self, struct py_object* args) {
	enum aga_result result;

	struct py_object* v;
	double f;
	float col[4];
	aga_size_t i;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_CLEAR);

	if(!aga_arg_list(args, PY_TYPE_LIST)) return aga_arg_error("clear", "list");

	for(i = 0; i < AGA_LEN(col); ++i) {
		if(aga_list_get(args, i, &v)) return 0;
		if(aga_script_float(v, &f)) return 0;
		col[i] = (float) f;
	}

	result = aga_clear(col);
	if(aga_script_err("aga_clear", result)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_CLEAR);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_mktrans(struct py_object* self, struct py_object* args) {
	struct py_object* retval;
	struct py_object* list;
	struct py_object* f;
	aga_size_t i, j;

	(void) self;
	(void) args;

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

/* TODO: Move to draw flag. */
struct py_object* agan_shadeflat(
		struct py_object* self, struct py_object* args) {

	py_value_t v;

	AGAN_DEPRCALL("agan.shadeflat", "agan.setflag");

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_SHADEFLAT);

	if(!aga_arg_list(args, PY_TYPE_INT)) {
		return aga_arg_error("shadeflat", "int");
	}

	if(aga_script_int(args, &v)) return 0;

	glShadeModel(v ? GL_FLAT : GL_SMOOTH);
	if(aga_script_gl_err("glShadeModel")) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_SHADEFLAT);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_getpix(struct py_object* self, struct py_object* args) {
	static aga_uint_t surface_names[] = {
			GL_FRONT, GL_BACK, GL_STENCIL, GL_DEPTH };

	aga_uint8_t pix[3];
	py_value_t x, y;
	unsigned i;
	int h;
	struct py_object* xo;
	struct py_object* yo;
	struct py_object* retval;
	struct py_object* list;
	/* TODO: Gracefully handle single vs. double buffered envs. */
	enum agan_surface surface = AGAN_SURFACE_BACK;

	struct aga_win* win;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_GETPIX);

	if(!(win = aga_getscriptptr(AGA_SCRIPT_WIN))) return 0;

	list = args;
	if(!aga_arg_list(args, PY_TYPE_LIST)) {
		struct py_object* target;

		if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
			!aga_arg(&list, args, 0, PY_TYPE_LIST) ||
			!aga_arg(&target, args, 1, PY_TYPE_INT)) {

			return aga_arg_error("getpix", "list and int");
		}
		else {
			py_value_t v;
			if(aga_script_int(target, &v)) return 0;

			if(v < AGAN_SURFACE_FRONT || v > AGAN_SURFACE_DEPTH) {
				aga_log(__FILE__, "err: Surface name out of range `%u'", v);
				py_error_set_badarg();
				return 0;
			}

			surface = v;
		}
	}

	if(aga_list_get(list, 0, &xo)) return 0;
	if(aga_list_get(list, 1, &yo)) return 0;

	if(aga_script_int(xo, &x)) return 0;
	if(aga_script_int(yo, &y)) return 0;

	glReadBuffer(surface_names[surface]);
	if(aga_script_gl_err("glReadBuffer")) return 0;

	/* TODO: Invert `y' to start from top left. */
	h = (int) (win->height - y);
	glReadPixels((int) x, h, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pix);
	if(aga_script_gl_err("glReadPixels")) return 0;

	if(!(retval = py_list_new(AGA_LEN(pix)))) return 0;

	for(i = 0; i < AGA_LEN(pix); ++i) {
		struct py_object* v;

		if(!(v = py_int_new(pix[i]))) return 0;
		if(aga_list_set(retval, i, v)) return 0;
	}

	apro_stamp_end(APRO_SCRIPTGLUE_GETPIX);

	return retval;
}

/* TODO: Could just take a list here. */
struct py_object* agan_setflag(
		struct py_object* self, struct py_object* args) {

	py_value_t v;

	(void) self;

	if(!aga_arg_list(args, PY_TYPE_INT)) {
		return aga_arg_error("setflag", "int");
	}

	if(aga_script_int(args, &v)) return 0;

	if(aga_script_err("aga_setdraw", aga_setdraw(v))) return 0;

	return py_object_incref(PY_NONE);
}

struct py_object* agan_getflag(
		struct py_object* self, struct py_object* args) {

	(void) self;
	(void) args;

	return py_int_new(aga_getdraw());
}

struct py_object* agan_line3d(struct py_object* self, struct py_object* args) {
	aga_size_t i;

	struct py_object* from;
	struct py_object* to;
	struct py_object* pt;
	struct py_object* col;

	double fromf[3];
	double tof[3];
	double ptf;
	double colf[3];

	(void) self;

	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
		!aga_arg(&from, args, 0, PY_TYPE_LIST) ||
		py_varobject_size(from) != 3 ||
		!aga_arg(&to, args, 1, PY_TYPE_LIST) ||
		py_varobject_size(to) != 3 ||
		!aga_arg(&pt, args, 2, PY_TYPE_FLOAT) ||
		!aga_arg(&col, args, 3, PY_TYPE_LIST) ||
		py_varobject_size(col) != 3) {

		/* TODO: Add length checking like this elsewhere. */
		/*
		 * TODO: Make some of these optional -- let's make a cleaner/clearer
		 * 		 Optional arg IF.
		 */
		return aga_arg_error("line3d", "list[3], list[3], float and list[3]");
	}

	/*
	 * TODO: We really don't need to do these checked gets if the type was
	 * 		 Already verified (i.e. by `aga_arg').
	 */
	if(aga_script_float(pt, &ptf)) return 0;

	for(i = 0; i < 3; ++i) {
		struct py_object* v;

		if(aga_list_get(from, i, &v)) return 0;
		if(aga_script_float(v, &fromf[i])) return 0;

		if(aga_list_get(to, i, &v)) return 0;
		if(aga_script_float(v, &tof[i])) return 0;

		if(aga_list_get(col, i, &v)) return 0;
		if(aga_script_float(v, &colf[i])) return 0;
	}

	glLineWidth((float) ptf);
	if(aga_script_gl_err("glLineWidth")) return 0;

	glBegin(GL_LINES);
		glColor3dv(colf);
		glVertex3dv(fromf);
		glVertex3dv(tof);
	glEnd();
	if(aga_script_gl_err("glEnd")) return 0;

	return py_object_incref(PY_NONE);
}
