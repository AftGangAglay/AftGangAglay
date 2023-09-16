/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_GLUE_H
#define AGA_SCRIPT_GLUE_H

#include <modsupport.h>

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
	if(!retval) return 0;

	x = newfloatobject(script_ctx->pointer_dx);
	if(!x) return 0;

	y = newfloatobject(script_ctx->pointer_dy);
	if(!y) return 0;

	if(setlistitem(retval, 0, x) == -1) return 0;
	if(setlistitem(retval, 1, y) == -1) return 0;

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
		if(!pos) return 0;

		for(i = 0; i < 3; ++i) {
			float f;
			object* p;

			p = getlistitem(pos, (int) i);
			if(!p) return 0;

			f = (float) getfloatvalue(p);
			aga_scriptchk();

			script_ctx->cam.pos.comp[i] = f;
		}
	}

	{
		object* v;
		float f;
		object* rot = getattr(arg, "rot");
		if(!rot) return 0;

		v = getlistitem(rot, 1);
		if(!v) return 0;

		f = (float) getfloatvalue(v);
		aga_scriptchk();
		/*
		 * NOTE: The error state reporting out of `getfloatvalue' is borked.
		if(f == -1) return 0;
		 */
		script_ctx->cam.yaw = f;

		v = getlistitem(rot, 0);
		if(!v) return 0;
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

static object* agan_getconf(object* self, object* arg) {
	object* conf;
	object* val;

	(void) self, (void) arg;

	conf = newdictobject();
	aga_scriptchk();

	val = newfloatobject(script_ctx->settings.sensitivity);
	if(!val) return 0;
	if(dictinsert(conf, "sensitivity", val) == -1) return 0;

	val = newfloatobject(script_ctx->settings.move_speed);
	if(!val) return 0;
	if(dictinsert(conf, "move_speed", val)) return 0;

	val = newfloatobject(script_ctx->settings.fov);
	if(!val) return 0;
	if(dictinsert(conf, "fov", val)) return 0;

	val = newintobject(script_ctx->settings.width);
	if(!val) return 0;
	if(dictinsert(conf, "width", val)) return 0;

	val = newintobject(script_ctx->settings.height);
	if(!val) return 0;
	if(dictinsert(conf, "height", val)) return 0;

	val = newintobject(script_ctx->settings.audio_enabled);
	if(!val) return 0;
	if(dictinsert(conf, "audio_enabled", val)) return 0;

	val = newstringobject((char*) script_ctx->settings.audio_dev);
	if(!val) return 0;
	if(dictinsert(conf, "audio_dev", val)) return 0;

	val = newstringobject((char*) script_ctx->settings.startup_script);
	if(!val) return 0;
	if(dictinsert(conf, "startup_script", val)) return 0;

	val = newstringobject((char*) script_ctx->settings.python_path);
	if(!val) return 0;
	if(dictinsert(conf, "python_path", val)) return 0;

	return conf;
}

enum af_err aga_mkmod(void) {
	struct methodlist methods[] = {
		{ "getkey", agan_getkey },
		{ "setcam", agan_setcam },
		{ "getmotion", agan_getmotion },
		{ "getconf", agan_getconf },
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
