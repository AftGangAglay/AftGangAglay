/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/io.h>

#include <aga/window.h>
#include <aga/script.h>

#include <apro.h>

static enum aga_result aga_setkeys(void);

enum aga_result agan_io_register(struct py_env* env) {
	enum aga_result result;

	(void) env;

	if((result = aga_insertint("CLICK", AGA_BUTTON_CLICK))) return result;
	if((result = aga_insertint("DOWN", AGA_BUTTON_DOWN))) return result;
	if((result = aga_insertint("UP", AGA_BUTTON_UP))) return result;

	if((result = aga_setkeys())) {
		aga_script_engine_trace();
		return result;
	}

	return AGA_RESULT_OK;
}

struct py_object* agan_getkey(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum aga_result result;
	aga_bool_t b;
	struct aga_keymap* keymap;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_GETKEY);

	if(!(keymap = aga_getscriptptr(AGA_SCRIPT_KEYMAP))) return 0;

	/* getkey(int) */
	if(!aga_arg_list(args, PY_TYPE_INT)) return aga_arg_error("getkey", "int");

	result = aga_keymap_lookup(keymap, py_int_get(args), &b);
	if(aga_script_err("aga_keymap_lookup", result)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_GETKEY);

	return py_object_incref(b ? PY_TRUE : PY_FALSE);
}

/*
 * TODO: Implement `gluUnProject' mechanism for finding mouse motion relative
 * 		 To the scene.
 */
struct py_object* agan_getmotion(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* retval;
	struct py_object* o;
	struct aga_pointer* pointer;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_GETMOTION);

	if(args) return aga_arg_error("getmotion", "none");

	if(!(pointer = aga_getscriptptr(AGA_SCRIPT_POINTER))) return 0;

	if(!(retval = py_list_new(2))) return py_error_set_nomem();

	if(!(o = py_float_new(pointer->dx))) return py_error_set_nomem();
	py_list_set(retval, 0, o);

	if(!(o = py_float_new(pointer->dy))) return py_error_set_nomem();
	py_list_set(retval, 1, o);

	apro_stamp_end(APRO_SCRIPTGLUE_GETMOTION);

	return retval;
}

struct py_object* agan_setcursor(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum aga_result result;

	struct py_object* v;
	struct py_object* c;

	struct aga_window_device* window_device;
	struct aga_window* win;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_SETCURSOR);

	if(!(window_device = aga_getscriptptr(AGA_SCRIPT_WINDOW_DEVICE))) return 0;
	if(!(win = aga_getscriptptr(AGA_SCRIPT_WINDOW))) return 0;

	/* setcursor(int, int) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
		!aga_arg(&v, args, 0, PY_TYPE_INT) ||
		!aga_arg(&c, args, 1, PY_TYPE_INT)) {

		return aga_arg_error("setcursor", "int and int");
	}

	result = aga_window_set_cursor(
			window_device, win, !!py_int_get(v), !!py_int_get(c));
	if(aga_script_err("aga_window_set_cursor", result)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_SETCURSOR);

	return py_object_incref(PY_NONE);
}

/*
 * TODO: We need to do a massive revamp where we *decref anything we don't
 * 		 Return during an error state.
 */
struct py_object* agan_getbuttons(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* retval;
	struct aga_buttons* buttons;
	unsigned i;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_GETBUTTONS);

	if(args) return aga_arg_error("getbuttons", "none");

	if(!(buttons = aga_getscriptptr(AGA_SCRIPT_BUTTONS))) return 0;

	if(!(retval = py_list_new(AGA_BUTTON_MAX))) return py_error_set_nomem();

	for(i = 0; i < AGA_LEN(buttons->states); ++i) {
		struct py_object* v;

		if(!(v = py_int_new(buttons->states[i]))) return py_error_set_nomem();
		py_list_set(retval, i, v);
	}

	apro_stamp_end(APRO_SCRIPTGLUE_GETBUTTONS);

	return retval;
}

struct py_object* agan_getpos(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* retval;
	struct py_object* v;

	struct aga_pointer* pointer;

	(void) env;
	(void) self;

	if(!(pointer = aga_getscriptptr(AGA_SCRIPT_POINTER))) return 0;

	if(args) return aga_arg_error("getpos", "none");

	if(!(retval = py_list_new(2))) return py_error_set_nomem();

	if(!(v = py_int_new(pointer->x))) {
		py_error_set_nomem();
		return 0;
	}
	py_list_set(retval, 0, v);

	if(!(v = py_int_new(pointer->y))) {
		py_error_set_nomem();
		return 0;
	}
	py_list_set(retval, 1, v);

	return retval;
}


static enum aga_result aga_setkeys(void) {
	enum aga_result result;
#define aga_(name, value) \
	do { \
		result = aga_insertint(name, value); \
		if(result) return result; \
	} while(0)
#ifdef _WIN32
/*
 * Values taken from:
 * https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
 */
	aga_("KEY_CANCEL", 0x03);
	aga_("KEY_BACKSPACE", 0x08);
	aga_("KEY_CLEAR", 0x0C);
	aga_("KEY_PAUSE", 0x13);
	aga_("KEY_ESCAPE", 0x1B);
	aga_("KEY_MENU", 0x12);
	aga_("KEY_TAB", 0x09);
	aga_("KEY_RETURN", 0x0D);
	aga_("KEY_CAPSLOCK", 0x14);
	aga_("KEY_SHIFT_L", 0xA0);
	aga_("KEY_SHIFT_R", 0xA1);
	aga_("KEY_CONTROL_L", 0xA2);
	aga_("KEY_CONTROL_R", 0xA3);
	aga_("KEY_ALT_L", 0xA4);
	aga_("KEY_ALT_R", 0xA5);
	aga_("KEY_KANALOCK", 0x15);
	aga_("KEY_KANASHIFT", 0x15);
	aga_("KEY_HANGUL", 0x15);
	aga_("KEY_HANJA", 0x19);
	aga_("KEY_KANJI", 0x19);
	aga_("KEY_SPACE", 0x20);
	aga_("KEY_PRIOR", 0x21);
	aga_("KEY_NEXT", 0x22);
	aga_("KEY_END", 0x23);
	aga_("KEY_HOME", 0x24);
	aga_("KEY_LEFT", 0x25);
	aga_("KEY_UP", 0x26);
	aga_("KEY_RIGHT", 0x27);
	aga_("KEY_DOWN", 0x28);
	aga_("KEY_SELECT", 0x29);
	aga_("KEY_PRINT", 0x2A);
	aga_("KEY_EXECUTE", 0x2B);
	aga_("KEY_INSERT", 0x2D);
	aga_("KEY_DELETE", 0x2E);
	aga_("KEY_HELP", 0x2F);
	aga_("KEY_0", 0x30);
	aga_("KEY_1", 0x31);
	aga_("KEY_2", 0x32);
	aga_("KEY_3", 0x33);
	aga_("KEY_4", 0x34);
	aga_("KEY_5", 0x35);
	aga_("KEY_6", 0x36);
	aga_("KEY_7", 0x37);
	aga_("KEY_8", 0x38);
	aga_("KEY_9", 0x39);
	aga_("KEY_A", 0x41);
	aga_("KEY_B", 0x42);
	aga_("KEY_C", 0x43);
	aga_("KEY_D", 0x44);
	aga_("KEY_E", 0x45);
	aga_("KEY_F", 0x46);
	aga_("KEY_G", 0x47);
	aga_("KEY_H", 0x48);
	aga_("KEY_I", 0x49);
	aga_("KEY_J", 0x4A);
	aga_("KEY_K", 0x4B);
	aga_("KEY_L", 0x4C);
	aga_("KEY_M", 0x4D);
	aga_("KEY_N", 0x4E);
	aga_("KEY_O", 0x4F);
	aga_("KEY_P", 0x50);
	aga_("KEY_Q", 0x51);
	aga_("KEY_R", 0x52);
	aga_("KEY_S", 0x53);
	aga_("KEY_T", 0x54);
	aga_("KEY_U", 0x55);
	aga_("KEY_V", 0x56);
	aga_("KEY_W", 0x57);
	aga_("KEY_X", 0x58);
	aga_("KEY_Y", 0x59);
	aga_("KEY_Z", 0x5A);
	aga_("KEY_META_L", 0x5B);
	aga_("KEY_META_R", 0x5C);
	aga_("KEY_SUPER_L", 0x5B);
	aga_("KEY_SUPER_R", 0x5C);
	aga_("KEY_NUMPAD0", 0x60);
	aga_("KEY_NUMPAD1", 0x61);
	aga_("KEY_NUMPAD2", 0x62);
	aga_("KEY_NUMPAD3", 0x63);
	aga_("KEY_NUMPAD4", 0x64);
	aga_("KEY_NUMPAD5", 0x65);
	aga_("KEY_NUMPAD6", 0x66);
	aga_("KEY_NUMPAD7", 0x67);
	aga_("KEY_NUMPAD8", 0x68);
	aga_("KEY_NUMPAD9", 0x69);
	aga_("KEY_KP_MULTIPLY", 0x6A);
	aga_("KEY_KP_ADD", 0x6B);
	aga_("KEY_KP_SEPARATOR", 0x6C);
	aga_("KEY_KP_SUBTRACT", 0x6D);
	aga_("KEY_KP_DECIMAL", 0x6E);
	aga_("KEY_KP_DIVIDE", 0x6F);
	aga_("KEY_F1", 0x70);
	aga_("KEY_F2", 0x71);
	aga_("KEY_F3", 0x72);
	aga_("KEY_F4", 0x73);
	aga_("KEY_F5", 0x74);
	aga_("KEY_F6", 0x75);
	aga_("KEY_F7", 0x76);
	aga_("KEY_F8", 0x77);
	aga_("KEY_F9", 0x78);
	aga_("KEY_F10", 0x79);
	aga_("KEY_F11", 0x7A);
	aga_("KEY_F12", 0x7B);
	aga_("KEY_F13", 0x7C);
	aga_("KEY_F14", 0x7D);
	aga_("KEY_F15", 0x7E);
	aga_("KEY_F16", 0x7F);
	aga_("KEY_F17", 0x80);
	aga_("KEY_F18", 0x81);
	aga_("KEY_F19", 0x82);
	aga_("KEY_F20", 0x83);
	aga_("KEY_F21", 0x84);
	aga_("KEY_F22", 0x85);
	aga_("KEY_F23", 0x86);
	aga_("KEY_F24", 0x87);
	aga_("KEY_NUMLOCK", 0x90);
	aga_("KEY_SCROLLLOCK", 0x91);
	aga_("KEY_ATTN", 0xF6);
	aga_("KEY_EXSELECT", 0xF7);
	aga_("KEY_CURSORSELECT", 0xF8);
	aga_("KEY_PLAY", 0xFA);
	aga_("KEY_ERASEEOF", 0xF9);
	aga_("KEY_PA1", 0xFD);
#else
	/*
	 * Values taken from `X11/keysymdef.h'
	 */
	aga_("KEY_CANCEL", 0xFF69);
	aga_("KEY_BACKSPACE", 0xFF08);
	aga_("KEY_CLEAR", 0xFF0B);
	aga_("KEY_PAUSE", 0xFF13);
	aga_("KEY_ESCAPE", 0xFF1B);
	aga_("KEY_MENU", 0xFF67);
	aga_("KEY_TAB", 0xFF09);
	aga_("KEY_RETURN", 0xFF0D);
	aga_("KEY_CAPSLOCK", 0xFFE5);
	aga_("KEY_SHIFT_L", 0xFFE1);
	aga_("KEY_SHIFT_R", 0xFFE2);
	aga_("KEY_CONTROL_L", 0xFFE3);
	aga_("KEY_CONTROL_R", 0xFFE4);
	aga_("KEY_ALT_L", 0xFFE9);
	aga_("KEY_ALT_R", 0xFFEA);
	aga_("KEY_KANALOCK", 0xFF2D);
	aga_("KEY_KANASHIFT", 0xFF2E);
	aga_("KEY_HANGUL", 0xFF31);
	aga_("KEY_HANJA", 0xFF34);
	aga_("KEY_KANJI", 0xFF21);
	aga_("KEY_SPACE", 0x0020);
	aga_("KEY_PRIOR", 0xFF55);
	aga_("KEY_NEXT", 0xFF56);
	aga_("KEY_END", 0xFF57);
	aga_("KEY_HOME", 0xFF50);
	aga_("KEY_LEFT", 0xFF51);
	aga_("KEY_UP", 0xFF52);
	aga_("KEY_RIGHT", 0xFF53);
	aga_("KEY_DOWN", 0xFF54);
	aga_("KEY_PAGE_UP", 0xFF55);
	aga_("KEY_PAGE_DOWN", 0xFF56);
	aga_("KEY_BEGIN", 0xFF58);
	aga_("KEY_SELECT", 0xFF60);
	aga_("KEY_PRINT", 0xFF61);
	aga_("KEY_EXECUTE", 0xFF62);
	aga_("KEY_INSERT", 0xFF63);
	aga_("KEY_DELETE", 0xFFFF);
	aga_("KEY_HELP", 0xFF6A);
	aga_("KEY_0", 0x0030);
	aga_("KEY_1", 0x0031);
	aga_("KEY_2", 0x0032);
	aga_("KEY_3", 0x0033);
	aga_("KEY_4", 0x0034);
	aga_("KEY_5", 0x0035);
	aga_("KEY_6", 0x0036);
	aga_("KEY_7", 0x0037);
	aga_("KEY_8", 0x0038);
	aga_("KEY_9", 0x0039);
	aga_("KEY_A", 0x0061);
	aga_("KEY_B", 0x0062);
	aga_("KEY_C", 0x0063);
	aga_("KEY_D", 0x0064);
	aga_("KEY_E", 0x0065);
	aga_("KEY_F", 0x0066);
	aga_("KEY_G", 0x0067);
	aga_("KEY_H", 0x0068);
	aga_("KEY_I", 0x0069);
	aga_("KEY_J", 0x006A);
	aga_("KEY_K", 0x006B);
	aga_("KEY_L", 0x006C);
	aga_("KEY_M", 0x006D);
	aga_("KEY_N", 0x006E);
	aga_("KEY_O", 0x006F);
	aga_("KEY_P", 0x0070);
	aga_("KEY_Q", 0x0071);
	aga_("KEY_R", 0x0072);
	aga_("KEY_S", 0x0073);
	aga_("KEY_T", 0x0074);
	aga_("KEY_U", 0x0075);
	aga_("KEY_V", 0x0076);
	aga_("KEY_W", 0x0077);
	aga_("KEY_X", 0x0078);
	aga_("KEY_Y", 0x0079);
	aga_("KEY_Z", 0x007A);
	aga_("KEY_META_L", 0xFFE7);
	aga_("KEY_META_R", 0xFFE8);
	aga_("KEY_SUPER_L", 0xFFEB);
	aga_("KEY_SUPER_R", 0xFFEC);
	aga_("KEY_KP_0", 0xFFB0);
	aga_("KEY_KP_1", 0xFFB1);
	aga_("KEY_KP_2", 0xFFB2);
	aga_("KEY_KP_3", 0xFFB3);
	aga_("KEY_KP_4", 0xFFB4);
	aga_("KEY_KP_5", 0xFFB5);
	aga_("KEY_KP_6", 0xFFB6);
	aga_("KEY_KP_7", 0xFFB7);
	aga_("KEY_KP_8", 0xFFB8);
	aga_("KEY_KP_9", 0xFFB9);
	aga_("KEY_KP_MULTIPLY", 0xFFAA);
	aga_("KEY_KP_ADD", 0xFFAB);
	aga_("KEY_KP_SEPARATOR", 0xFFAC);
	aga_("KEY_KP_SUBTRACT", 0xFFAD);
	aga_("KEY_KP_DECIMAL", 0xFFAE);
	aga_("KEY_KP_DIVIDE", 0xFFAF);
	aga_("KEY_F1", 0xFFBE);
	aga_("KEY_F2", 0xFFBF);
	aga_("KEY_F3", 0xFFC0);
	aga_("KEY_F4", 0xFFC1);
	aga_("KEY_F5", 0xFFC2);
	aga_("KEY_F6", 0xFFC3);
	aga_("KEY_F7", 0xFFC4);
	aga_("KEY_F8", 0xFFC5);
	aga_("KEY_F9", 0xFFC6);
	aga_("KEY_F10", 0xFFC7);
	aga_("KEY_F11", 0xFFC8);
	aga_("KEY_F12", 0xFFC9);
	aga_("KEY_F13", 0xFFCA);
	aga_("KEY_F14", 0xFFCB);
	aga_("KEY_F15", 0xFFCC);
	aga_("KEY_F16", 0xFFCD);
	aga_("KEY_F17", 0xFFCE);
	aga_("KEY_F18", 0xFFCF);
	aga_("KEY_F19", 0xFFD0);
	aga_("KEY_F20", 0xFFD1);
	aga_("KEY_F21", 0xFFD2);
	aga_("KEY_F22", 0xFFD3);
	aga_("KEY_F23", 0xFFD4);
	aga_("KEY_F24", 0xFFD5);
	aga_("KEY_NUMLOCK", 0xFF7F);
	aga_("KEY_SCROLLLOCK", 0xFF14);
	aga_("KEY_ATTN", 0xFD0E);
	aga_("KEY_EXSELECT", 0xFD1B);
	aga_("KEY_CURSORSELECT", 0xFD1C);
	aga_("KEY_PLAY", 0xFD16);
	aga_("KEY_ERASEEOF", 0xFD06);
	aga_("KEY_PA1", 0xFD0A);
#endif
#undef aga_

	return AGA_RESULT_OK;
}
