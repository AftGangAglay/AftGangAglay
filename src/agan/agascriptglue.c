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

#include <agan/aganobj.h>
#include <agan/aganio.h>
#include <agan/agandraw.h>
#include <agan/aganmisc.h>
#include <agan/aganmath.h>

#include <agagl.h>
#include <agalog.h>
#include <agascript.h>
#include <agawin.h>
#include <agastartup.h>

#include <apro.h>

static enum aga_result aga_insertstr(const char* key, const char* value) {
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
			_(getkey), _(getmotion), _(setcursor), _(getbuttons), _(getpos),

			/* Drawing */
			_(setcam), _(text), _(fogparam), _(fogcol), _(clear), _(mktrans),
			_(shadeflat), _(getpix),

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

	result = aga_insertint("CLICK", AGA_BUTTON_CLICK);
	if(result) return result;

	result = aga_insertint("DOWN", AGA_BUTTON_DOWN);
	if(result) return result;

	result = aga_insertint("UP", AGA_BUTTON_UP);
	if(result) return result;

	result = aga_insertstr("VERSION", AGA_VERSION);
	if(result) return result;

#ifdef _WIN32
	result = aga_insertstr("PLATFORM", "win32");
#else
	result = aga_insertstr("PLATFORM", "x");
#endif
	if(result) return result;

#ifdef _DEBUG
	result = aga_insertstr("MODE", "debug");
#else
	result = aga_insertstr("MODE", "release");
#endif
	if(result) return result;

#ifdef _MSC_VER
	result = aga_insertstr("CENV", "vc");
#elif defined(__GNUC__)
	result = aga_insertstr("CENV", "gnu");
#else
	result = aga_insertstr("CENV", "std");
#endif
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
