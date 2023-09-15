/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_GLUE_H
#define AGA_SCRIPT_GLUE_H

#include <modsupport.h>

struct aga_nativeptr {
	OB_HEAD
	void* ptr;
};

static typeobject aga_nativeptrtype = {
	OB_HEAD_INIT(&Typetype)
	0, "nativeptr", sizeof(struct aga_nativeptr),
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void aga_setfmterr(object* exception, const char* fmt, ...) {
	static char str[2048 + 1];

	va_list l;
	va_start(l, fmt);

	/* NOTE: `vsnprintf' doesn't exist yet. Demons may arise. */
	vsprintf(str, fmt, l);
	err_setstr(exception, str);

	va_end(l);
}

static object* agan_getkey(object* self, object* arg) {
	(void) self;

	if(!arg || !is_intobject(arg)) {
		err_setstr(TypeError, "getkey() argument must be int");
		return 0;
	}

	return script_ctx->keystates[getintvalue(arg)] ? True : False;
}

static object* agan_getmotion(object* self, object* arg) {
	object* retval;
	object* x;
	object* y;

	(void) self, (void) arg;

	retval = newlistobject(2);
	aga_scriptchk();

	x = newfloatobject(script_ctx->pointer_dx);
	aga_scriptchk();

	y = newfloatobject(script_ctx->pointer_dy);
	aga_scriptchk();

	setlistitem(retval, 0, x);
	aga_scriptchk();

	setlistitem(retval, 1, y);
	aga_scriptchk();

	return retval;
}

static object* agan_setcam(object* self, object* arg) {
	af_size_t i;

	(void) self;

	if(!arg || !is_classmemberobject(arg)) {
		err_setstr(RuntimeError, "setcam() argument must be transform");
		return 0;
	}

	{
		object* pos = getattr(arg, "pos");
		aga_scriptchk();

		for(i = 0; i < 3; ++i) {
			float f;
			object* p;

			p = getlistitem(pos, (int) i);
			aga_scriptchk();

			f = (float) getfloatvalue(p);
			aga_scriptchk();

			script_ctx->cam.pos.comp[i] = f;
		}
	}

	{
		object* v;
		float f;
		object* rot = getattr(arg, "rot");
		aga_scriptchk();

		v = getlistitem(rot, 1);
		aga_scriptchk();

		f = (float) getfloatvalue(v);
		aga_scriptchk();

		script_ctx->cam.yaw = f;

		v = getlistitem(rot, 0);
		aga_scriptchk();

		f = (float) getfloatvalue(v);
		aga_scriptchk();

		script_ctx->cam.pitch = f;
	}

	/* TODO: `af_err` -> python exception. */
	if(aga_setcam(script_ctx)) {
		aga_setfmterr(RuntimeError, "aga_setcam() failed");
		return 0;
	}

	return 0;
}

enum af_err aga_mkmod(void) {
	struct methodlist methods[] = {
		{ "getkey", agan_getkey },
		{ "setcam", agan_setcam },
		{ "getmotion", agan_getmotion },
		{ 0, 0 }
	};

	object* module = initmodule("agan", methods);
	AF_VERIFY(module, AF_ERR_UNKNOWN);

	return AF_ERR_NONE;
}

enum af_err aga_killmod(void) {
	return AF_ERR_NONE;
}

#endif
