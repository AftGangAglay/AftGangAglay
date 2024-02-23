/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

/*
 * TODO: Switch to unchecked List/Tuple/String accesses for release/noverify
 * 		 Builds? Disable parameter type strictness for noverify+dist?
 */

/* TODO: Report failing `agan_' proc in trace (artificial frame?). */

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

aga_bool_t aga_script_glerr(const char* proc) {
	return aga_script_err(proc, aga_glerr(__FILE__, proc));
}

aga_bool_t agan_settransmat(aga_pyobject_t trans, aga_bool_t inv) {
	aga_pyobject_t comp, xo, yo, zo;
	float x, y, z;
	aga_size_t i;

	for(i = inv ? 2 : 0; i < 3; inv ? --i : ++i) {
		if(!(comp = dictlookup(trans, (char*) agan_trans_components[i]))) {
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
                if(aga_script_glerr("glTranslatef")) return AGA_TRUE;
                break;
            }
			case 1: {
				glRotatef(x, 1.0f, 0.0f, 0.0f);
                if(aga_script_glerr("glRotatef")) return AGA_TRUE;
				glRotatef(y, 0.0f, 1.0f, 0.0f);
                if(aga_script_glerr("glRotatef")) return AGA_TRUE;
				glRotatef(z, 0.0f, 0.0f, 1.0f);
                if(aga_script_glerr("glRotatef")) return AGA_TRUE;
				break;
			}
			case 2: {
				glScalef(x, y, z);
                if(aga_script_glerr("glScalef")) return AGA_TRUE;
				break;
			}
		}
	}

	return AGA_FALSE;
}

aga_pyobject_t agan_scriptconf(
        struct aga_conf_node* node, aga_bool_t root, aga_pyobject_t list) {

    const char* str;
    struct aga_conf_node* out;
    const char** names;
    aga_size_t i, len = getlistsize(list);
    aga_pyobject_t v;

    if(!(names = malloc(sizeof(char*) * len)))
        return err_nomem();

    for(i = 0; i < len; ++i) {
        if(aga_list_get(list, i, &v)) return 0;
        if(!(names[i] = getstringvalue(v))) {
            free(names);
            return 0;
        }
    }

    if(aga_script_err("aga_conftree_raw", aga_conftree_raw(
            root ? node->children : node, names, len, &out))) {

        free(names);
        return 0;
    }
    free(names);

    str = out->data.string ? out->data.string : "";
    switch(out->type) {
        default: {
            AGA_FALLTHROUGH;
            /* FALLTHRU */
        }
        case AGA_NONE: return AGA_INCREF(None);
        case AGA_STRING: return newstringobject((char*) str);
        case AGA_INTEGER: return newintobject(out->data.integer);
        case AGA_FLOAT: return newfloatobject(out->data.flt);
    }
}

AGAN_SCRIPTPROC(getkey) {
	int value;
	aga_pyobject_t retval = None;
	struct aga_keymap* keymap;

	if(!(keymap = aga_getscriptptr(AGA_SCRIPT_KEYMAP))) return 0;

	if(!AGA_ARGLIST(int)) AGA_ARGERR("getkey", "int");

	if(aga_script_int(arg, &value)) return 0;

	if(keymap->keystates) {
		if(value < keymap->keysyms_per_keycode * keymap->keycode_len) {
			retval = keymap->keystates[value] ? True : False;
		}
	}

	return AGA_INCREF(retval);
}

AGAN_SCRIPTPROC(getmotion) {
	aga_pyobject_t retval, o;
	struct aga_pointer* pointer;

	if(!(pointer = aga_getscriptptr(AGA_SCRIPT_POINTER))) return 0;

	if(!(retval = newlistobject(2))) return 0;

	if(!(o = newfloatobject(pointer->dx))) return 0;
	if(aga_list_set(retval, 0, o)) return 0;

	if(!(o = newfloatobject(pointer->dy))) return 0;
	if(aga_list_set(retval, 1, o)) return 0;

	return retval;
}

/* TODO: Inject true/false constants. */
AGAN_SCRIPTPROC(setcam) {
	aga_pyobject_t t, mode;
	aga_bool_t b;
	double ar;

	struct aga_opts* opts;

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(t, 0, dict) ||
		!AGA_ARG(mode, 1, int)) {

		AGA_ARGERR("setcam", "dict and int");
	}

	if(aga_script_bool(mode, &b)) return 0;

	ar = (double) opts->height / (double) opts->width;

	glMatrixMode(GL_PROJECTION);
    if(aga_script_glerr("glMatrixMode")) return 0;
        glLoadIdentity();
        if(aga_script_glerr("glLoadIdentity")) return 0;

	if(b) {
		gluPerspective(opts->fov, 1.0 / ar, 0.1, 10000.0);
		if(aga_script_glerr("gluPerspective")) return 0;
	}
	else {
		glOrtho(-1.0, 1.0, -ar, ar, 0.001, 1.0);
		if(aga_script_glerr("glOrtho")) return 0;
	}

	glMatrixMode(GL_MODELVIEW);
    if(aga_script_glerr("glMatrixMode")) return 0;
        glLoadIdentity();
        if(aga_script_glerr("glLoadIdentity")) return 0;
        if(agan_settransmat(t, AGA_TRUE)) return 0;

	return AGA_INCREF(None);
}

AGAN_SCRIPTPROC(getconf) {
	struct aga_opts* opts;

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!AGA_ARGLIST(list)) AGA_ARGERR("getconf", "list");
	return agan_scriptconf(&opts->config, AGA_TRUE, arg);
}

AGAN_SCRIPTPROC(log) {
	char* str;
	char* loc;

	if(!arg) {
		err_setstr(RuntimeError, "log() takes one argument");
		return 0;
	}

	if(aga_script_string(current_frame->f_code->co_filename, &loc)) return 0;

	if(!is_stringobject(arg)) {
		aga_size_t i;
		for(i = 0; i < aga_logctx.len; ++i) {
			FILE* s = aga_logctx.targets[i];
			aga_loghdr(s, loc, AGA_NORM);
			printobject(arg, s, 0);
			if(putc('\n', s) == EOF) perror("putc");
		}

		return AGA_INCREF(None);
	}

	if(aga_script_string(arg, &str)) return 0;

	aga_log(loc, str);

	return AGA_INCREF(None);
}

/*
 * NOTE: Fog params follow the following format:
 * 		 [ density(norm), start, end ]
 */
AGAN_SCRIPTPROC(fogparam) {
	aga_pyobject_t v;
	float f;

	if(!AGA_ARGLIST(list)) AGA_ARGERR("fogparam", "list");

	glFogi(GL_FOG_MODE, GL_EXP);
	if(aga_script_glerr("glFogi")) return 0;

	if(aga_list_get(arg, 0, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_DENSITY, f);
	if(aga_script_glerr("glFogf")) return 0;

	if(aga_list_get(arg, 1, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_START, f);
	if(aga_script_glerr("glFogf")) return 0;

	if(aga_list_get(arg, 2, &v)) return 0;
	if(aga_script_float(v, &f)) return 0;

	glFogf(GL_FOG_END, f);
	if(aga_script_glerr("glFogf")) return 0;

	return AGA_INCREF(None);
}

AGAN_SCRIPTPROC(fogcol) {
	aga_pyobject_t v;
	aga_size_t i;
	float col[3];

	if(!AGA_ARGLIST(list)) AGA_ARGERR("fogcol", "list");

	for(i = 0; i < 3; ++i) {
		if(aga_list_get(arg, i, &v)) return 0;
		if(aga_script_float(v, &col[i])) return 0;
	}

	glFogfv(GL_FOG_COLOR, col);
	if(aga_script_glerr("glFogfv")) return 0;

	return AGA_INCREF(None);
}

AGAN_SCRIPTPROC(text) {
	char* text;
	aga_pyobject_t str, t, f;
	float x, y;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(str, 0, string) ||
		!AGA_ARG(t, 1, list)) {

		AGA_ARGERR("text", "string and list");
	}

	if(aga_script_string(str, &text)) return 0;

	/* TODO: Use general list get N items API here. */
	if(aga_list_get(t, 0, &f)) return 0;
	if(aga_script_float(f, &x)) return 0;

	if(aga_list_get(t, 1, &f)) return 0;
	if(aga_script_float(f, &y)) return 0;

	if(aga_script_err("aga_puttext", aga_puttext(x, y, text)))
		return 0;

	return AGA_INCREF(None);
}

AGAN_SCRIPTPROC(clear) {
	enum aga_result result;

	aga_pyobject_t v;
	float col[4];
	aga_size_t i;

	if(!AGA_ARGLIST(list)) AGA_ARGERR("clear", "list");

	for(i = 0; i < AGA_LEN(col); ++i) {
		if(aga_list_get(arg, i, &v)) return 0;
		if(aga_script_float(v, &col[i])) return 0;
	}

	result = aga_clear(col);
	if(aga_script_err("aga_clear", result)) return 0;

	return AGA_INCREF(None);
}

/* Python lacks native bitwise ops @-@ */
AGAN_SCRIPTPROC(bitand) {
	aga_pyobject_t a, b;
	int av, bv;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(a, 0, int) || !AGA_ARG(b, 1, int)) {
		AGA_ARGERR("bitand", "int and int");
	}

	if(aga_script_int(a, &av)) return 0;
	if(aga_script_int(b, &bv)) return 0;

	return newintobject(av & bv);
}

AGAN_SCRIPTPROC(bitshl) {
	aga_pyobject_t a, b;
	int av, bv;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(a, 0, int) || !AGA_ARG(b, 1, int)) {
		AGA_ARGERR("bitshl", "int and int");
	}

	if(aga_script_int(a, &av)) return 0;
	if(aga_script_int(b, &bv)) return 0;

	return newintobject(av << bv);
}

AGAN_SCRIPTPROC(randnorm) {
	return newfloatobject((double) rand() / (double) RAND_MAX);
}

AGAN_SCRIPTPROC(die) {
	aga_bool_t* die;
	if(!(die = aga_getscriptptr(AGA_SCRIPT_DIE))) return 0;
	*die = AGA_TRUE;

	return AGA_INCREF(None);
}

AGAN_SCRIPTPROC(setcursor) {
	enum aga_result result;

	aga_pyobject_t o, v;
	aga_bool_t visible, captured;

	struct aga_winenv* env;
	struct aga_win* win;

	if(!(env = aga_getscriptptr(AGA_SCRIPT_WINENV))) return 0;
	if(!(win = aga_getscriptptr(AGA_SCRIPT_WIN))) return 0;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(o, 0, int) || !AGA_ARG(v, 1, int)) {
		AGA_ARGERR("setcursor", "int and int");
	}

	if(aga_script_bool(o, &visible)) return 0;
	if(aga_script_bool(o, &captured)) return 0;

	result = aga_setcursor(env, win, visible, captured);
	if(aga_script_err("aga_setcursor", result)) return 0;

	return AGA_INCREF(None);
}

static enum aga_result aga_insertfloat(const char* key, double value) {
	aga_pyobject_t o;

	if(!(o = newfloatobject(value))) {
        aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	if(dictinsert(agan_dict, (char*) key, o) == -1) {
        aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

static enum aga_result aga_insertint(const char* key, long value) {
	aga_pyobject_t o;

	if(!(o = newintobject(value))) {
        aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	if(dictinsert(agan_dict, (char*) key, o) == -1) {
        aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}

static enum aga_result aga_setkeys(void);

enum aga_result aga_mkmod(void** dict) {
	static const double pi = 3.14159265358979323846;
	static const double rads = pi / 180.0;

	enum aga_result result;

#define _(name) { #name, agan_##name }
	struct methodlist methods[] = {
		/* Input */
		_(getkey),
		_(getmotion),
		_(setcursor),

		/* Drawing */
		_(setcam),
		_(text),
		_(fogparam),
		_(fogcol),
        _(clear),
        _(mktrans),

		/* Miscellaneous */
		_(getconf),
		_(log),
		_(die),

		/* Objects */
        _(mkobj),
        _(inobj),
		_(putobj),
		_(killobj),
		_(objtrans),
		_(objconf),

		/* Maths */
		_(bitand),
		_(bitshl),
		_(randnorm),

		{ 0, 0 }
	};
#undef _

	aga_pyobject_t module = initmodule("agan", methods);
	AGA_VERIFY(module, AGA_RESULT_ERROR);

	if(!(agan_dict = getmoduledict(module))) {
        aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	AGA_CHK(aga_insertfloat("PI", pi));
	AGA_CHK(aga_insertfloat("RADS", rads));

	if((result = aga_setkeys())) {
        aga_script_trace();
		return result;
	}

	*dict = agan_dict;

	return AGA_RESULT_OK;
}

static enum aga_result aga_setkeys(void) {
#define _(name, value) AGA_CHK(aga_insertint(name, value))
#ifdef AGA_GLX
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
#elif defined(AGA_WGL)
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
#endif
#undef _

	return AGA_RESULT_OK;
}
