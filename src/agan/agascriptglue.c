/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

/*
 * TODO: Switch to unchecked List/Tuple/String accesses for release/noverify
 * 		 Builds? Disable parameter type strictness for noverify+dist?
 * 		 Enforce in main Python as global switch?
 */

/* TODO: Mode disable error-to-exception propagation - "continue". */

#include <agagl.h>
#include <agaresult.h>
#include <agalog.h>
#include <agaerr.h>
#include <agascript.h>
#include <agawin.h>
#include <agapack.h>
#include <agadraw.h>
#include <agastartup.h>
#include <agapyinc.h>
#include <agascripthelp.h>

#include <agan/aganobj.h>

aga_bool_t aga_script_gl_err(const char* proc) {
	return aga_script_err(proc, aga_gl_error(__FILE__, proc));
}

aga_bool_t agan_settransmat(struct py_object* trans, aga_bool_t inv) {
	struct py_object* comp;
	struct py_object* xo;
	struct py_object* yo;
	struct py_object* zo;
	float x, y, z;
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
				glTranslatef(x, y, z);
				if(aga_script_gl_err("glTranslatef")) return AGA_TRUE;
				break;
			}
			case 1: {
				glRotatef(x, 1.0f, 0.0f, 0.0f);
				if(aga_script_gl_err("glRotatef")) return AGA_TRUE;
				glRotatef(y, 0.0f, 1.0f, 0.0f);
				if(aga_script_gl_err("glRotatef")) return AGA_TRUE;
				glRotatef(z, 0.0f, 0.0f, 1.0f);
				if(aga_script_gl_err("glRotatef")) return AGA_TRUE;
				break;
			}
			case 2: {
				glScalef(x, y, z);
				if(aga_script_gl_err("glScalef")) return AGA_TRUE;
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

struct py_object* agan_getkey(struct py_object* self, struct py_object* arg) {
	enum aga_result result;
	int value;
	aga_bool_t b;
	struct aga_keymap* keymap;

	(void) self;

	if(!(keymap = aga_getscriptptr(AGA_SCRIPT_KEYMAP))) return 0;

	if(!aga_arg_list(arg, PY_TYPE_INT)) return aga_arg_error("getkey", "int");

	if(aga_script_int(arg, &value)) return 0;

	result = aga_keylook(keymap, (aga_uint8_t) value, &b);
 	if(aga_script_err("aga_keylook", result)) return 0;

	return py_object_incref(b ? PY_TRUE : PY_FALSE);
}

struct py_object*
agan_getmotion(struct py_object* self, struct py_object* arg) {
	struct py_object* retval;
	struct py_object* o;
	struct aga_pointer* pointer;

	(void) self;
	(void) arg;

	if(!(pointer = aga_getscriptptr(AGA_SCRIPT_POINTER))) return 0;

	if(!(retval = py_list_new(2))) return 0;

	if(!(o = py_float_new(pointer->dx))) return 0;
	if(aga_list_set(retval, 0, o)) return 0;

	if(!(o = py_float_new(pointer->dy))) return 0;
	if(aga_list_set(retval, 1, o)) return 0;

	return retval;
}

struct py_object* agan_setcam(struct py_object* self, struct py_object* arg) {
	struct py_object* t;
	struct py_object* mode;
	aga_bool_t b;
	double ar;

	struct aga_opts* opts;

	(void) self;

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

	return py_object_incref(PY_NONE);
}

struct py_object* agan_getconf(struct py_object* self, struct py_object* arg) {
	struct aga_opts* opts;

	(void) self;

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!aga_arg_list(arg, PY_TYPE_LIST)) {
		return aga_arg_error(
				"getconf", "list");
	}
	return agan_scriptconf(&opts->config, AGA_TRUE, arg);
}

struct py_object* agan_log(struct py_object* self, struct py_object* arg) {
	const char* str;
	const char* loc;

	(void) self;

	if(!arg) {
		py_error_set_string(py_runtime_error, "log() takes one argument");
		return 0;
	}

	if(aga_script_string(py_frame_current->code->filename, &loc)) return 0;

	if(aga_script_string(arg, &str)) return 0;

	aga_log(loc, str);

	return py_object_incref(PY_NONE);
}

/*
 * NOTE: Fog params follow the following format:
 * 		 [ density(norm), start, end ]
 */
struct py_object* agan_fogparam(struct py_object* self, struct py_object* arg) {
	struct py_object* v;
	float f;

	(void) self;

	if(!aga_arg_list(arg, PY_TYPE_LIST)) {
		return aga_arg_error(
				"fogparam", "list");
	}

	glFogi(GL_FOG_MODE, GL_EXP);
	if(aga_script_gl_err("glFogi")) return 0;

	if(aga_list_get(arg, 0, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_DENSITY, f);
	if(aga_script_gl_err("glFogf")) return 0;

	if(aga_list_get(arg, 1, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_START, f);
	if(aga_script_gl_err("glFogf")) return 0;

	if(aga_list_get(arg, 2, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_END, f);
	if(aga_script_gl_err("glFogf")) return 0;

	return py_object_incref(PY_NONE);
}

struct py_object* agan_fogcol(struct py_object* self, struct py_object* arg) {
	struct py_object* v;
	aga_size_t i;
	float col[3];

	(void) self;

	if(!aga_arg_list(arg, PY_TYPE_LIST)) {
		return aga_arg_error(
				"fogcol", "list");
	}

	for(i = 0; i < 3; ++i) {
		if(aga_list_get(arg, i, &v)) return 0;
		if(aga_script_float(v, &col[i])) return 0;
	}

	glFogfv(GL_FOG_COLOR, col);
	if(aga_script_gl_err("glFogfv")) return 0;

	return py_object_incref(PY_NONE);
}

struct py_object* agan_text(struct py_object* self, struct py_object* arg) {
	const char* text;
	struct py_object* str;
	struct py_object* t;
	struct py_object* f;
	float x, y;

	(void) self;

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

	if(aga_script_err("aga_puttext", aga_puttext(x, y, text))) {
		return 0;
	}

	return py_object_incref(PY_NONE);
}

struct py_object* agan_clear(struct py_object* self, struct py_object* arg) {
	enum aga_result result;

	struct py_object* v;
	float col[4];
	aga_size_t i;

	(void) self;

	if(!aga_arg_list(arg, PY_TYPE_LIST)) return aga_arg_error("clear", "list");

	for(i = 0; i < AGA_LEN(col); ++i) {
		if(aga_list_get(arg, i, &v)) return 0;
		if(aga_script_float(v, &col[i])) return 0;
	}

	result = aga_clear(col);
	if(aga_script_err("aga_clear", result)) return 0;

	return py_object_incref(PY_NONE);
}

/* Python lacks native bitwise ops @-@ */
struct py_object* agan_bitand(struct py_object* self, struct py_object* arg) {
	struct py_object* a;
	struct py_object* b;
	int av, bv;

	(void) self;

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&a, arg, 0, PY_TYPE_INT) ||
	   !aga_arg(&b, arg, 1, PY_TYPE_INT)) {
		return aga_arg_error("bitand", "int and int");
	}

	if(aga_script_int(a, &av)) return 0;
	if(aga_script_int(b, &bv)) return 0;

	return py_int_new(av & bv);
}

struct py_object* agan_bitshl(struct py_object* self, struct py_object* arg) {
	struct py_object* a;
	struct py_object* b;
	int av, bv;

	(void) self;

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&a, arg, 0, PY_TYPE_INT) ||
	   !aga_arg(&b, arg, 1, PY_TYPE_INT)) {
		return aga_arg_error("bitshl", "int and int");
	}

	if(aga_script_int(a, &av)) return 0;
	if(aga_script_int(b, &bv)) return 0;

	return py_int_new(av << bv);
}

struct py_object* agan_randnorm(struct py_object* self, struct py_object* arg) {
	(void) self;
	(void) arg;

	return py_float_new((double) rand() / (double) RAND_MAX);
}

struct py_object* agan_die(struct py_object* self, struct py_object* arg) {
	aga_bool_t* die;

	(void) self;
	(void) arg;

	if(!(die = aga_getscriptptr(AGA_SCRIPT_DIE))) return 0;
	*die = AGA_TRUE;

	return py_object_incref(PY_NONE);
}

struct py_object*
agan_setcursor(struct py_object* self, struct py_object* arg) {
	enum aga_result result;

	struct py_object* o;
	struct py_object* v;
	aga_bool_t visible, captured;

	struct aga_winenv* env;
	struct aga_win* win;

	(void) self;

	if(!(env = aga_getscriptptr(AGA_SCRIPT_WINENV))) return 0;
	if(!(win = aga_getscriptptr(AGA_SCRIPT_WIN))) return 0;

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&o, arg, 0, PY_TYPE_INT) ||
	   !aga_arg(&v, arg, 1, PY_TYPE_INT)) {
		return aga_arg_error("setcursor", "int and int");
	}

	if(aga_script_bool(o, &visible)) return 0;
	if(aga_script_bool(o, &captured)) return 0;

	result = aga_setcursor(env, win, visible, captured);
	if(aga_script_err("aga_setcursor", result)) return 0;

	return py_object_incref(PY_NONE);
}

static enum aga_result aga_insertfloat(const char* key, double value) {
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

static enum aga_result aga_insertint(const char* key, py_value_t value) {
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

static enum aga_result aga_setkeys(void);

enum aga_result aga_mkmod(void** dict) {
	const double pi = 3.14159265358979323846;
	const double e = 2.71828182845904523536;
	const double rads = pi / 180.0;

	enum aga_result result;

#define _(name) { #name, agan_##name }
	static const struct py_methodlist methods[] = {
			/* Input */
			_(getkey), _(getmotion), _(setcursor),

			/* Drawing */
			_(setcam), _(text), _(fogparam), _(fogcol), _(clear), _(mktrans),

			/* Miscellaneous */
			_(getconf), _(log), _(die),

			/* Objects */
			_(mkobj), _(inobj), _(putobj), _(killobj), _(objtrans), _(objconf),

			/* Maths */
			_(bitand), _(bitshl), _(randnorm),

			{ 0, 0 } };
#undef _

	struct py_object* module = py_module_new_methods("agan", methods);
	if(!module) return AGA_RESULT_ERROR;

	if(!(agan_dict = ((struct py_module*) module)->attr)) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	result = aga_insertfloat("PI", pi);
	if(result) return result;

	result = aga_insertfloat("RADS", rads);
	if(result) return result;

	result = aga_insertfloat("E", e);
	if(result) return result;

	if((result = aga_setkeys())) {
		aga_script_trace();
		return result;
	}

	*dict = agan_dict;

	return AGA_RESULT_OK;
}

static enum aga_result aga_setkeys(void) {
	enum aga_result result;
#define _(name, value) \
    do { \
        result = aga_insertint(name, value); \
        if(result) return result; \
    } while(0)
#ifdef _WIN32
/*
 * Values taken from:
 * https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
 */
	_("KEY_CANCEL", 0x03);
	_("KEY_BACKSPACE", 0x08);
	_("KEY_CLEAR", 0x0C);
	_("KEY_PAUSE", 0x13);
	_("KEY_ESCAPE", 0x1B);
	_("KEY_MENU", 0x12);
	_("KEY_TAB", 0x09);
	_("KEY_RETURN", 0x0D);
	_("KEY_CAPSLOCK", 0x14);
	_("KEY_SHIFT_L", 0xA0);
	_("KEY_SHIFT_R", 0xA1);
	_("KEY_CONTROL_L", 0xA2);
	_("KEY_CONTROL_R", 0xA3);
	_("KEY_ALT_L", 0xA4);
	_("KEY_ALT_R", 0xA5);
	_("KEY_KANALOCK", 0x15);
	_("KEY_KANASHIFT", 0x15);
	_("KEY_HANGUL", 0x15);
	_("KEY_HANJA", 0x19);
	_("KEY_KANJI", 0x19);
	_("KEY_SPACE", 0x20);
	_("KEY_PRIOR", 0x21);
	_("KEY_NEXT", 0x22);
	_("KEY_END", 0x23);
	_("KEY_HOME", 0x24);
	_("KEY_LEFT", 0x25);
	_("KEY_UP", 0x26);
	_("KEY_RIGHT", 0x27);
	_("KEY_DOWN", 0x28);
	_("KEY_SELECT", 0x29);
	_("KEY_PRINT", 0x2A);
	_("KEY_EXECUTE", 0x2B);
	_("KEY_INSERT", 0x2D);
	_("KEY_DELETE", 0x2E);
	_("KEY_HELP", 0x2F);
	_("KEY_0", 0x30);
	_("KEY_1", 0x31);
	_("KEY_2", 0x32);
	_("KEY_3", 0x33);
	_("KEY_4", 0x34);
	_("KEY_5", 0x35);
	_("KEY_6", 0x36);
	_("KEY_7", 0x37);
	_("KEY_8", 0x38);
	_("KEY_9", 0x39);
	_("KEY_A", 0x41);
	_("KEY_B", 0x42);
	_("KEY_C", 0x43);
	_("KEY_D", 0x44);
	_("KEY_E", 0x45);
	_("KEY_F", 0x46);
	_("KEY_G", 0x47);
	_("KEY_H", 0x48);
	_("KEY_I", 0x49);
	_("KEY_J", 0x4A);
	_("KEY_K", 0x4B);
	_("KEY_L", 0x4C);
	_("KEY_M", 0x4D);
	_("KEY_N", 0x4E);
	_("KEY_O", 0x4F);
	_("KEY_P", 0x50);
	_("KEY_Q", 0x51);
	_("KEY_R", 0x52);
	_("KEY_S", 0x53);
	_("KEY_T", 0x54);
	_("KEY_U", 0x55);
	_("KEY_V", 0x56);
	_("KEY_W", 0x57);
	_("KEY_X", 0x58);
	_("KEY_Y", 0x59);
	_("KEY_Z", 0x5A);
	_("KEY_META_L", 0x5B);
	_("KEY_META_R", 0x5C);
	_("KEY_SUPER_L", 0x5B);
	_("KEY_SUPER_R", 0x5C);
	_("KEY_NUMPAD0", 0x60);
	_("KEY_NUMPAD1", 0x61);
	_("KEY_NUMPAD2", 0x62);
	_("KEY_NUMPAD3", 0x63);
	_("KEY_NUMPAD4", 0x64);
	_("KEY_NUMPAD5", 0x65);
	_("KEY_NUMPAD6", 0x66);
	_("KEY_NUMPAD7", 0x67);
	_("KEY_NUMPAD8", 0x68);
	_("KEY_NUMPAD9", 0x69);
	_("KEY_KP_MULTIPLY", 0x6A);
	_("KEY_KP_ADD", 0x6B);
	_("KEY_KP_SEPARATOR", 0x6C);
	_("KEY_KP_SUBTRACT", 0x6D);
	_("KEY_KP_DECIMAL", 0x6E);
	_("KEY_KP_DIVIDE", 0x6F);
	_("KEY_F1", 0x70);
	_("KEY_F2", 0x71);
	_("KEY_F3", 0x72);
	_("KEY_F4", 0x73);
	_("KEY_F5", 0x74);
	_("KEY_F6", 0x75);
	_("KEY_F7", 0x76);
	_("KEY_F8", 0x77);
	_("KEY_F9", 0x78);
	_("KEY_F10", 0x79);
	_("KEY_F11", 0x7A);
	_("KEY_F12", 0x7B);
	_("KEY_F13", 0x7C);
	_("KEY_F14", 0x7D);
	_("KEY_F15", 0x7E);
	_("KEY_F16", 0x7F);
	_("KEY_F17", 0x80);
	_("KEY_F18", 0x81);
	_("KEY_F19", 0x82);
	_("KEY_F20", 0x83);
	_("KEY_F21", 0x84);
	_("KEY_F22", 0x85);
	_("KEY_F23", 0x86);
	_("KEY_F24", 0x87);
	_("KEY_NUMLOCK", 0x90);
	_("KEY_SCROLLLOCK", 0x91);
	_("KEY_ATTN", 0xF6);
	_("KEY_EXSELECT", 0xF7);
	_("KEY_CURSORSELECT", 0xF8);
	_("KEY_PLAY", 0xFA);
	_("KEY_ERASEEOF", 0xF9);
	_("KEY_PA1", 0xFD);
#else
	/*
	 * Values taken from `X11/keysymdef.h'
	 */
		_("KEY_CANCEL", 0xFF69);
		_("KEY_BACKSPACE", 0xFF08);
		_("KEY_CLEAR", 0xFF0B);
		_("KEY_PAUSE", 0xFF13);
		_("KEY_ESCAPE", 0xFF1B);
		_("KEY_MENU", 0xFF67);
		_("KEY_TAB", 0xFF09);
		_("KEY_RETURN", 0xFF0D);
		_("KEY_CAPSLOCK", 0xFFE5);
		_("KEY_SHIFT_L", 0xFFE1);
		_("KEY_SHIFT_R", 0xFFE2);
		_("KEY_CONTROL_L", 0xFFE3);
		_("KEY_CONTROL_R", 0xFFE4);
		_("KEY_ALT_L", 0xFFE9);
		_("KEY_ALT_R", 0xFFEA);
		_("KEY_KANALOCK", 0xFF2D);
		_("KEY_KANASHIFT", 0xFF2E);
		_("KEY_HANGUL", 0xFF31);
		_("KEY_HANJA", 0xFF34);
		_("KEY_KANJI", 0xFF21);
		_("KEY_SPACE", 0x0020);
		_("KEY_PRIOR", 0xFF55);
		_("KEY_NEXT", 0xFF56);
		_("KEY_END", 0xFF57);
		_("KEY_HOME", 0xFF50);
		_("KEY_LEFT", 0xFF51);
		_("KEY_UP", 0xFF52);
		_("KEY_RIGHT", 0xFF53);
		_("KEY_DOWN", 0xFF54);
		_("KEY_PAGE_UP", 0xFF55);
		_("KEY_PAGE_DOWN", 0xFF56);
		_("KEY_BEGIN", 0xFF58);
		_("KEY_SELECT", 0xFF60);
		_("KEY_PRINT", 0xFF61);
		_("KEY_EXECUTE", 0xFF62);
		_("KEY_INSERT", 0xFF63);
		_("KEY_DELETE", 0xFFFF);
		_("KEY_HELP", 0xFF6A);
		_("KEY_0", 0x0030);
		_("KEY_1", 0x0031);
		_("KEY_2", 0x0032);
		_("KEY_3", 0x0033);
		_("KEY_4", 0x0034);
		_("KEY_5", 0x0035);
		_("KEY_6", 0x0036);
		_("KEY_7", 0x0037);
		_("KEY_8", 0x0038);
		_("KEY_9", 0x0039);
		_("KEY_A", 0x0061);
		_("KEY_B", 0x0062);
		_("KEY_C", 0x0063);
		_("KEY_D", 0x0064);
		_("KEY_E", 0x0065);
		_("KEY_F", 0x0066);
		_("KEY_G", 0x0067);
		_("KEY_H", 0x0068);
		_("KEY_I", 0x0069);
		_("KEY_J", 0x006A);
		_("KEY_K", 0x006B);
		_("KEY_L", 0x006C);
		_("KEY_M", 0x006D);
		_("KEY_N", 0x006E);
		_("KEY_O", 0x006F);
		_("KEY_P", 0x0070);
		_("KEY_Q", 0x0071);
		_("KEY_R", 0x0072);
		_("KEY_S", 0x0073);
		_("KEY_T", 0x0074);
		_("KEY_U", 0x0075);
		_("KEY_V", 0x0076);
		_("KEY_W", 0x0077);
		_("KEY_X", 0x0078);
		_("KEY_Y", 0x0079);
		_("KEY_Z", 0x007A);
		_("KEY_META_L", 0xFFE7);
		_("KEY_META_R", 0xFFE8);
		_("KEY_SUPER_L", 0xFFEB);
		_("KEY_SUPER_R", 0xFFEC);
		_("KEY_KP_0", 0xFFB0);
		_("KEY_KP_1", 0xFFB1);
		_("KEY_KP_2", 0xFFB2);
		_("KEY_KP_3", 0xFFB3);
		_("KEY_KP_4", 0xFFB4);
		_("KEY_KP_5", 0xFFB5);
		_("KEY_KP_6", 0xFFB6);
		_("KEY_KP_7", 0xFFB7);
		_("KEY_KP_8", 0xFFB8);
		_("KEY_KP_9", 0xFFB9);
		_("KEY_KP_MULTIPLY", 0xFFAA);
		_("KEY_KP_ADD", 0xFFAB);
		_("KEY_KP_SEPARATOR", 0xFFAC);
		_("KEY_KP_SUBTRACT", 0xFFAD);
		_("KEY_KP_DECIMAL", 0xFFAE);
		_("KEY_KP_DIVIDE", 0xFFAF);
		_("KEY_F1", 0xFFBE);
		_("KEY_F2", 0xFFBF);
		_("KEY_F3", 0xFFC0);
		_("KEY_F4", 0xFFC1);
		_("KEY_F5", 0xFFC2);
		_("KEY_F6", 0xFFC3);
		_("KEY_F7", 0xFFC4);
		_("KEY_F8", 0xFFC5);
		_("KEY_F9", 0xFFC6);
		_("KEY_F10", 0xFFC7);
		_("KEY_F11", 0xFFC8);
		_("KEY_F12", 0xFFC9);
		_("KEY_F13", 0xFFCA);
		_("KEY_F14", 0xFFCB);
		_("KEY_F15", 0xFFCC);
		_("KEY_F16", 0xFFCD);
		_("KEY_F17", 0xFFCE);
		_("KEY_F18", 0xFFCF);
		_("KEY_F19", 0xFFD0);
		_("KEY_F20", 0xFFD1);
		_("KEY_F21", 0xFFD2);
		_("KEY_F22", 0xFFD3);
		_("KEY_F23", 0xFFD4);
		_("KEY_F24", 0xFFD5);
		_("KEY_NUMLOCK", 0xFF7F);
		_("KEY_SCROLLLOCK", 0xFF14);
		_("KEY_ATTN", 0xFD0E);
		_("KEY_EXSELECT", 0xFD1B);
		_("KEY_CURSORSELECT", 0xFD1C);
		_("KEY_PLAY", 0xFD16);
		_("KEY_ERASEEOF", 0xFD06);
		_("KEY_PA1", 0xFD0A);
#endif
#undef _

	return AGA_RESULT_OK;
}
