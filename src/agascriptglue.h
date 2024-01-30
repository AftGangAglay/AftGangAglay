/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_GLUE_H
#define AGA_SCRIPT_GLUE_H

/* TODO: Object/scene-based lighting. */
#define AGAN_LIGHT_INVAL (~0UL)
/* GL1.0 doesn't use `GL_MAX_LIGHTS'. */
#define AGAN_MAXLIGHT (7)
static af_size_t aga_current_light = AGAN_LIGHT_INVAL;
static af_bool_t aga_light_cap_warned = AF_FALSE;
static af_bool_t aga_light_start_warned = AF_FALSE;

struct agan_object {
	aga_pyobject_t transform;
	af_uint_t drawlist;
};

static const char* agan_trans_components[] = {
	"pos", "rot", "scale" };

static const char* agan_conf_components[] = {
	"Position", "Rotation", "Scale" };

static const char* agan_xyz[] = { "X", "Y", "Z" };

static af_bool_t agan_settransmat(aga_pyobject_t trans, af_bool_t inv) {
	aga_pyobject_t comp, xo, yo, zo;
	float x, y, z;
	af_size_t i;

	for(i = inv ? 2 : 0; i < 3; inv ? --i : ++i) {
		if(!(comp = dictlookup(trans, (char*) agan_trans_components[i]))) {
			return 0;
		}

		AGA_GETLISTITEM(comp, 0, xo);
		AGA_GETLISTITEM(comp, 1, yo);
		AGA_GETLISTITEM(comp, 2, zo);

		AGA_SCRIPTVAL(x, xo, float);
		AGA_SCRIPTVAL(y, yo, float);
		AGA_SCRIPTVAL(z, zo, float);

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

AGA_SCRIPTPROC(mktrans) {
	aga_pyobject_t new;
	af_size_t i, j;

	if(!(new = newdictobject())) return 0;

	for(i = 0; i < AF_ARRLEN(agan_conf_components); ++i) {
		const char* s = agan_trans_components[i];
		aga_pyobject_t p = newlistobject(3);
		if(!p) return 0;

		for(j = 0; j < 3; ++j) {
			aga_pyobject_t flobj;
			AGA_NEWOBJ(flobj, float, (i == 2 ? 1.0 : 0.0));
			AGA_SETLISTITEM(p, j, flobj);
		}
		if(dictinsert(new, (char*) s, p)) return 0;
	}

	return new;
}

AGA_SCRIPTPROC(getkey) {
	long value;
	aga_pyobject_t retval = None;
	struct aga_keymap* keymap;

	if(!(keymap = aga_getscriptptr(AGA_SCRIPT_KEYMAP))) return 0;

	if(!AGA_ARGLIST(int)) AGA_ARGERR("getkey", "int");

	AGA_SCRIPTVAL(value, arg, int);

	if(keymap->keystates) {
		if(value < keymap->keysyms_per_keycode * keymap->keycode_len) {
			retval = keymap->keystates[value] ? True : False;
		}
	}

	INCREF(retval);
	return retval;
}

AGA_SCRIPTPROC(getmotion) {
	aga_pyobject_t retval;
	aga_pyobject_t x, y;
	struct aga_pointer* pointer;

	if(!(pointer = aga_getscriptptr(AGA_SCRIPT_POINTER))) return 0;

	AGA_NEWOBJ(retval, list, (2));

	AGA_NEWOBJ(x, float, (pointer->dx));
	AGA_NEWOBJ(y, float, (pointer->dy));

	AGA_SETLISTITEM(retval, 0, x);
	AGA_SETLISTITEM(retval, 1, y);

	return retval;
}

AGA_SCRIPTPROC(setcam) {
	aga_pyobject_t t, mode;
	af_bool_t b;
	double ar;

	struct aga_opts* opts;

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(t, 0, dict) ||
		!AGA_ARG(mode, 1, int)) {

		AGA_ARGERR("setcam", "dict and int");
	}

	AGA_SCRIPTBOOL(b, mode);

	ar = (double) opts->height / (double) opts->width;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if(b) gluPerspective(opts->fov, 1.0 / ar, 0.1, 10000.0);
	else glOrtho(-1.0, 1.0, -ar, ar, 0.001, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if(!agan_settransmat(t, AF_TRUE)) return 0;

	AGA_NONERET;
}

AGA_SCRIPTPROC(getconf) {
	struct aga_conf_node* node;
	const char** names;
	af_size_t i, len;
	aga_pyobject_t v;

	struct aga_opts* opts;

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!AGA_ARGLIST(list)) AGA_ARGERR("getconf", "list");

	if(!(names = malloc(sizeof(char*) * (len = getlistsize(arg)))))
		return err_nomem();

	for(i = 0; i < len; ++i) {
		AGA_GETLISTITEM(arg, i, v);
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
		case AGA_NONE: AGA_NONERET;
		case AGA_STRING: return newstringobject(node->data.string);
		case AGA_INTEGER: return newintobject(node->data.integer);
		case AGA_FLOAT: return newfloatobject(node->data.flt);
	}
}

AGA_SCRIPTPROC(log) {
	const char* str;
	const char* loc;

	if(!arg) {
		err_setstr(RuntimeError, "log() takes one argument");
		return 0;
	}

	AGA_SCRIPTVAL(loc, current_frame->f_code->co_filename, string);

	if(!is_stringobject(arg)) {
		af_size_t i;
		for(i = 0; i < aga_logctx.len; ++i) {
			FILE* s = aga_logctx.targets[i];
			aga_loghdr(s, loc, AGA_NORM);
			printobject(arg, s, 0);
			putc('\n', s);
		}

		AGA_NONERET;
	}

	AGA_SCRIPTVAL(str, arg, string);

	aga_log(loc, str);

	AGA_NONERET;
}

/*
 * NOTE: Fog params follow the following format:
 * 		 [ density(norm), start, end ]
 */
AGA_SCRIPTPROC(fogparam) {
	aga_pyobject_t v;
	float f;

	if(!AGA_ARGLIST(list)) AGA_ARGERR("fogparam", "list");

	glFogi(GL_FOG_MODE, GL_EXP);
	if(aga_script_glerr("glFogi")) return 0;

	AGA_GETLISTITEM(arg, 0, v);
	AGA_SCRIPTVAL(f, v, float);

	glFogf(GL_FOG_DENSITY, f);
	if(aga_script_glerr("glFogf")) return 0;

	AGA_GETLISTITEM(arg, 1, v);
	AGA_SCRIPTVAL(f, v, float);

	glFogf(GL_FOG_START, f);
	if(aga_script_glerr("glFogf")) return 0;

	AGA_GETLISTITEM(arg, 2, v);
	AGA_SCRIPTVAL(f, v, float);

	glFogf(GL_FOG_END, f);
	if(aga_script_glerr("glFogf")) return 0;

	AGA_NONERET;
}

AGA_SCRIPTPROC(fogcol) {
	aga_pyobject_t v;
	af_size_t i;
	float col[3];

	if(!AGA_ARGLIST(list)) AGA_ARGERR("fogcol", "list");

	for(i = 0; i < 3; ++i) {
		AGA_GETLISTITEM(arg, i, v);
		AGA_SCRIPTVAL(col[i], v, float);
	}

	glFogfv(GL_FOG_COLOR, col);
	if(aga_script_glerr("glFogfv")) return 0;

	AGA_NONERET;
}

AGA_SCRIPTPROC(startlight) {
	af_size_t i;

	for(i = 0; i < AGAN_MAXLIGHT; ++i) {
		glDisable(GL_LIGHT0 + i);
		if(aga_script_glerr("glDisable")) return 0;
	}
	aga_current_light = GL_LIGHT0;

	AGA_NONERET;
}

AGA_SCRIPTPROC(mklight) {
	aga_pyobject_t retval;

	if(aga_current_light == AGAN_LIGHT_INVAL) {
		if(!aga_light_start_warned) {
			aga_log(__FILE__, "warn: mklight() called before startlight()");
			aga_light_start_warned = !aga_light_start_warned;
		}

		AGA_NONERET;
	}

	glEnable(aga_current_light);
	if(aga_script_glerr("glEnable")) return 0;

	AGA_NEWOBJ(retval, int, ((long) aga_current_light));

	if(++aga_current_light >= GL_LIGHT0 + AGAN_MAXLIGHT) {
		if(!aga_light_cap_warned) {
			aga_log(__FILE__, "warn: light cap (%i) reached", AGAN_MAXLIGHT);
			aga_light_cap_warned = !aga_light_cap_warned;
		}
		aga_current_light = AGAN_LIGHT_INVAL;

		AGA_NONERET;
	}

	return retval;
}

AGA_SCRIPTPROC(lightpos) {
	static const float pos[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	aga_pyobject_t t;
	aga_pyobject_t v;

	long light;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(v, 0, int) ||
		!AGA_ARG(t, 1, dict)) {

		AGA_ARGERR("lightpos", "int and dict");
	}

	AGA_SCRIPTVAL(light, v, int);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if(!agan_settransmat(t, AF_FALSE)) return 0;

	glLightfv(light, GL_POSITION, pos);
	if(aga_script_glerr("glLightfv")) return 0;

	glPopMatrix();

	AGA_NONERET;
}

/*
 * NOTE: Light params follow the following format:
 * 		 [ exp(0-128), const atten(norm), lin atten(norm), quad atten(norm) ]
 */
AGA_SCRIPTPROC(lightparam) {
	aga_pyobject_t l, o, v;

	long light;
	float f;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(o, 0, int) || !AGA_ARG(l, 1, list)) {
		AGA_ARGERR("lightparam", "int and list");
	}

	AGA_SCRIPTVAL(light, o, int);

	if(light != GL_LIGHT0) {
		static const float diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glLightfv(light, GL_DIFFUSE, diffuse);
		if(aga_script_glerr("glLightf")) return 0;
	}

	AGA_GETLISTITEM(l, 0, v);
	AGA_SCRIPTVAL(f, v, float);

	glLightf(light, GL_SPOT_EXPONENT, f);
	if(aga_script_glerr("glLightf")) return 0;

	AGA_GETLISTITEM(l, 1, v);
	AGA_SCRIPTVAL(f, v, float);

	glLightf(light, GL_CONSTANT_ATTENUATION, f);
	if(aga_script_glerr("glLightf")) return 0;

	AGA_GETLISTITEM(l, 2, v);
	AGA_SCRIPTVAL(f, v, float);

	glLightf(light, GL_LINEAR_ATTENUATION, f);
	if(aga_script_glerr("glLightf")) return 0;

	AGA_GETLISTITEM(l, 3, v);
	AGA_SCRIPTVAL(f, v, float);

	glLightf(light, GL_QUADRATIC_ATTENUATION, f);
	if(aga_script_glerr("glLightf")) return 0;

	AGA_NONERET;
}

/*
 * TODO: Failure states here are super leaky - we can probably compartmentalise
 * 		 This function a lot more to help remedy this.
 */
AGA_SCRIPTPROC(mkobj) {
	enum af_err result;

	af_size_t i;
	aga_pyobject_t retval;

	struct agan_object* obj;
	struct aga_nativeptr* nativeptr;

	struct af_buf model, tex;
	int unlit = 0, scaletex = 0, filter = 0;

	struct aga_respack* pack;
	struct af_ctx* af;
	struct af_vert* vert;

	const char* path;

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;
	if(!(af = aga_getscriptptr(AGA_SCRIPT_AFCTX))) return 0;
	if(!(vert = aga_getscriptptr(AGA_SCRIPT_AFVERT))) return 0;

	if(!AGA_ARGLIST(string)) AGA_ARGERR("mkobj", "string");

	AGA_SCRIPTVAL(path, arg, string);

	AGA_NEWOBJ(retval, nativeptr, ());
	nativeptr = (struct aga_nativeptr*) retval;

	if(!(nativeptr->ptr = calloc(1, sizeof(struct agan_object))))
		return err_nomem();

	obj = nativeptr->ptr;
	if(!(obj->transform = newdictobject())) return 0;

	{
		struct aga_conf_node conf;
		struct aga_conf_node* root;
		struct aga_conf_node* it;
		struct aga_conf_node* v;
		const char* str;
		void* conf_fp;
		af_size_t conf_size;
		struct aga_res* res;

		result = aga_resfptr(pack, path, &conf_fp, &conf_size);
		if(aga_script_aferr("aga_resfptr", result)) return 0;

		aga_conf_debug_file = path;
		result = aga_mkconf(conf_fp, conf_size, &conf);
		if(aga_script_aferr("aga_mkconf", result)) return 0;

		root = conf.children;
		for(it = root->children; it < root->children + root->len; ++it) {
			aga_pyobject_t flobj;

			/*
			 * TODO: Handle materials.
			 */
			if(aga_confvar("Model", it, AGA_STRING, &str)) {
				result = aga_mkres(pack, str, &res);
				if(aga_script_aferr("aga_mkres", result)) return 0;

				result = aga_releaseres(res);
				if(aga_script_aferr("aga_releaseres", result)) return 0;

				result = af_mkbuf(af, &model, AF_BUF_VERT);
				if(aga_script_aferr("af_mkbuf", result)) return 0;

				result = af_upload(af, &model, res->data, res->size);
				if(aga_script_aferr("af_upload", result)) return 0;

				continue;
			}

			if(aga_confvar("Texture", it, AGA_STRING, &str)) {
                static const char* width_path = "Width";
                int width;

                /* TODO: Fix leaky error states here once we remove "img". */
				result = aga_mkres(pack, str, &res);
				if(aga_script_aferr("aga_mkres", result)) return 0;

				result = aga_releaseres(res);
				if(aga_script_aferr("aga_releaseres", result)) return 0;

				/* NOTE: Filter mode must appear above texture entry. */
				result = af_mkbuf(af, &tex, AF_BUF_TEX);
				if(aga_script_aferr("af_mkbuf", result)) return 0;

                result = aga_conftree_nonroot(
                    res->conf, &width_path, 1, &width, AGA_INTEGER);
                if(aga_script_aferr("aga_conftree_nonroot", result)) return 0;

                tex.tex_width = width;
                tex.tex_filter = filter;

				result = af_upload(af, &tex, res->data, res->size);
				if(aga_script_aferr("af_upload", result)) return 0;

				continue;
			}

			if(aga_confvar("Unlit", it, AGA_INTEGER, &unlit)) {
				unlit = !!unlit;
				continue;
			}

			if(aga_confvar("ScaleTex", it, AGA_INTEGER, &scaletex)) {
				scaletex = !!scaletex;
				continue;
			}

			if(aga_confvar("Filter", it, AGA_INTEGER, &filter)) {
				filter = !!filter;
				continue;
			}

			for(i = 0; i < AF_ARRLEN(agan_conf_components); ++i) {
				if(af_streql(it->name, agan_conf_components[i])) {
					const char* s = agan_trans_components[i];
					aga_pyobject_t p = newlistobject(3);
					if(!p) return 0;

					for(v = it->children; v < it->children + it->len; ++v) {
						for(i = 0; i < AF_ARRLEN(agan_xyz); ++i) {
							const char* c = agan_xyz[i];
							float f;

							if(aga_confvar(c, v, AGA_FLOAT, &f)) {
								AGA_NEWOBJ(flobj, float, (f));
								AGA_SETLISTITEM(p, i, flobj);
							}
						}

						if(dictinsert(obj->transform, (char*) s, p)) return 0;
					}

					break;
				}
			}
		}

		result = aga_killconf(&conf);
		if(aga_script_aferr("aga_killconf", result)) return 0;
	}

	obj->drawlist = glGenLists(1);
    if(aga_script_glerr("glGenLists")) return 0;

	glNewList(obj->drawlist, GL_COMPILE);
	{
		result = af_settex(af, &tex);
		if(aga_script_aferr("af_settex", result)) return 0;

		if(unlit) {
			glDisable(GL_LIGHTING);
			if(aga_script_glerr("glDisable")) return 0;

			glDisable(GL_FOG);
			if(aga_script_glerr("glDisable")) return 0;
		}
		else {
			glEnable(GL_LIGHTING);
			if(aga_script_glerr("glEnable")) return 0;

			glEnable(GL_FOG);
			if(aga_script_glerr("glEnable")) return 0;
		}

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		if(scaletex && !agan_settransmat(obj->transform, AF_FALSE)) {
			return 0;
		}

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		if(!agan_settransmat(obj->transform, AF_FALSE)) return 0;

		result = af_drawbuf(af, &model, vert, AF_TRIANGLES);
		if(aga_script_aferr("af_drawbuf", result)) return 0;

		glPopMatrix();
	}
	glEndList();
	if(aga_script_glerr("glNewList")) return 0;

	result = af_killbuf(af, &model);
	if(aga_script_aferr("af_killbuf", result)) return 0;

	result = af_killbuf(af, &tex);
	if(aga_script_aferr("af_killbuf", result)) return 0;

	return (aga_pyobject_t) retval;
}

AGA_SCRIPTPROC(putobj) {
	struct aga_nativeptr* nativeptr;
	struct agan_object* obj;

	if(!AGA_ARGLIST(nativeptr)) AGA_ARGERR("putobj", "nativeptr");

	nativeptr = (struct aga_nativeptr*) arg;
	obj = nativeptr->ptr;

	glCallList(obj->drawlist);
	if(aga_script_glerr("glCallList")) return 0;

	AGA_NONERET;
}

AGA_SCRIPTPROC(killobj) {
	struct aga_nativeptr* nativeptr;
	struct agan_object* obj;

	if(!AGA_ARGLIST(nativeptr)) AGA_ARGERR("killobj", "nativeptr");

	nativeptr = (struct aga_nativeptr*) arg;
	obj = nativeptr->ptr;

	glDeleteLists(obj->drawlist, 1);
	if(aga_script_glerr("glDeleteLists")) return 0;

	DECREF(obj->transform);

	AGA_NONERET;
}

AGA_SCRIPTPROC(text) {
	const char* text;
	aga_pyobject_t str, t, f;
	float x, y;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(str, 0, string) ||
		!AGA_ARG(t, 1, list)) {

		AGA_ARGERR("text", "string and list");
	}

	AGA_SCRIPTVAL(text, str, string);

	AGA_GETLISTITEM(t, 0, f);
	AGA_SCRIPTVAL(x, f, float);

	AGA_GETLISTITEM(t, 1, f);
	AGA_SCRIPTVAL(y, f, float);

	if(aga_script_aferr("aga_puttext", aga_puttext(x, y, text)))
		return 0;

	AGA_NONERET;
}

AGA_SCRIPTPROC(clear) {
	enum af_err result;

	aga_pyobject_t v;
	float col[4];
	af_size_t i;

	struct af_ctx* af;

	if(!(af = aga_getscriptptr(AGA_SCRIPT_AFCTX))) return 0;

	if(!AGA_ARGLIST(list)) AGA_ARGERR("clear", "list");

	for(i = 0; i < AF_ARRLEN(col); ++i) {
		AGA_GETLISTITEM(arg, i, v);
		AGA_SCRIPTVAL(col[i], v, float);
	}

	result = af_clear(af, col);
	if(aga_script_aferr("af_clear", result)) return 0;

	AGA_NONERET;
}

/* Python lacks native bitwise ops @-@ */
AGA_SCRIPTPROC(bitand) {
	aga_pyobject_t a, b;
	long av, bv;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(a, 0, int) || !AGA_ARG(b, 1, int)) {
		AGA_ARGERR("bitand", "int and int");
	}

	AGA_SCRIPTVAL(av, a, int);
	AGA_SCRIPTVAL(bv, b, int);

	return newintobject(av & bv);
}

AGA_SCRIPTPROC(bitshl) {
	aga_pyobject_t a, b;
	long av, bv;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(a, 0, int) || !AGA_ARG(b, 1, int)) {
		AGA_ARGERR("bitshl", "int and int");
	}

	AGA_SCRIPTVAL(av, a, int);
	AGA_SCRIPTVAL(bv, b, int);

	return newintobject(av << bv);
}

AGA_SCRIPTPROC(getobjtrans) {
	struct agan_object* obj;

	if(!AGA_ARGLIST(nativeptr)) AGA_ARGERR("getobjtrans", "nativeptr");

	obj = ((struct aga_nativeptr*) arg)->ptr;

	return obj->transform;
}

AGA_SCRIPTPROC(randnorm) {
	return newfloatobject((double) rand() / (double) RAND_MAX);
}

AGA_SCRIPTPROC(die) {
	af_bool_t* die;
	if(!(die = aga_getscriptptr(AGA_SCRIPT_DIE))) return 0;
	*die = AF_TRUE;

	AGA_NONERET;
}

AGA_SCRIPTPROC(setcursor) {
	enum af_err result;

	aga_pyobject_t o, v;
	af_bool_t visible, captured;

	struct aga_winenv* env;
	struct aga_win* win;

	if(!(env = aga_getscriptptr(AGA_SCRIPT_WINENV))) return 0;
	if(!(win = aga_getscriptptr(AGA_SCRIPT_WIN))) return 0;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(o, 0, int) || !AGA_ARG(v, 1, int)) {
		AGA_ARGERR("setcursor", "int and int");
	}

	AGA_SCRIPTBOOL(visible, o);
	AGA_SCRIPTBOOL(captured, v);

	result = aga_setcursor(env, win, visible, captured);
	if(aga_script_aferr("aga_setcursor", result)) return 0;

	AGA_NONERET;
}

static enum af_err aga_insertfloat(const char* key, double value) {
	aga_pyobject_t o;
	if(!(o = newfloatobject(value))) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	if(dictinsert(agan_dict, (char*) key, o) == -1) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	return AF_ERR_NONE;
}

static enum af_err aga_insertint(const char* key, long value) {
	aga_pyobject_t o;
	if(!(o = newintobject(value))) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}
	if(dictinsert(agan_dict, (char*) key, o) == -1) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	return AF_ERR_NONE;
}

static enum af_err aga_setkeys(void);
enum af_err aga_mkmod(aga_pyobject_t* dict) {
	static const double pi = 3.14159265358979323846;
	static const double rads = pi / 180.0;

	enum af_err result;

#define _(name) { #name, agan_##name }
	struct methodlist methods[] = {
		/* Input */
		_(getkey),
		_(getmotion),
		_(setcursor),

		/* Drawing */
		_(mktrans),
		_(setcam),
		_(text),
		_(fogparam),
		_(fogcol),
		_(clear),

		/* Miscellaneous */
		_(getconf),
		_(log),
		_(die),

		/* Lights */
		_(startlight),
		_(mklight),
		_(lightpos),
		_(lightparam),

		/* Objects */
		_(mkobj),
		_(putobj),
		_(killobj),
		_(getobjtrans),

		/* Maths */
		_(bitand),
		_(bitshl),
		_(randnorm),

		{ 0, 0 }
	};
#undef _

	aga_pyobject_t module = initmodule("agan", methods);
	AF_VERIFY(module, AF_ERR_UNKNOWN);

	if(!(agan_dict = getmoduledict(module))) {
		aga_scripttrace();
		return AF_ERR_UNKNOWN;
	}

	AF_CHK(aga_insertfloat("PI", pi));
	AF_CHK(aga_insertfloat("RADS", rads));

	if((result = aga_setkeys())) {
		aga_scripttrace();
		return result;
	}

	*dict = agan_dict;

	return AF_ERR_NONE;
}

static enum af_err aga_setkeys(void) {
#define _(name, value) AF_CHK(aga_insertint(name, value))
#ifdef AF_GLXABI
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
#elif defined(AF_WGL)
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

	return AF_ERR_NONE;
}

#endif
