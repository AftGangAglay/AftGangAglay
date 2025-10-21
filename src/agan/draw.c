/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/draw.h>

#include <aga/script.h>
#include <aga/startup.h>
#include <aga/draw.h>
#include <aga/diagnostic.h>
#include <aga/render.h>

#include <asys/log.h>

#include <mil/system.h>
#include <mil/widget.h>

#include <apro.h>

/*
 * TODO: Implement GL-native pick buffers so we can get vertex colouration
 * 		 Back.
 */

enum asys_result agan_draw_register(struct py_env* env) {
	enum asys_result result;

	(void) env;

	if((result = aga_insert_int("BACKFACE", AGA_DRAW_BACKFACE))) return result;
	if((result = aga_insert_int("BLEND", AGA_DRAW_BLEND))) return result;
	if((result = aga_insert_int("FOG", AGA_DRAW_FOG))) return result;
	if((result = aga_insert_int("TEXTURE", AGA_DRAW_TEXTURE))) return result;
	if((result = aga_insert_int("LIGHTING", AGA_DRAW_LIGHTING))) return result;
	if((result = aga_insert_int("DEPTH", AGA_DRAW_DEPTH))) return result;
	if((result = aga_insert_int("SHADEFLAT", AGA_DRAW_FLAT))) return result;
	if((result = aga_insert_int("FIDELITY", AGA_DRAW_FIDELITY))) return result;

	if((result = aga_insert_int("FRONT", AGAN_SURFACE_FRONT))) return result;
	if((result = aga_insert_int("BACK", AGAN_SURFACE_BACK))) return result;
	if((result = aga_insert_int("STENCIL", AGAN_SURFACE_STENCIL))) {
		return result;
	}
	if((result = aga_insert_int("DEPTHBUF", AGAN_SURFACE_DEPTH))) {
		return result;
	}

	return ASYS_RESULT_OK;
}

/* TODO: Add FOV as optional extra param. */
struct py_object* agan_setcam(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* t;
	struct py_object* mode;
	asys_bool_t b;
	double ar;
	unsigned width, height;

	struct aga_settings* opts = AGA_GET_USERDATA(env)->opts;
	struct mil_ctx* mil = AGA_GET_USERDATA(env)->mil;
	mil_widget_t gl_area = AGA_GET_USERDATA(env)->gl_area;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_SETCAM);

	/* setcam(dict, int) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
		!aga_arg(&t, args, 0, PY_TYPE_DICT) ||
		!aga_arg(&mode, args, 1, PY_TYPE_INT)) {

		return aga_arg_error("setcam", "dict and int");
	}

	b = !!py_int_get(mode);

	mil_widget_get_size(mil, gl_area, &width, &height);

	ar = (double) height / (double) width;

	glMatrixMode(GL_PROJECTION);
	if(aga_script_gl_err(__FILE__, "glMatrixMode")) return 0;
	glLoadIdentity();
	if(aga_script_gl_err(__FILE__, "glLoadIdentity")) return 0;

	if(b) {
		/* TODO: Parameterized FOV. */
		gluPerspective(opts->fov, 1.0 / ar, 0.1, 10000.0);
		if(aga_script_gl_err(__FILE__, "gluPerspective")) return 0;
	}
	else {
		glOrtho(-1.0, 1.0, -ar, ar, 0.001, 1.0);
		if(aga_script_gl_err(__FILE__, "glOrtho")) return 0;
	}

	glMatrixMode(GL_MODELVIEW);
	if(aga_script_gl_err(__FILE__, "glMatrixMode")) return 0;
	glLoadIdentity();
	if(aga_script_gl_err(__FILE__, "glLoadIdentity")) return 0;
	if(agan_settransmat(t, ASYS_TRUE)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_SETCAM);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_text(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	static const float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	const char* text;
	struct py_object* str;
	struct py_object* t;
	float x, y;

	AGA_DEPRECATED("agan.text", "agan.text2d");

	/* TODO: Implement engine-native translations `agan.text('msg.hello')'. */

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_TEXT);

	/* text(string, float[2]) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
		!aga_arg(&str, args, 0, PY_TYPE_STRING) ||
		!aga_vararg_typed(&t, args, 1, PY_TYPE_LIST, 2, PY_TYPE_FLOAT)) {

		return aga_arg_error("text", "string and float[2]");
	}

	text = py_string_get(str);

	/* TODO: Use general list get N items API here. */
	x = (float) py_float_get(py_list_get(t, 0));
	y = (float) py_float_get(py_list_get(t, 1));

	/* TODO: Color. */
	if(aga_script_err(
			__FILE__, "aga_render_text", aga_render_text(x, y, color, text))) {

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
		struct py_env* env, struct py_object* self, struct py_object* args) {

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_FOGPARAM);

	/* fogparam(float[3]) */
	if(!aga_vararg_list_typed(args, PY_TYPE_LIST, 3, PY_TYPE_FLOAT)) {
		return aga_arg_error("fogparam", "float[3]");
	}

	glFogi(GL_FOG_MODE, GL_EXP);
	if(aga_script_gl_err(__FILE__, "glFogi")) return 0;

	glFogf(GL_FOG_DENSITY, (float) py_float_get(py_list_get(args, 0)));
	if(aga_script_gl_err(__FILE__, "glFogf")) return 0;

	glFogf(GL_FOG_START, (float) py_float_get(py_list_get(args, 1)));
	if(aga_script_gl_err(__FILE__, "glFogf")) return 0;

	glFogf(GL_FOG_END, (float) py_float_get(py_list_get(args, 2)));
	if(aga_script_gl_err(__FILE__, "glFogf")) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_FOGPARAM);

	return py_object_incref(PY_NONE);
}

/*
 * TODO: We should ideally have a consolidated way to get vector-y types
 * 		 Out of scriptland.
 */
struct py_object* agan_fogcol(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	unsigned i;
	float col[3];

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_FOGCOL);

	/* fogcol(float[3]) */
	if(!aga_vararg_list_typed(args, PY_TYPE_LIST, 3, PY_TYPE_FLOAT)) {
		return aga_arg_error("fogcol", "list");
	}

	for(i = 0; i < ASYS_LENGTH(col); ++i) {
		col[i] = (float) py_float_get(py_list_get(args, i));
	}

	glFogfv(GL_FOG_COLOR, col);
	if(aga_script_gl_err(__FILE__, "glFogfv")) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_FOGCOL);

	return py_object_incref(PY_NONE);
}

/* TODO: Take surface ID/mask like `getpix()'. */
struct py_object* agan_clear(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	unsigned i;
	float color[4];

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_CLEAR);

	/* fogcol(float[4]) */
	if(!aga_vararg_list_typed(args, PY_TYPE_LIST, 4, PY_TYPE_FLOAT)) {
		return aga_arg_error("clear", "float[4]");
	}

	for(i = 0; i < ASYS_LENGTH(color); ++i) {
		color[i] = (float) py_float_get(py_list_get(args, i));
	}

	if(aga_script_err(__FILE__, "aga_render_clear", aga_render_clear(color))) {
		return 0;
	}

	apro_stamp_end(APRO_SCRIPTGLUE_CLEAR);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_mktrans(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* retval;
	struct py_object* list;
	unsigned i, j;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_MKTRANS);

	if(args) return aga_arg_error("mktrans", "none");

	if(!(retval = py_dict_new())) return py_error_set_nomem();

	for(i = 0; i < 3; ++i) {
		if(!(list = py_list_new(3))) return py_error_set_nomem();

		for(j = 0; j < 3; ++j) {
			py_list_set(list, j, py_float_new(i == 2 ? 1.0 : 0.0));
		}

		if(py_dict_insert(retval, agan_trans_components[i], list) == -1) {
			py_error_set_key();
			return 0;
		}
	}

	apro_stamp_end(APRO_SCRIPTGLUE_MKTRANS);

	return retval;
}

struct py_object* agan_shadeflat(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	AGA_DEPRECATED("agan.shadeflat", "agan.setflag");

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_SHADEFLAT);

	/* shadeflat(int) */
	if(!aga_arg_list(args, PY_TYPE_INT)) {
		return aga_arg_error("shadeflat", "int");
	}

	glShadeModel(py_int_get(args) ? GL_FLAT : GL_SMOOTH);
	if(aga_script_gl_err(__FILE__, "glShadeModel")) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_SHADEFLAT);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_getpix(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	static asys_uint_t surface_names[] = {
			GL_FRONT, GL_BACK, GL_STENCIL, GL_DEPTH };

	asys_uchar_t pix[3];
	py_value_t x, y;
	unsigned i, width, height;
	int h;
	struct py_object* retval;
	struct py_object* list;
	/* TODO: Gracefully handle single vs. double buffered envs. */
	enum agan_surface surface = AGAN_SURFACE_BACK;

	struct mil_ctx* mil = AGA_GET_USERDATA(env)->mil;
	mil_widget_t gl_area = AGA_GET_USERDATA(env)->gl_area;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_GETPIX);

	mil_widget_get_size(mil, gl_area, &width, &height);

	list = args;

	/* getpix(int[2][, int]) */
	if(!aga_vararg_list_typed(args, PY_TYPE_LIST, 2, PY_TYPE_INT)) {
		struct py_object* target;

		/* Handle alternate form. */
		if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
			!aga_vararg_typed(&list, args, 0, PY_TYPE_LIST, 2, PY_TYPE_INT) ||
			!aga_arg(&target, args, 1, PY_TYPE_INT)) {

			return aga_arg_error("getpix", "int[2] [and int]");
		}
		else {
			py_value_t v = py_int_get(target);

			if(v < AGAN_SURFACE_FRONT || v > AGAN_SURFACE_DEPTH) {
				asys_log(__FILE__, "err: Surface name out of range `%u'", v);
				py_error_set_badarg();
				return 0;
			}

			surface = v;
		}
	}

	x = py_int_get(py_list_get(list, 0));
	y = py_int_get(py_list_get(list, 1));

	glReadBuffer(surface_names[surface]);
	if(aga_script_gl_err(__FILE__, "glReadBuffer")) return 0;

	h = (int) (height - y);
	glReadPixels((int) x, h, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pix);
	if(aga_script_gl_err(__FILE__, "glReadPixels")) return 0;

	if(!(retval = py_list_new(ASYS_LENGTH(pix)))) return py_error_set_nomem();

	for(i = 0; i < ASYS_LENGTH(pix); ++i) {
		py_list_set(retval, i, py_int_new(pix[i]));
	}

	apro_stamp_end(APRO_SCRIPTGLUE_GETPIX);

	return retval;
}

/* TODO: Could just take a list here. */
struct py_object* agan_setflag(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	(void) env;
	(void) self;

	/* setflag(int) */
	if(!aga_arg_list(args, PY_TYPE_INT)) {
		return aga_arg_error("setflag", "int");
	}

	if(aga_script_err(
			__FILE__, "aga_draw_set", aga_draw_set(py_int_get(args)))) {

		return 0;
	}

	return py_object_incref(PY_NONE);
}

struct py_object* agan_getflag(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	(void) env;
	(void) self;

	if(args) return aga_arg_error("getflag", "none");

	return py_int_new(aga_draw_get());
}

/* TODO: Line stippling. */
struct py_object* agan_line3d(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	unsigned i;

	struct py_object* from;
	struct py_object* to;
	struct py_object* pt;
	struct py_object* col;

	double fromf[3];
	double tof[3];
	double colf[3];

	(void) env;
	(void) self;

	/* line3d(float[3], float[3], float, float[3]) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 4) ||
		!aga_vararg_typed(&from, args, 0, PY_TYPE_LIST, 3, PY_TYPE_FLOAT) ||
		!aga_vararg_typed(&to, args, 1, PY_TYPE_LIST, 3, PY_TYPE_FLOAT) ||
		!aga_arg(&pt, args, 2, PY_TYPE_FLOAT) ||
		!aga_vararg_typed(&col, args, 3, PY_TYPE_LIST, 3, PY_TYPE_FLOAT)) {

		/*
		 * TODO: Make some of these optional -- let's make a cleaner/clearer
		 * 		 Optional arg IF.
		 */
		return aga_arg_error(
				"line3d", "float[3], float[3], float and float[3]");
	}

	for(i = 0; i < ASYS_LENGTH(fromf); ++i) {
		fromf[i] = py_float_get(py_list_get(from, i));
		tof[i] = py_float_get(py_list_get(to, i));

		colf[i] = py_float_get(py_list_get(col, i));
	}

	glLineWidth((float) py_float_get(pt));
	if(aga_script_gl_err(__FILE__, "glLineWidth")) return 0;

	glBegin(GL_LINES);
		glColor3dv(colf);
		glVertex3dv(fromf);
		glVertex3dv(tof);
	glEnd();
	if(aga_script_gl_err(__FILE__, "glEnd")) return 0;

	return py_object_incref(PY_NONE);
}
