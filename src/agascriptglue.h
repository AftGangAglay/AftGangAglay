/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_GLUE_H
#define AGA_SCRIPT_GLUE_H

#include <agaio.h>
#include <agaimg.h>
#include <agalog.h>

#include <modsupport.h>

struct aga_nativeptr {
	OB_HEAD
	void* ptr;
	af_size_t len;
};

static void aga_nativeptr_dealloc(object* _) { (void) _; }

static const typeobject aga_nativeptr_type = {
	OB_HEAD_INIT(&Typetype)
	0,
	"nativeptr", sizeof(struct aga_nativeptr), 0,
	aga_nativeptr_dealloc, 0, 0, 0, 0, 0, 0, 0, 0
};

static void aga_setfmterr(object* exception, const char* fmt, ...) {
	aga_fixed_buf_t str = { 0 };

	va_list l;
	va_start(l, fmt);

	/* NOTE: `vsnprintf' doesn't exist yet. Demons may arise. */
	vsprintf(str, fmt, l);
	err_setstr(exception, str);

	va_end(l);
}

static object* agan_getkey(object* self, object* arg) {
	long value;

	(void) self;

	if(!arg || !is_intobject(arg)) {
		err_setstr(TypeError, "getkey() argument must be int");
		return 0;
	}

	value = getintvalue(arg);
	aga_scriptchk();

	return script_ctx->keystates[value] ? True : False;
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

	INCREF(None);
	return None;
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

static object* agan_mklargefile(object* self, object* arg) {
	struct aga_nativeptr* retval;
	const char* path;

	(void) self;

	if(!arg || !is_stringobject(arg)) {
		err_setstr(RuntimeError, "mklargefile() argument must be string");
		return 0;
	}

	path = getstringvalue(arg);
	retval = NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
	if(!retval) return 0;

	if(AGA_MK_LARGE_FILE_STRATEGY(
		path, (af_uchar_t**) &retval->ptr, &retval->len)) {

		err_setstr(RuntimeError, "AGA_MK_LARGE_FILE_STRATEGY() failed");
		return 0;
	}

	return (object*) retval;
}

static object* agan_killlargefile(object* self, object* arg) {
	struct aga_nativeptr* nativeptr;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "killlargefile() argument must be nativeptr");
		return 0;
	}

	nativeptr = (struct aga_nativeptr*) arg;

	if(AGA_KILL_LARGE_FILE_STRATEGY(nativeptr->ptr, nativeptr->len)) {
		err_setstr(RuntimeError, "AGA_KILL_LARGE_FILE_STRATEGY() failed");
		return 0;
	}

	INCREF(None);
	return None;
}

static object* agan_mkvertbuf(object* self, object* arg) {
	struct aga_nativeptr* nativeptr;
	struct aga_nativeptr* retval;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "mkvertbuf() argument must be nativeptr");
		return 0;
	}

	nativeptr = (struct aga_nativeptr*) arg;

	retval = NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
	if(!retval) return 0;

	if(!(retval->ptr = malloc(sizeof(struct af_buf)))) {
		err_nomem();
		return 0;
	}

	if(af_mkbuf(&script_ctx->af_ctx, retval->ptr, AF_BUF_VERT)) {
		err_setstr(RuntimeError, "af_mkbuf() failed");
		return 0;
	}

	if(af_upload(
		&script_ctx->af_ctx, retval->ptr, nativeptr->ptr,
		nativeptr->len)) {

		err_setstr(RuntimeError, "af_upload() failed");
		return 0;
	}

	return (object*) retval;
}

static object* agan_killvertbuf(object* self, object* arg) {
	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "killvertbuf() argument must be nativeptr");
		return 0;
	}

	if(af_killbuf(&script_ctx->af_ctx, ((struct aga_nativeptr*) arg)->ptr)) {
		err_setstr(RuntimeError, "af_killbuf() failed");
		return 0;
	}

	free(((struct aga_nativeptr*) arg)->ptr);

	INCREF(None);
	return None;
}

static object* agan_drawbuf(object* self, object* arg) {
	object* v;
	object* o;
	object* t;

	void* ptr;
	long primitive;

	(void) self;

	if(!arg || !is_tupleobject(arg) ||
		!(v = gettupleitem(arg, 0)) || v->ob_type != &aga_nativeptr_type ||
		!(o = gettupleitem(arg, 1)) || !is_intobject(o) ||
		!(t = gettupleitem(arg, 2)) || !is_classmemberobject(t)) {

		err_setstr(
			RuntimeError,
			"drawbuf() arguments must be nativeptr, int and transform");
		return 0;
	}

	ptr = ((struct aga_nativeptr*) v)->ptr;
	primitive = getintvalue(o);
	aga_scriptchk();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	{
		const char* comps[] = { "pos", "rot", "scale" };
		object* comp;
		object* xo;
		object* yo;
		object* zo;
		float x, y, z;
		af_size_t i;

		for(i = 0; i < 3; ++i) {
			void (*proc)(float, float, float) = glScalef;

			if(!(comp = getattr(t, (char*) comps[i]))) return 0;
			if(!(xo = getlistitem(comp, 0))) return 0;
			if(!(yo = getlistitem(comp, 1))) return 0;
			if(!(zo = getlistitem(comp, 2))) return 0;

			x = (float) getfloatvalue(xo);
			aga_scriptchk();
			y = (float) getfloatvalue(yo);
			aga_scriptchk();
			z = (float) getfloatvalue(zo);
			aga_scriptchk();

			switch(i) {
				default: break;
				case 1: {
					glRotatef(x, 1.0f, 0.0f, 0.0f);
					glRotatef(y, 0.0f, 1.0f, 0.0f);
					glRotatef(z, 0.0f, 0.0f, 1.0f);
					break;
				}
				case 0: proc = glTranslatef;
					AF_FALLTHROUGH;
					/* FALLTHRU */
				case 2: {
					proc(x, y, z);
					break;
				}
			}
		}
	}

	if(af_drawbuf(&script_ctx->af_ctx, ptr, &script_ctx->vert, primitive)) {
		err_setstr(RuntimeError, "af_drawbuf() failed");
		return 0;
	}

	INCREF(None);
	return None;
}

struct agan_teximg {
	struct aga_img img;
	struct af_buf tex;
};

static object* agan_mkteximg(object* self, object* arg) {
	struct aga_nativeptr* retval;
	const char* path;

	(void) self;

	if(!arg || !is_stringobject(arg)) {
		err_setstr(
			RuntimeError, "mkteximg() argument must be string");
		return 0;
	}

	retval = NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
	if(!retval) return 0;

	if(!(retval->ptr = malloc(sizeof(struct agan_teximg)))) {
		err_nomem();
		return 0;
	}

	if(!(path = getstringvalue(arg))) return 0;

	if(aga_mkimg(&((struct agan_teximg*) retval->ptr)->img, path)) {
		err_setstr(RuntimeError, "aga_mkimg() failed");
		return 0;
	}

	if(aga_mkteximg(
		&script_ctx->af_ctx, &((struct agan_teximg*) retval->ptr)->img,
		&((struct agan_teximg*) retval->ptr)->tex)) {

		err_setstr(RuntimeError, "aga_mkteximg() failed");
		return 0;
	}

	return (object*) retval;
}

static object* agan_settex(object* self, object* arg) {
	struct agan_teximg* teximg;

	(void) self;

	if (!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(RuntimeError, "settex() argument must be nativeptr");
		return 0;
	}

	teximg = ((struct aga_nativeptr*) arg)->ptr;

	if(af_settex(&script_ctx->af_ctx, &teximg->tex)) {
		err_setstr(RuntimeError, "af_settex() failed");
		return 0;
	}

	INCREF(None);
	return None;
}

static object* agan_killteximg(object* self, object* arg) {
	struct agan_teximg* teximg;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "killteximg() argument must be nativeptr");
		return 0;
	}

	teximg = ((struct aga_nativeptr*) arg)->ptr;

	if(af_killbuf(&script_ctx->af_ctx, &teximg->tex)) {
		err_setstr(RuntimeError, "af_killbuf() failed");
		return 0;
	}

	if(aga_killimg(&teximg->img)) {
		err_setstr(RuntimeError, "aga_killimg() failed");
		return 0;
	}

	free(((struct aga_nativeptr*) arg)->ptr);

	INCREF(None);
	return None;
}

static object* agan_setcol(object* self, object* arg) {
	object* v;
	float r, g, b;

	(void) self;

	if(!arg || !is_listobject(arg)) {
		err_setstr(
			RuntimeError, "setcol() argument must be list");
		return 0;
	}

	if(!(v = getlistitem(arg, 0))) return 0;
	r = (float) getfloatvalue(v);
	aga_scriptchk();
	if(!(v = getlistitem(arg, 1))) return 0;
	g = (float) getfloatvalue(v);
	aga_scriptchk();
	if(!(v = getlistitem(arg, 2))) return 0;
	b = (float) getfloatvalue(v);
	aga_scriptchk();

	glColor3f(r, g, b);

	INCREF(None);
	return None;
}

static object* agan_mkclip(object* self, object* arg) {
	struct aga_nativeptr* retval;
	struct aga_nativeptr* file;
	struct aga_clip* clip;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "mkclip() argument must be nativeptr");
		return 0;
	}

	file = (struct aga_nativeptr*) arg;

	retval = NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
	if(!retval) return 0;

	if(!(retval->ptr = malloc(sizeof(struct aga_clip)))) {
		err_nomem();
		return 0;
	}

	clip = retval->ptr;

	clip->len = file->len;
	clip->pcm = file->ptr;
	clip->pos = 0;

	return (object*) retval;
}

static object* agan_putclip(object* self, object* arg) {
	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "putclip() argument must be nativeptr");
		return 0;
	}

	if(script_ctx->settings.audio_enabled) {
		if(aga_putclip(
			&script_ctx->snddev, ((struct aga_nativeptr*) arg)->ptr)) {

			err_setstr(RuntimeError, "aga_putclip() failed");
			return 0;
		}
	}

	INCREF(None);
	return None;
}

static object* agan_killclip(object* self, object* arg) {
	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "killclip() argument must be nativeptr");
		return 0;
	}

	free(((struct aga_nativeptr*) arg)->ptr);

	INCREF(None);
	return None;
}

static object* agan_log(object* self, object* arg) {
	const char* str;
	const char* loc;

	(void) self;

	if(!arg || !is_stringobject(arg)) {
		err_setstr(
			RuntimeError, "log() argument must be string");
		return 0;
	}

	str = getstringvalue(arg);
	if(!str) return 0;

	if(!(loc = getstringvalue(current_frame->f_code->co_filename))) return 0;

	aga_log(loc, str);

	INCREF(None);
	return None;
}

static object* agan_yeslight(object* self, object* arg) {
	(void) self, (void) arg;

	glEnable(GL_LIGHTING);
	aga_af_chk("glEnable", af_gl_chk());

	INCREF(None);
	return None;
}

static object* agan_nolight(object* self, object* arg) {
	(void) self, (void) arg;

	glDisable(GL_LIGHTING);
	aga_af_chk("glDisable", af_gl_chk());

	INCREF(None);
	return None;
}

enum af_err aga_mkmod(void) {
	struct methodlist methods[] = {
		{ "getkey", agan_getkey },
		{ "setcam", agan_setcam },
		{ "getmotion", agan_getmotion },
		{ "getconf", agan_getconf },
		{ "mklargefile", agan_mklargefile },
		{ "killlargefile", agan_killlargefile },
		{ "mkvertbuf", agan_mkvertbuf },
		{ "killvertbuf", agan_killvertbuf },
		{ "drawbuf", agan_drawbuf },
		{ "mkteximg", agan_mkteximg },
		{ "settex", agan_settex },
		{ "killteximg", agan_killteximg },
		{ "setcol", agan_setcol },
		{ "mkclip", agan_mkclip },
		{ "putclip", agan_putclip },
		{ "killclip", agan_killclip },
		{ "log", agan_log },
		{ "nolight", agan_nolight },
		{ "yeslight", agan_yeslight },
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
