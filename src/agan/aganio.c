/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/aganio.h>

#include <agawin.h>
#include <agascript.h>
#include <agascripthelp.h>

#include <apro.h>

struct py_object* agan_getkey(struct py_object* self, struct py_object* arg) {
	enum aga_result result;
	py_value_t value;
	aga_bool_t b;
	struct aga_keymap* keymap;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_GETKEY);

	if(!(keymap = aga_getscriptptr(AGA_SCRIPT_KEYMAP))) return 0;

	if(!aga_arg_list(arg, PY_TYPE_INT)) return aga_arg_error("getkey", "int");

	if(aga_script_int(arg, &value)) return 0;

	result = aga_keylook(keymap, (aga_uint8_t) value, &b);
	if(aga_script_err("aga_keylook", result)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_GETKEY);

	return py_object_incref(b ? PY_TRUE : PY_FALSE);
}

struct py_object* agan_getmotion(
		struct py_object* self, struct py_object* arg) {

	struct py_object* retval;
	struct py_object* o;
	struct aga_pointer* pointer;

	(void) self;
	(void) arg;

	apro_stamp_start(APRO_SCRIPTGLUE_GETMOTION);

	if(!(pointer = aga_getscriptptr(AGA_SCRIPT_POINTER))) return 0;

	if(!(retval = py_list_new(2))) return 0;

	if(!(o = py_float_new(pointer->dx))) return 0;
	if(aga_list_set(retval, 0, o)) return 0;

	if(!(o = py_float_new(pointer->dy))) return 0;
	if(aga_list_set(retval, 1, o)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_GETMOTION);

	return retval;
}

struct py_object* agan_setcursor(
		struct py_object* self, struct py_object* arg) {

	enum aga_result result;

	struct py_object* o;
	struct py_object* v;
	aga_bool_t visible, captured;

	struct aga_winenv* env;
	struct aga_win* win;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_SETCURSOR);

	if(!(env = aga_getscriptptr(AGA_SCRIPT_WINENV))) return 0;
	if(!(win = aga_getscriptptr(AGA_SCRIPT_WIN))) return 0;

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&o, arg, 0, PY_TYPE_INT) ||
	   !aga_arg(&v, arg, 1, PY_TYPE_INT)) {

		return aga_arg_error("setcursor", "int and int");
	}

	if(aga_script_bool(o, &visible)) return 0;
	if(aga_script_bool(v, &captured)) return 0;

	result = aga_setcursor(env, win, visible, captured);
	if(aga_script_err("aga_setcursor", result)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_SETCURSOR);

	return py_object_incref(PY_NONE);
}

/*
 * TODO: We need to do a massive revamp where we *decref anything we don't
 * 		 Return during an error state.
 */
struct py_object* agan_getbuttons(
		struct py_object* self, struct py_object* arg) {

	struct py_object* retval;
	struct aga_buttons* buttons;
	unsigned i;

	(void) self;
	(void) arg;

	apro_stamp_start(APRO_SCRIPTGLUE_GETBUTTONS);

	if(!(buttons = aga_getscriptptr(AGA_SCRIPT_BUTTONS))) return 0;

	if(!(retval = py_list_new(AGA_BUTTON_MAX))) return 0;

	for(i = 0; i < AGA_LEN(buttons->states); ++i) {
		struct py_object* v;

		if(!(v = py_int_new(buttons->states[i]))) return 0;
		if(aga_list_set(retval, i, v)) return 0;
	}

	apro_stamp_end(APRO_SCRIPTGLUE_GETBUTTONS);

	return retval;
}
