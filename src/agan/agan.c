/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/agan.h>
#include <agan/aganobj.h>
#include <agan/aganio.h>
#include <agan/agandraw.h>
#include <agan/aganmisc.h>
#include <agan/aganmath.h>

#include <agaerr.h>
#include <agascripthelp.h>
#include <agadraw.h>
#include <agaconf.h>
#include <agagl.h>

/*
 * TODO: Switch to unchecked List/Tuple/String accesses for release/noverify
 * 		 Builds? Disable parameter type strictness for noverify+dist?
 * 		 Enforce in main Python as global switch?
 */

/* TODO: Mode disable error-to-exception propagation - "continue". */

/*
 * NOTE: We need a bit of global state here to get engine system contexts etc.
 * 		 Into script land because this version of Python's state is spread
 * 		 Across every continent.
 */
struct py_object* agan_dict = 0;

const char* agan_trans_components[3] = { "pos", "rot", "scale" };
const char* agan_conf_components[3] = { "Position", "Rotation", "Scale" };
const char* agan_xyz[3] = { "X", "Y", "Z" };
const char* agan_rgb[3] = { "R", "G", "B" };

enum aga_result aga_insertstr(const char* key, const char* value) {
	struct py_object* o;

	if(!(o = py_string_new(value))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	if(py_dict_insert(agan_dict, key, o) == -1) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_insertfloat(const char* key, double value) {
	struct py_object* o;

	if(!(o = py_float_new(value))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	if(py_dict_insert(agan_dict, key, o) == -1) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_insertint(const char* key, py_value_t value) {
	struct py_object* o;

	if(!(o = py_int_new(value))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	if(py_dict_insert(agan_dict, key, o) == -1) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_mkmod(void** dict) {
	enum aga_result result;

	/* TODO: Move method registration out to modules aswell. */
#define _(name) { #name, agan_##name }
	static const struct py_methodlist methods[] = {
			/* Input */
			_(getkey), _(getmotion), _(setcursor), _(getbuttons), _(getpos),

			/* Drawing */
			_(setcam), _(text), _(fogparam), _(fogcol), _(clear), _(mktrans),
			_(shadeflat), _(getpix), _(setflag), _(getflag),

			/* Miscellaneous */
			_(getconf), _(log), _(die),

			/* Objects */
			_(mkobj), _(inobj), _(putobj), _(killobj), _(objtrans), _(objconf),
			_(objind),

			/* Maths */
			_(bitand), _(bitshl), _(randnorm), _(bitor),

			{ 0, 0 } };
#undef _

	struct py_object* module = py_module_new_methods("agan", methods);
	if(!module) return AGA_RESULT_ERROR;

	if(!(agan_dict = ((struct py_module*) module)->attr)) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	if((result = agan_draw_register())) return result;
	if((result = agan_io_register())) return result;
	if((result = agan_math_register())) return result;
	if((result = agan_misc_register())) return result;

	*dict = agan_dict;

	return AGA_RESULT_OK;
}

aga_bool_t aga_script_err(const char* proc, enum aga_result err) {
	aga_fixed_buf_t buf = { 0 };

	if(!err) return AGA_FALSE;

	if(sprintf(buf, "%s: %s", proc, aga_result_name(err)) < 0) {
		aga_errno(__FILE__, "sprintf");
		return AGA_TRUE;
	}
	py_error_set_string(py_runtime_error, buf);

	return AGA_TRUE;
}

aga_bool_t aga_script_gl_err(const char* proc) {
	return aga_script_err(proc, aga_gl_error(__FILE__, proc));
}

/* TODO: Generalised lookup helper since we no longer do nativeptr? */
void* aga_getscriptptr(const char* key) {
	struct py_object* ptr;

	if(!key) {
		py_error_set_string(py_runtime_error, "unexpected null pointer");
		return 0;
	}

	ptr = py_dict_lookup(agan_dict, key);
	if(!ptr) {
		py_error_set_string(
				py_runtime_error, "failed to resolve script pointer");
		return 0;
	}

	if(ptr->type != PY_TYPE_INT) {
		py_error_set_badarg();
		return 0;
	}

	return aga_script_getptr(ptr);
}

aga_bool_t agan_settransmat(struct py_object* trans, aga_bool_t inv) {
	struct py_object* comp;
	struct py_object* xo;
	struct py_object* yo;
	struct py_object* zo;
	double x, y, z;
	aga_size_t i;

	for(i = inv ? 2 : 0; i < 3; inv ? --i : ++i) {
		if(!(comp = py_dict_lookup(trans, agan_trans_components[i]))) {
			return AGA_TRUE;
		}

		if(aga_list_get(comp, 0, &xo)) return 0;
		if(aga_list_get(comp, 1, &yo)) return 0;
		if(aga_list_get(comp, 2, &zo)) return 0;

		if(aga_script_float(xo, &x)) return 0;
		if(aga_script_float(yo, &y)) return 0;
		if(aga_script_float(zo, &z)) return 0;

		switch(i) {
			default: break;
			case 0: {
				glTranslated(x, y, z);
				if(aga_script_gl_err("glTranslated")) return AGA_TRUE;
				break;
			}
			case 1: {
				glRotated(x, 1.0, 0.0, 0.0);
				if(aga_script_gl_err("glRotated")) return AGA_TRUE;
				glRotated(y, 0.0, 1.0, 0.0);
				if(aga_script_gl_err("glRotated")) return AGA_TRUE;
				glRotated(z, 0.0, 0.0, 1.0);
				if(aga_script_gl_err("glRotated")) return AGA_TRUE;
				break;
			}
			case 2: {
				glScaled(x, y, z);
				if(aga_script_gl_err("glScaled")) return AGA_TRUE;
				break;
			}
		}
	}

	return AGA_FALSE;
}

struct py_object* agan_scriptconf(
		struct aga_conf_node* node, aga_bool_t root, struct py_object* list) {

	enum aga_result result;

	const char* str;
	struct aga_conf_node* out;
	const char** names;
	aga_size_t i, len = py_varobject_size(list);
	struct py_object* v;

	if(!(names = malloc(len * sizeof(char*)))) return py_error_set_nomem();

	for(i = 0; i < len; ++i) {
		if(aga_list_get(list, i, &v)) return 0;
		if(!(names[i] = py_string_get(v))) {
			free(names);
			return 0;
		}
	}

	result = aga_conftree_raw(root ? node->children : node, names, len, &out);
	free(names);
	if(aga_script_err("aga_conftree_raw", result)) return 0;

	str = out->data.string ? out->data.string : "";
	switch(out->type) {
		default: {
			AGA_FALLTHROUGH;
			/* FALLTHRU */
		}
		case AGA_NONE: return py_object_incref(PY_NONE);
		case AGA_STRING: return py_string_new(str);
		case AGA_INTEGER: return py_int_new(out->data.integer);
		case AGA_FLOAT: return py_float_new(out->data.flt);
	}
}
