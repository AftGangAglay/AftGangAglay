/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_GLUE_H
#define AGA_SCRIPT_GLUE_H

static af_bool_t aga_script_aferr(const char* proc, enum af_err err) {
	aga_fixed_buf_t buf = { 0 };

	if(!err) return AF_FALSE;

	if(sprintf(buf, "%s: %s", proc, aga_af_errname(err)) < 0) {
		aga_af_errno(__FILE__, "sprintf");
		return AF_TRUE;
	}
	err_setstr(RuntimeError, buf);

	return AF_TRUE;
}

static af_bool_t aga_script_glerr(const char* proc) {
	aga_fixed_buf_t buf = { 0 };
	af_uint_t err;
	af_uint_t tmp = glGetError();
	const char* s;
	if(!tmp) return AF_FALSE;

	do {
		err = tmp;
		s = (const char*) gluErrorString(err);
		aga_log(__FILE__, "err: %s: %s", proc, s);
	} while((tmp = glGetError()));

	if(sprintf(buf, "%s: %s", proc, s) < 0) {
		aga_af_errno(__FILE__, "sprintf");
		return AF_TRUE;
	}
	err_setstr(RuntimeError, buf);

	return AF_TRUE;
}

/*
 * NOTE: We need a bit of global state here to get engine system contexts etc.
 * 		 Into script land because this version of Python's state is spread
 * 		 Across every continent.
 */
static object* agan_dict;
static object* aga_dict;

static void* aga_getscriptptr(const char* key) {
	object* ptr;

	if(!key) {
		err_setstr(RuntimeError, "unexpected null pointer");
		return 0;
	}

	ptr = dictlookup(agan_dict, (char*) key);
	if(!ptr) {
		err_setstr(RuntimeError, "failed to resolve script nativeptr");
		return 0;
	}

	if(ptr->ob_type != &aga_nativeptr_type) {
		err_badarg();
		return 0;
	}

	return ((struct aga_nativeptr*) ptr)->ptr;
}

static object* agan_getkey(object* self, object* arg) {
	long value;
	object* retval = None;
	struct aga_keymap* keymap;

	(void) self;

	if(!(keymap = aga_getscriptptr(AGA_SCRIPT_KEYMAP))) return 0;

	if(!arg || !is_intobject(arg)) {
		err_setstr(TypeError, "getkey() argument must be int");
		return 0;
	}

	value = getintvalue(arg);
	if(err_occurred()) return 0;

	if(keymap->keystates) {
		if(value < keymap->keysyms_per_keycode * keymap->keycode_len) {
			retval = keymap->keystates[value] ? True : False;
		}
	}

	INCREF(retval);
	return retval;
}

static object* agan_getmotion(object* self, object* arg) {
	object* retval;
	object* x;
	object* y;
	struct aga_pointer* pointer;

	(void) self, (void) arg;

	if(!(pointer = aga_getscriptptr(AGA_SCRIPT_POINTER))) return 0;

	retval = newlistobject(2);
	if(!retval) return 0;

	x = newfloatobject(pointer->dx);
	if(!x) return 0;

	y = newfloatobject(pointer->dy);
	if(!y) return 0;

	if(setlistitem(retval, 0, x) == -1) return 0;
	if(setlistitem(retval, 1, y) == -1) return 0;

	return retval;
}

static object* agan_setcam(object* self, object* arg) {
	af_size_t i;

	object* t;
	object* mode;
	af_bool_t b;
	double ar;

	struct aga_opts* opts;

	(void) self;

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!arg || !is_tupleobject(arg) ||
		!(t = gettupleitem(arg, 0)) || !is_classmemberobject(t) ||
		!(mode = gettupleitem(arg, 1)) || !is_intobject(mode)) {

		err_setstr(
			RuntimeError, "setcam() arguments must be transform and int");
		return 0;
	}

	b = !!getintvalue(mode);
	if(err_occurred()) return 0;

	ar = (double) opts->height / (double) opts->width;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if(b) gluPerspective(opts->fov, 1.0 / ar, 0.1, 10000.0);
	else glOrtho(-1.0, 1.0, -ar, ar, 0.001, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	{
		object* v;
		float f;
		object* rot = getattr(t, "rot");
		if(!rot) return 0;

		v = getlistitem(rot, 0);
		if(!v) return 0;
		f = (float) getfloatvalue(v);
		if(err_occurred()) return 0;
		glRotatef(f, 1.0f, 0.0f, 0.0f);

		v = getlistitem(rot, 1);
		if(!v) return 0;
		f = (float) getfloatvalue(v);
		if(err_occurred()) return 0;
		glRotatef(f, 0.0f, 1.0f, 0.0f);
	}

	{
		float comps[3];
		object* pos = getattr(t, "pos");
		if(!pos) return 0;

		for(i = 0; i < 3; ++i) {
			object* p;

			p = getlistitem(pos, (int) i);
			if(!p) return 0;

			comps[i] = (float) getfloatvalue(p);
			if(err_occurred()) return 0;
		}

		glTranslatef(comps[0], comps[1], comps[2]);
	}

	INCREF(None);
	return None;
}

static object* agan_getconf(object* self, object* arg) {
	struct aga_conf_node* node;
	const char** names;
	af_size_t i, len;
	object* v;

	struct aga_opts* opts;

	(void) self;

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!arg || !is_listobject(arg)) {
		err_setstr(RuntimeError, "getconf() argument must be list");
		return 0;
	}

	if(!(names = malloc(sizeof(char*) * (len = getlistsize(arg)))))
		return err_nomem();

	for(i = 0; i < len; ++i) {
		v = getlistitem(arg, (int) i);
		if(!(names[i] = getstringvalue(v))) {
			free(names);
			return 0;
		}
	}

	if(aga_script_aferr("aga_conftree_raw", aga_conftree_raw(
		opts->config.children, names, len, &node))) {

		free(names);
		return 0;
	}
	free(names);

	switch(node->type) {
		default:
			AF_FALLTHROUGH;
			/* FALLTHRU */
		case AGA_NONE: {
			INCREF(None);
			return None;
		}
		case AGA_STRING: return newstringobject(node->data.string);
		case AGA_INTEGER: return newintobject(node->data.integer);
		case AGA_FLOAT: return newfloatobject(node->data.flt);
	}
}

static object* agan_mklargefile(object* self, object* arg) {
	struct aga_nativeptr* retval;
	const char* path;
	enum af_err result;

	(void) self;

	if(!arg || !is_stringobject(arg)) {
		err_setstr(RuntimeError, "mklargefile() argument must be string");
		return 0;
	}

	path = getstringvalue(arg);
	retval = NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
	if(!retval) return 0;

	result = aga_mklargefile(path, &retval->ptr, &retval->len);
	if(aga_script_aferr("aga_mklargefile", result)) return 0;

	return (object*) retval;
}

static object* agan_killlargefile(object* self, object* arg) {
	struct aga_nativeptr* nativeptr;
	enum af_err result;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "killlargefile() argument must be nativeptr");
		return 0;
	}

	nativeptr = (struct aga_nativeptr*) arg;

	result = aga_killlargefile(nativeptr->ptr, nativeptr->len);
	if(aga_script_aferr("aga_killlargefile", result)) return 0;

	INCREF(None);
	return None;
}

static object* agan_mkvertbuf(object* self, object* arg) {
	struct aga_nativeptr* nativeptr;
	struct aga_nativeptr* retval;
	enum af_err result;

	struct af_ctx* af;

	(void) self;

	if(!(af = aga_getscriptptr(AGA_SCRIPT_AFCTX))) return 0;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(RuntimeError, "mkvertbuf() argument must be nativeptr");
		return 0;
	}

	nativeptr = (struct aga_nativeptr*) arg;

	retval = NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
	if(!retval) return 0;

	if(!(retval->ptr = malloc(sizeof(struct af_buf)))) return err_nomem();

	result = af_mkbuf(af, retval->ptr, AF_BUF_VERT);
	if(aga_script_aferr("af_mkbuf", result)) return 0;

	result = af_upload(af, retval->ptr, nativeptr->ptr, nativeptr->len);
	if(aga_script_aferr("af_upload", result)) return 0;

	return (object*) retval;
}

static object* agan_killvertbuf(object* self, object* arg) {
	enum af_err result;

	struct af_ctx* af;

	(void) self;

	if(!(af = aga_getscriptptr(AGA_SCRIPT_AFCTX))) return 0;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "killvertbuf() argument must be nativeptr");
		return 0;
	}

	result = af_killbuf(af, ((struct aga_nativeptr*) arg)->ptr);
	if(aga_script_aferr("af_killbuf", result)) return 0;

	free(((struct aga_nativeptr*) arg)->ptr);

	INCREF(None);
	return None;
}

static af_bool_t agan_settransmat(object* trans) {
	const char* comps[] = { "pos", "rot", "scale" };
	object* comp;
	object* xo;
	object* yo;
	object* zo;
	float x, y, z;
	af_size_t i;

	for(i = 0; i < 3; ++i) {
		if(!(comp = getattr(trans, (char*) comps[i]))) return AF_FALSE;
		if(!(xo = getlistitem(comp, 0))) return AF_FALSE;
		if(!(yo = getlistitem(comp, 1))) return AF_FALSE;
		if(!(zo = getlistitem(comp, 2))) return AF_FALSE;

		x = (float) getfloatvalue(xo);
		if(err_occurred()) return AF_FALSE;
		y = (float) getfloatvalue(yo);
		if(err_occurred()) return AF_FALSE;
		z = (float) getfloatvalue(zo);
		if(err_occurred()) return AF_FALSE;

		switch(i) {
			default: break;
			case 1: {
				glRotatef(x, 1.0f, 0.0f, 0.0f);
				glRotatef(y, 0.0f, 1.0f, 0.0f);
				glRotatef(z, 0.0f, 0.0f, 1.0f);
				break;
			}
			case 0: {
				glTranslatef(x, y, z);
				break;
			}
			case 2: {
				glScalef(x, y, z);
				break;
			}
		}
	}

	return AF_TRUE;
}

static object* agan_drawbuf(object* self, object* arg) {
	object* v;
	object* o;
	object* t;

	enum af_err result;

	void* ptr;
	long primitive;

	struct af_ctx* af;
	struct af_vert* vert;

	(void) self;

	if(!(af = aga_getscriptptr(AGA_SCRIPT_AFCTX))) return 0;
	if(!(vert = aga_getscriptptr(AGA_SCRIPT_AFVERT))) return 0;

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
	if(err_occurred()) return 0;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if(!agan_settransmat(t)) return 0;

	result = af_drawbuf(af, ptr, vert, primitive);
	if(aga_script_aferr("af_drawbuf", result)) return 0;

	glPopMatrix();

	INCREF(None);
	return None;
}

struct agan_teximg {
	struct aga_img img;
	struct af_buf tex;
};

static object* agan_mkteximg(object* self, object* arg) {
	object* str;
	object* filter;

	enum af_err result;

	struct aga_nativeptr* retval;
	struct agan_teximg* teximg;
	const char* path;

	af_bool_t f;

	struct af_ctx* af;

	(void) self;

	if(!(af = aga_getscriptptr(AGA_SCRIPT_AFCTX))) return 0;

	if(!arg || !is_tupleobject(arg) ||
		!(str = gettupleitem(arg, 0)) || !is_stringobject(str) ||
		!(filter = gettupleitem(arg, 1)) || !is_intobject(filter)) {
		err_setstr(
			RuntimeError, "mkteximg() arguments must be string and int");
		return 0;
	}

	f = !!getintvalue(filter);
	if(err_occurred()) return 0;

	retval = NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
	if(!retval) return 0;

	if(!(retval->ptr = malloc(sizeof(struct agan_teximg)))) return err_nomem();
	teximg = retval->ptr;

	if(!(path = getstringvalue(str))) return 0;

	result = aga_mkimg(&teximg->img, path);
	if(aga_script_aferr("aga_mkimg", result)) return 0;

	result = aga_mkteximg(af, &teximg->img, &teximg->tex, f);
	if(aga_script_aferr("aga_mkteximg", result)) return 0;

	return (object*) retval;
}

static object* agan_settex(object* self, object* arg) {
	enum af_err result;

	struct agan_teximg* teximg;

	struct af_ctx* af;

	(void) self;

	if(!(af = aga_getscriptptr(AGA_SCRIPT_AFCTX))) return 0;

	if (!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(RuntimeError, "settex() argument must be nativeptr");
		return 0;
	}

	teximg = ((struct aga_nativeptr*) arg)->ptr;

	result = af_settex(af, &teximg->tex);
	if(aga_script_aferr("af_settex", result)) return 0;

	INCREF(None);
	return None;
}

static object* agan_killteximg(object* self, object* arg) {
	enum af_err result;

	struct agan_teximg* teximg;

	struct af_ctx* af;

	(void) self;

	if(!(af = aga_getscriptptr(AGA_SCRIPT_AFCTX))) return 0;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(
			RuntimeError, "killteximg() argument must be nativeptr");
		return 0;
	}

	teximg = ((struct aga_nativeptr*) arg)->ptr;

	result = af_killbuf(af, &teximg->tex);
	if(aga_script_aferr("af_killbuf", result)) return 0;

	result = aga_killimg(&teximg->img);
	if(aga_script_aferr("aga_killimg", result)) return 0;

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
	if(err_occurred()) return 0;
	if(!(v = getlistitem(arg, 1))) return 0;
	g = (float) getfloatvalue(v);
	if(err_occurred()) return 0;
	if(!(v = getlistitem(arg, 2))) return 0;
	b = (float) getfloatvalue(v);
	if(err_occurred()) return 0;

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
		err_setstr(RuntimeError, "mkclip() argument must be nativeptr");
		return 0;
	}

	file = (struct aga_nativeptr*) arg;

	retval = NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
	if(!retval) return 0;

	if(!(retval->ptr = malloc(sizeof(struct aga_clip)))) return err_nomem();

	clip = retval->ptr;

	clip->len = file->len;
	clip->pcm = file->ptr;
	clip->pos = 0;

	return (object*) retval;
}

static object* agan_putclip(object* self, object* arg) {
	enum af_err result;

	struct aga_opts* opts;
	struct aga_snddev* snd;

	(void) self;

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;
	if(!(snd = aga_getscriptptr(AGA_SCRIPT_SNDDEV))) return 0;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(RuntimeError, "putclip() argument must be nativeptr");
		return 0;
	}

	if(opts->audio_enabled) {
		result = aga_putclip(snd, ((struct aga_nativeptr*) arg)->ptr);
		if(aga_script_aferr("aga_putclip", result)) return 0;
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

	if(!arg) {
		err_setstr(RuntimeError, "log() takes one argument");
		return 0;
	}

	if(!(loc = getstringvalue(current_frame->f_code->co_filename))) return 0;

	if(!is_stringobject(arg)) {
		af_size_t i;
		for(i = 0; i < aga_logctx.len; ++i) {
			FILE* s = aga_logctx.targets[i];
			aga_loghdr(s, loc, AGA_NORM);
			printobject(arg, s, 0);
			putc('\n', s);
		}

		INCREF(None);
		return None;
	}

	str = getstringvalue(arg);
	if(!str) return 0;

	aga_log(loc, str);

	INCREF(None);
	return None;
}

static object* agan_yeslight(object* self, object* arg) {
	(void) self, (void) arg;

	glEnable(GL_LIGHTING);
	if(aga_script_glerr("glEnable")) return 0;

	INCREF(None);
	return None;
}

static object* agan_nolight(object* self, object* arg) {
	(void) self, (void) arg;

	glDisable(GL_LIGHTING);
	if(aga_script_glerr("glDisable")) return 0;

	INCREF(None);
	return None;
}

static object* agan_yesfog(object* self, object* arg) {
	(void) self, (void) arg;

	glEnable(GL_FOG);
	if(aga_script_glerr("glEnable")) return 0;

	glFogi(GL_FOG_MODE, GL_EXP);
	if(aga_script_glerr("glFogi")) return 0;

	INCREF(None);
	return None;
}

static object* agan_nofog(object* self, object* arg) {
	(void) self, (void) arg;

	glDisable(GL_FOG);
	if(aga_script_glerr("glDisable")) return 0;

	INCREF(None);
	return None;
}

/*
 * NOTE: Fog params follow the following format:
 * 		 [ density(norm), start, end ]
 */
static object* agan_fogparam(object* self, object* arg) {
	object* v;
	float f;

	(void) self;

	if(!arg || !is_listobject(arg)) {
		err_setstr(RuntimeError, "fogparam() argument must be list");
		return 0;
	}

	if(!(v = getlistitem(arg, 0))) return 0;
	f = (float) getfloatvalue(v);
	if(err_occurred()) return 0;
	glFogf(GL_FOG_DENSITY, f);
	if(aga_script_glerr("glFogf")) return 0;

	if(!(v = getlistitem(arg, 1))) return 0;
	f = (float) getfloatvalue(v);
	if(err_occurred()) return 0;
	glFogf(GL_FOG_START, f);
	if(aga_script_glerr("glFogf")) return 0;

	if(!(v = getlistitem(arg, 2))) return 0;
	f = (float) getfloatvalue(v);
	if(err_occurred()) return 0;
	glFogf(GL_FOG_END, f);

	INCREF(None);
	return None;
}

static object* agan_fogcol(object* self, object* arg) {
	object* v;
	af_size_t i;
	float col[3];

	(void) self;

	if(!arg || !is_listobject(arg)) {
		err_setstr(RuntimeError, "fogcol() argument must be list");
		return 0;
	}

	for(i = 0; i < 3; ++i) {
		if(!(v = getlistitem(arg, i))) return 0;
		col[i] = getfloatvalue(v);
		if(err_occurred()) return 0;
	}

	glFogfv(GL_FOG_COLOR, col);
	if(aga_script_glerr("glFogfv")) return 0;

	INCREF(None);
	return None;
}

#define AGAN_LIGHT_INVAL (~0UL)
/* GL1.0 doesn't use `GL_MAX_LIGHTS'. */
#define AGAN_MAXLIGHT (7)
static af_size_t aga_current_light = AGAN_LIGHT_INVAL;
static af_bool_t aga_light_cap_warned = AF_FALSE;
static af_bool_t aga_light_start_warned = AF_FALSE;

static object* agan_startlight(object* self, object* arg) {
	af_size_t i;

	(void) self, (void) arg;

	for(i = 0; i < AGAN_MAXLIGHT; ++i) {
		glDisable(GL_LIGHT0 + i);
		if(aga_script_glerr("glDisable")) return 0;
	}
	aga_current_light = GL_LIGHT0;

	INCREF(None);
	return None;
}

static object* agan_mklight(object* self, object* arg) {
	object* retval;

	(void) self, (void) arg;

	if(aga_current_light == AGAN_LIGHT_INVAL) {
		if(!aga_light_start_warned) {
			aga_log(__FILE__, "warn: mklight() called before startlight()");
			aga_light_start_warned = !aga_light_start_warned;
		}
		INCREF(None);
		return None;
	}

	glEnable(aga_current_light);
	if(aga_script_glerr("glEnable")) return 0;

	retval = newintobject((long) aga_current_light);
	if(!retval) return 0;

	if(++aga_current_light >= GL_LIGHT0 + AGAN_MAXLIGHT) {
		if(!aga_light_cap_warned) {
			aga_log(__FILE__, "warn: light cap (%i) reached", AGAN_MAXLIGHT);
			aga_light_cap_warned = !aga_light_cap_warned;
		}
		aga_current_light = AGAN_LIGHT_INVAL;
		INCREF(None);
		return None;
	}

	return retval;
}

static object* agan_lightpos(object* self, object* arg) {
	static const float pos[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	object* t;
	object* v;

	long light;

	(void) self;

	if(!arg || !is_tupleobject(arg) ||
	   !(v = gettupleitem(arg, 0)) || !is_intobject(v) ||
	   !(t = gettupleitem(arg, 1)) || !is_classmemberobject(t)) {

		err_setstr(
			RuntimeError,
			"lightpos() arguments must be int and transform");
		return 0;
	}

	light = getintvalue(v);
	if(err_occurred()) return 0;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if(!agan_settransmat(t)) return 0;

	glLightfv(light, GL_POSITION, pos);
	if(aga_script_glerr("glLightfv")) return 0;

	glPopMatrix();

	INCREF(None);
	return None;
}

/*
 * NOTE: Light params follow the following format:
 * 		 [ exp(0-128), const atten(norm), lin atten(norm), quad atten(norm) ]
 */
static object* agan_lightparam(object* self, object* arg) {
	object* l;

	object* o;
	long light;

	object* v;
	float f;
	(void) self;

	if(!arg || !is_tupleobject(arg) ||
		!(o = gettupleitem(arg, 0)) || !is_intobject(o) ||
		!(l = gettupleitem(arg, 1)) || !is_listobject(l)) {

		err_setstr(
			RuntimeError,
			"lightparam() arguments must be int and list");
		return 0;
	}

	light = getintvalue(o);
	if(err_occurred()) return 0;

	if(light != GL_LIGHT0) {
		float diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glLightfv(light, GL_DIFFUSE, diffuse);
		if(aga_script_glerr("glLightf")) return 0;
	}

	if(!(v = getlistitem(l, 0))) return 0;
	f = (float) getfloatvalue(v);
	if(err_occurred()) return 0;
	glLightf(light, GL_SPOT_EXPONENT, f);
	if(aga_script_glerr("glLightf")) return 0;

	if(!(v = getlistitem(l, 1))) return 0;
	f = (float) getfloatvalue(v);
	if(err_occurred()) return 0;
	glLightf(light, GL_CONSTANT_ATTENUATION, f);
	if(aga_script_glerr("glLightf")) return 0;

	if(!(v = getlistitem(l, 2))) return 0;
	f = (float) getfloatvalue(v);
	if(err_occurred()) return 0;
	glLightf(light, GL_LINEAR_ATTENUATION, f);
	if(aga_script_glerr("glLightf")) return 0;

	if(!(v = getlistitem(l, 3))) return 0;
	f = (float) getfloatvalue(v);
	if(err_occurred()) return 0;
	glLightf(light, GL_QUADRATIC_ATTENUATION, f);
	if(aga_script_glerr("glLightf")) return 0;

	INCREF(None);
	return None;
}

struct agan_object {
	object* transform;

	object* modelfile;
	object* model;

	af_bool_t unlit;
	af_bool_t scaletex;
	af_bool_t filter;

	object* tex;

	object* modelpath;
	object* texpath;

	af_uint_t drawlist;
};

static object* agan_putobj(object* self, object* arg);
static object* agan_mkobj(object* self, object* arg) {
	struct aga_nativeptr* retval;
	struct agan_object* obj;
	const char* path;
	enum af_err result;
	object* trans;

	(void) self;

	if(!arg || !is_stringobject(arg)) {
		err_setstr(RuntimeError, "mkobj() argument must be string");
		return 0;
	}

	if(!(path = getstringvalue(arg))) return 0;

	retval = NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
	if(!retval) return 0;

	if(!(retval->ptr = calloc(1, sizeof(struct agan_object))))
		return err_nomem();

	obj = retval->ptr;

	if(!(trans = dictlookup(aga_dict, "transform"))) return 0;

	if(!(obj->transform = newclassmemberobject(trans))) {
		return 0;
	}
	{
		struct aga_scriptclass class;
		struct aga_scriptinst inst;
		inst.class = &class;
		inst.object = obj->transform;
		class.class = trans;

		result = aga_instcall(&inst, "create");
		if(aga_script_aferr("aga_instcall", result)) return 0;
	}

	{
		struct aga_conf_node conf;
		struct aga_conf_node* item;
		struct aga_conf_node* v;
		const char* str;
		object* strobj;

		result = aga_mkconf(path, &conf);
		if(aga_script_aferr("aga_mkconf", result)) return 0;

		for(item = conf.children->children;
			item < conf.children->children + conf.children->len;
			++item) {

			object* aspect = 0;
			object* flobj;
			int unlit;
			int scaletex;
			int filter;

			/*
			 * TODO: Handle materials.
			 */
			if(aga_confvar("Model", item, AGA_STRING, &str)) {
				object* modelcache;
				object* filecache;
				object* lookup;
				if(!(modelcache = dictlookup(agan_dict, "modelcache"))) {
					return 0;
				}
				if(!(filecache = dictlookup(agan_dict, "filecache"))) return 0;

				if(!(strobj = newstringobject((char*) str))) return 0;
				obj->modelpath = strobj;

				if(!filecache ||
					!(lookup = dictlookup(filecache, (char*) str))) {

					if(!(obj->modelfile = agan_mklargefile(0, strobj)))
						return 0;
					if(dictinsert(
						filecache, (char*) str, obj->modelfile) == -1) {

						return 0;
					}
				}
				else obj->modelfile = lookup;

				if(!modelcache ||
					!(lookup = dictlookup(modelcache, (char*) str))) {

					if(!(obj->model = agan_mkvertbuf(0, obj->modelfile)))
						return 0;
					if(dictinsert(modelcache, (char*) str, obj->model) == -1)
						return 0;
				}
				else obj->model = lookup;
			}
			if(aga_confvar("Texture", item, AGA_STRING, &str)) {
				object* texcache;
				object* lookup;

				if(!(texcache = dictlookup(agan_dict, "texcache"))) return 0;

				if(!(strobj = newstringobject((char*) str))) return 0;
				obj->texpath = strobj;

				if(!texcache ||
					!(lookup = dictlookup(texcache, (char*) str))) {

					object* filter_obj = obj->filter ? True : False;
					object* call = newtupleobject(2);
					if(!call) return 0;

					if(settupleitem(call, 0, strobj) == -1) return 0;
					if(settupleitem(call, 1, filter_obj) == -1)
						return 0;

					if(!(obj->tex = agan_mkteximg(0, call))) return 0;

					if(dictinsert(texcache, (char*) str, obj->tex) == -1)
						return 0;
				}
				else obj->tex = lookup;
			}
			if(aga_confvar("Unlit", item, AGA_INTEGER, &unlit))
				obj->unlit = !!unlit;
			if(aga_confvar("ScaleTex", item, AGA_INTEGER, &scaletex))
				obj->scaletex = !!scaletex;
			if(aga_confvar("Filter", item, AGA_INTEGER, &filter))
				obj->filter = !!filter;

			if(af_streql(item->name, "Position")) {
				if(!(aspect = getattr(obj->transform, "pos"))) return 0;
			}
			if(af_streql(item->name, "Rotation")) {
				if(!(aspect = getattr(obj->transform, "rot"))) return 0;
			}
			if(af_streql(item->name, "Scale")) {
				if(!(aspect = getattr(obj->transform, "scale"))) return 0;
			}

			if(aspect) {
				for(v = item->children; v < item->children + item->len; ++v) {
					float f;
					if(aga_confvar("X", v, AGA_FLOAT, &f)) {
						if(!(flobj = newfloatobject(f))) return 0;
						if(setlistitem(aspect, 0, flobj) == -1) return 0;
					}
					if(aga_confvar("Y", v, AGA_FLOAT, &f)) {
						if(!(flobj = newfloatobject(f))) return 0;
						if(setlistitem(aspect, 1, flobj) == -1) return 0;
					}
					if(aga_confvar("Z", v, AGA_FLOAT, &f)) {
						if(!(flobj = newfloatobject(f))) return 0;
						if(setlistitem(aspect, 2, flobj) == -1) return 0;
					}
				}
			}
		}

		result = aga_killconf(&conf);
		if(aga_script_aferr("aga_killconf", result)) return 0;
	}

	obj->drawlist = glGenLists(1);
	glNewList(obj->drawlist, GL_COMPILE);
		if(!agan_putobj(0, (void*) retval)) return 0;
	glEndList();
	if(aga_script_glerr("glNewList")) return 0;

	return (object*) retval;
}

static object* agan_putobj_quick(object* self, object* arg) {
	struct aga_nativeptr* nativeptr;
	struct agan_object* obj;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(RuntimeError, "putobj() argument must be nativeptr");
		return 0;
	}

	nativeptr = (struct aga_nativeptr*) arg;

	obj = nativeptr->ptr;

	glCallList(obj->drawlist);
	if(aga_script_glerr("glCallList")) return 0;

	INCREF(None);
	return None;
}

static object* agan_putobj(object* self, object* arg) {
	struct aga_nativeptr* nativeptr;
	struct agan_object* obj;
	object* call_tuple;
	object* prim;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(RuntimeError, "putobj() argument must be nativeptr");
		return 0;
	}

	nativeptr = (struct aga_nativeptr*) arg;

	obj = nativeptr->ptr;

	if(!agan_settex(0, obj->tex)) return 0;

	if(!(call_tuple = newtupleobject(3))) return 0;
	if(settupleitem(call_tuple, 0, obj->model) == -1) return 0;
	if(!(prim = newintobject(AF_TRIANGLES))) return 0;
	if(settupleitem(call_tuple, 1, prim) == -1) return 0;
	if(settupleitem(call_tuple, 2, obj->transform) == -1) return 0;
	if(obj->unlit) {
		if(!agan_nolight(0, 0)) return 0;
		if(!agan_nofog(0, 0)) return 0;
	}
	else {
		if(!agan_yeslight(0, 0)) return 0;
		if(!agan_yesfog(0, 0)) return 0;
	}

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	if(obj->scaletex && !agan_settransmat(obj->transform)) return 0;

	if(!agan_drawbuf(0, call_tuple)) return 0;

	INCREF(None);
	return None;
}

static object* agan_killobj(object* self, object* arg) {
	struct aga_nativeptr* nativeptr;
	struct agan_object* obj;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(RuntimeError, "killobj() argument must be nativeptr");
		return 0;
	}

	nativeptr = (struct aga_nativeptr*) arg;
	obj = nativeptr->ptr;

	glDeleteLists(obj->drawlist, 1);
	if(aga_script_glerr("glDeleteLists")) return 0;

	DECREF(obj->transform);
	DECREF(obj->modelfile);
	DECREF(obj->model);
	DECREF(obj->tex);
	DECREF(obj->modelpath);
	DECREF(obj->texpath);

	INCREF(None);
	return None;
}

static object* agan_objtrans(object* self, object* arg) {
	struct aga_nativeptr* nativeptr;
	struct agan_object* obj;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(RuntimeError, "objtrans() argument must me nativeptr");
		return 0;
	}

	nativeptr = (struct aga_nativeptr*) arg;
	obj = nativeptr->ptr;

	INCREF(obj->transform);
	return obj->transform;
}

static af_bool_t agan_dumpputs(FILE* f, const char* fmt, ...) {
	va_list l;
	va_start(l, fmt);

	if(vfprintf(f, fmt, l) == EOF) {
		aga_af_errno(__FILE__, "vfprintf");
		return AF_TRUE;
	}

	va_end(l);

	return AF_FALSE;
}

static object* agan_dumpobj(object* self, object* arg) {
	struct aga_nativeptr* nativeptr;
	struct agan_object* obj;
	object* path;
	const char* s;
	FILE* f;
	const char* aspects[] = { "Position", "Rotation", "Scale" };
	const char* pyaspects[] = { "pos", "rot", "scale" };
	af_size_t i;

	(void) self;

	if(!arg || !is_tupleobject(arg) ||
		!(nativeptr = (struct aga_nativeptr*) gettupleitem(arg, 0)) ||
		nativeptr->ob_type != &aga_nativeptr_type ||
		!(path = gettupleitem(arg, 1)) || !is_stringobject(path)) {

		err_setstr(
			RuntimeError, "dumpobj() arguments must be nativeptr and string");
		return 0;
	}

	obj = nativeptr->ptr;

	if(!(s = getstringvalue(path))) return 0;
	if(!(f = fopen(s, "w+"))) {
		aga_af_patherrno(__FILE__, "fopen", s);
		return 0;
	}

	if(agan_dumpputs(f, "<root>\n")) return 0;
	if(!(s = getstringvalue(obj->modelpath))) return 0;
	if(agan_dumpputs(
		f, "\t<item name=\"Model\" type=\"String\">%s</item>\n", s)) return 0;
	if(!(s = getstringvalue(obj->texpath))) return 0;
	if(agan_dumpputs(
		f, "\t<item name=\"Texture\" type=\"String\">%s</item>\n", s)) {

		return 0;
	}
	if(agan_dumpputs(
		f,
		"\t<item name=\"Unlit\" type=\"Integer\">%i</item>\n", obj->unlit)) {

		return 0;
	}
	if(agan_dumpputs(
		f, "\t<item name=\"ScaleTex\" type=\"Integer\">%i</item>\n",
		obj->scaletex)) {

		return 0;
	}

	if(agan_dumpputs(
		f, "\t<item name=\"Filter\" type=\"Integer\">%i</item>\n",
		obj->filter)) {

		return 0;
	}

	for(i = 0; i < AF_ARRLEN(aspects); ++i) {
		object* attr;
		af_size_t j;
		if(agan_dumpputs(f, "\t<item name=\"%s\">\n", aspects[i])) return 0;

		if(!(attr = getattr(obj->transform, (char*) pyaspects[i]))) return 0;
		for(j = 0; j < 3; ++j) {
			object* flobj;
			float fl;
			if(!(flobj = getlistitem(attr, j))) return 0;
			fl = getfloatvalue(flobj);
			if(err_occurred()) return 0;

			if(agan_dumpputs(
				f,
				"\t\t<item name=\"%c\" type=\"Float\">%f</item>\n",
				"XYZ"[j], fl)) {

				return 0;
			}
		}

		if(agan_dumpputs(f, "\t</item>\n")) return 0;
	}
	if(agan_dumpputs(f, "</root>\n")) return 0;

	if(fclose(f) == EOF) {
		aga_af_errno(__FILE__, "fclose");
		return 0;
	}

	INCREF(None);
	return None;
}

static object* agan_text(object* self, object* arg) {
	object* str;
	const char* text;
	object* t;
	object* f;
	float x, y;

	(void) self;

	if(!arg || !is_tupleobject(arg) ||
	   !(str = gettupleitem(arg, 0)) || !is_stringobject(str) ||
	   !(t = gettupleitem(arg, 1)) || !is_listobject(t)) {

		err_setstr(
			RuntimeError, "text() arguments must be string and list");
		return 0;
	}

	if(!(text = getstringvalue(str))) return 0;

	if(!(f = getlistitem(t, 0))) return 0;
	x = (float) getfloatvalue(f);
	if(err_occurred()) return 0;
	if(!(f = getlistitem(t, 1))) return 0;
	y = (float) getfloatvalue(f);
	if(err_occurred()) return 0;

	if(aga_script_aferr("aga_puttext", aga_puttext(x, y, text)))
		return 0;

	INCREF(None);
	return None;
}

static object* agan_glabi(object* self, object* arg) {
	(void) self;
	(void) arg;

#ifdef AF_GLXABI
	return newstringobject("x");
#elif defined(AF_WGL)
	return newstringobject("w");
#endif

	return 0;
}

static object* agan_clear(object* self, object* arg) {
	object* v;
	float col[4];
	af_size_t i;
	enum af_err result;

	struct af_ctx* af;

	(void) self;

	if(!(af = aga_getscriptptr(AGA_SCRIPT_AFCTX))) return 0;

	if(!arg || !is_listobject(arg)) {
		err_setstr(RuntimeError, "clear() argument must be list");
		return 0;
	}

	for(i = 0; i < 4; ++i) {
		if(!(v = getlistitem(arg, i))) return 0;
		col[i] = (float) getfloatvalue(v);
		if(err_occurred()) return 0;
	}

	result = af_clear(af, col);
	if(aga_script_aferr("af_clear", result)) return 0;

	INCREF(None);
	return None;
}

/* Python lacks native bitwise ops @-@ */
static object* agan_bitand(object* self, object* arg) {
	object* a;
	object* b;

	long av, bv;

	(void) self;

	if(!arg || !is_tupleobject(arg) ||
	   !(a = gettupleitem(arg, 0)) || !is_intobject(a) ||
	   !(b = gettupleitem(arg, 1)) || !is_intobject(b)) {

		err_setstr(RuntimeError, "bitand() arguments must be int and int");
		return 0;
	}

	av = getintvalue(a);
	if(err_occurred()) return 0;

	bv = getintvalue(b);
	if(err_occurred()) return 0;

	return newintobject(av & bv);
}

static object* agan_bitshl(object* self, object* arg) {
	object* a;
	object* b;

	long av, bv;

	(void) self;

	if(!arg || !is_tupleobject(arg) ||
	   !(a = gettupleitem(arg, 0)) || !is_intobject(a) ||
	   !(b = gettupleitem(arg, 1)) || !is_intobject(b)) {

		err_setstr(RuntimeError, "bitshl() arguments must be int and int");
		return 0;
	}

	av = getintvalue(a);
	if(err_occurred()) return 0;

	bv = getintvalue(b);
	if(err_occurred()) return 0;

	return newintobject(av << bv);
}

static object* agan_getobjmeta(object* self, object* arg) {
	object* ret;
	object* v;
	struct agan_object* obj;

	(void) self;

	if(!arg || arg->ob_type != &aga_nativeptr_type) {
		err_setstr(RuntimeError, "getobjmeta() argument must be nativeptr");
		return 0;
	}

	obj = ((struct aga_nativeptr*) arg)->ptr;

	if(!(ret = newdictobject())) return 0;

	if(dictinsert(ret, "transform", obj->transform) == -1) return 0;
	if(dictinsert(ret, "modelfile", obj->modelfile) == -1) return 0;
	if(dictinsert(ret, "model", obj->model) == -1) return 0;
	if(dictinsert(ret, "tex", obj->tex) == -1) return 0;
	if(dictinsert(ret, "modelpath", obj->modelpath) == -1) return 0;
	if(dictinsert(ret, "texpath", obj->texpath) == -1) return 0;

	INCREF(v = obj->unlit ? True : False);
	if(dictinsert(ret, "unlit", v) == -1) return 0;
	INCREF(v = obj->scaletex ? True : False);
	if(dictinsert(ret, "scaletex", v) == -1) return 0;
	INCREF(v = obj->filter ? True : False);
	if(dictinsert(ret, "filter", v) == -1) return 0;

	if(!(v = newintobject(obj->drawlist))) return 0;
	if(dictinsert(ret, "drawlist", v) == -1) return 0;

	return ret;
}

static object* agan_randnorm(object* self, object* arg) {
	(void) self;
	(void) arg;

	return newfloatobject((double) rand() / (double) RAND_MAX);
}

static object* agan_die(object* self, object* arg) {
	af_bool_t* die;

	(void) self;
	(void) arg;

	if(!(die = aga_getscriptptr(AGA_SCRIPT_DIE))) return 0;

	*die = AF_TRUE;

	INCREF(None);
	return None;
}

static object* agan_setcursor(object* self, object* arg) {
	enum af_err result;

	object* o;
	object* v;

	af_bool_t visible, captured;

	union aga_winenv* env;
	struct aga_win* win;

	if(!(env = aga_getscriptptr(AGA_SCRIPT_WINENV))) return 0;
	if(!(win = aga_getscriptptr(AGA_SCRIPT_WIN))) return 0;

	(void) self;

	if(!arg || !is_tupleobject(arg) ||
		!(o = gettupleitem(arg, 0)) || !is_intobject(o) ||
		!(v = gettupleitem(arg, 1)) || !is_intobject(v)) {

		err_setstr(RuntimeError, "setcursor() arguments must be int and int");
		return 0;
	}

	visible = !!getintvalue(o);
	if(err_occurred()) return 0;

	captured = !!getintvalue(v);
	if(err_occurred()) return 0;

	result = aga_setcursor(env, win, visible, captured);
	if(aga_script_aferr("aga_setcursor", result)) return 0;

	INCREF(None);
	return None;
}

enum af_err aga_mkmod(object** dict) {
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
		{ "startlight", agan_startlight },
		{ "mklight", agan_mklight },
		{ "lightpos", agan_lightpos },
		{ "lightparam", agan_lightparam },
		{ "mkobj", agan_mkobj },
		{ "putobj", agan_putobj_quick },
		{ "killobj", agan_killobj },
		{ "dumpobj", agan_dumpobj },
		{ "objtrans", agan_objtrans },
		{ "text", agan_text },
		{ "glabi", agan_glabi },
		{ "yesfog", agan_yesfog },
		{ "nofog", agan_nofog },
		{ "fogparam", agan_fogparam },
		{ "fogcol", agan_fogcol },
		{ "clear", agan_clear },
		{ "bitand", agan_bitand },
		{ "bitshl", agan_bitshl },
		{ "getobjmeta", agan_getobjmeta },
		{ "randnorm", agan_randnorm },
		{ "die", agan_die },
		{ "setcursor", agan_setcursor },
		{ 0, 0 }
	};

	object* module = initmodule("agan", methods);
	object* texcache;
	object* modelcache;
	object* filecache;
	AF_VERIFY(module, AF_ERR_UNKNOWN);

	if(!(texcache = newdictobject())) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	if(!(modelcache = newdictobject())) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	if(!(filecache = newdictobject())) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	if(!(agan_dict = getmoduledict(module))) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	*dict = agan_dict;

	if(dictinsert(agan_dict, "texcache", texcache) == -1) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	if(dictinsert(agan_dict, "modelcache", modelcache) == -1) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	if(dictinsert(agan_dict, "filecache", filecache) == -1) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	return AF_ERR_NONE;
}

enum af_err aga_killmod(void) {
	return AF_ERR_NONE;
}

#endif
