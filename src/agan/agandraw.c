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

struct py_object* agan_setcam(struct py_object* self, struct py_object* arg) {
	struct py_object* t;
	struct py_object* mode;
	aga_bool_t b;
	double ar;

	struct aga_opts* opts;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_SETCAM);

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&t, arg, 0, PY_TYPE_DICT) ||
	   !aga_arg(&mode, arg, 1, PY_TYPE_INT)) {

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

struct py_object* agan_text(struct py_object* self, struct py_object* arg) {
	const char* text;
	struct py_object* str;
	struct py_object* t;
	struct py_object* f;
	double x, y;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_TEXT);

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&str, arg, 0, PY_TYPE_STRING) ||
	   !aga_arg(&t, arg, 1, PY_TYPE_LIST)) {

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
		struct py_object* self, struct py_object* arg) {

	struct py_object* v;
	double f;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_FOGPARAM);

	if(!aga_arg_list(arg, PY_TYPE_LIST)) {
		return aga_arg_error("fogparam", "list");
	}

	glFogi(GL_FOG_MODE, GL_EXP);
	if(aga_script_gl_err("glFogi")) return 0;

	if(aga_list_get(arg, 0, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_DENSITY, (float) f);
	if(aga_script_gl_err("glFogf")) return 0;

	if(aga_list_get(arg, 1, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_START, (float) f);
	if(aga_script_gl_err("glFogf")) return 0;

	if(aga_list_get(arg, 2, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_END, (float) f);
	if(aga_script_gl_err("glFogf")) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_FOGPARAM);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_fogcol(struct py_object* self, struct py_object* arg) {
	struct py_object* v;
	aga_size_t i;
	double f;
	float col[3];

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_FOGCOL);

	if(!aga_arg_list(arg, PY_TYPE_LIST)) {
		return aga_arg_error("fogcol", "list");
	}

	for(i = 0; i < 3; ++i) {
		if(aga_list_get(arg, i, &v)) return 0;
		if(aga_script_float(v, &f)) return 0;
		col[i] = (float) f;
	}

	glFogfv(GL_FOG_COLOR, col);
	if(aga_script_gl_err("glFogfv")) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_FOGCOL);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_clear(struct py_object* self, struct py_object* arg) {
	enum aga_result result;

	struct py_object* v;
	double f;
	float col[4];
	aga_size_t i;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_CLEAR);

	if(!aga_arg_list(arg, PY_TYPE_LIST)) return aga_arg_error("clear", "list");

	for(i = 0; i < AGA_LEN(col); ++i) {
		if(aga_list_get(arg, i, &v)) return 0;
		if(aga_script_float(v, &f)) return 0;
		col[i] = (float) f;
	}

	result = aga_clear(col);
	if(aga_script_err("aga_clear", result)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_CLEAR);

	return py_object_incref(PY_NONE);
}

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

struct py_object* agan_shadeflat(
		struct py_object* self, struct py_object* arg) {

	py_value_t v;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_SHADEFLAT);

	if(!aga_arg_list(arg, PY_TYPE_INT)) {
		return aga_arg_error("shadeflat", "int");
	}

	if(aga_script_int(arg, &v)) return 0;

	glShadeModel(v ? GL_FLAT : GL_SMOOTH);
	if(aga_script_gl_err("glShadeModel")) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_SHADEFLAT);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_getpix(struct py_object* self, struct py_object* arg) {
	aga_uint8_t pix[3];
	py_value_t x, y;
	unsigned i;
	int h;
	struct py_object* xo;
	struct py_object* yo;
	struct py_object* retval;

	struct aga_win* win;

	(void) self;

	if(!(win = aga_getscriptptr(AGA_SCRIPT_WIN))) return 0;

	if(!aga_arg_list(arg, PY_TYPE_LIST)) {
		return aga_arg_error("getpix", "list");
	}

	if(aga_list_get(arg, 0, &xo)) return 0;
	if(aga_list_get(arg, 1, &yo)) return 0;

	if(aga_script_int(xo, &x)) return 0;
	if(aga_script_int(yo, &y)) return 0;

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

	return retval;
}
