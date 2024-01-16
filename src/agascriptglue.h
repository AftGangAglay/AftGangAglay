/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_GLUE_H
#define AGA_SCRIPT_GLUE_H

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
		AGA_GETATTR(trans, agan_trans_components[i], comp);

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

#define AGA_PUTITEM_STR(f, name, value) \
	if(aga_script_aferr("aga_fprintf", aga_fprintf( \
		f, "\t<item name=\"%s\" type=\"String\">%s</item>\n", name, value))) \
        \
		return 0

#define AGA_PUTITEM_INT(f, name, value) \
	if(aga_script_aferr("aga_fprintf", aga_fprintf( \
		f, "\t<item name=\"%s\" type=\"Integer\">%s</item>\n", name, value))) \
		\
		return 0

#define AGA_PUTITEM_FLOAT(f, name, value) \
	if(aga_script_aferr("aga_fprintf", aga_fprintf( \
		f, "\t<item name=\"%s\" type=\"Float\">%s</item>\n", name, value))) \
		\
		return 0

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

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(t, 0, classmember) ||
		!AGA_ARG(mode, 1, int)) {

		AGA_ARGERR("setcam", "transform and int");
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
		!AGA_ARG(t, 1, classmember)) {

		AGA_ARGERR("lightpos", "int and transform");
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
	aga_pyobject_t retval, trans;

	struct agan_object* obj;
	struct aga_nativeptr* nativeptr;

	struct aga_img img;
	struct af_buf model, tex;
	int unlit, scaletex, filter;

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

	if(!(trans = dictlookup(aga_dict, "transform"))) return 0;
	AGA_NEWOBJ(obj->transform, classmember, (trans));

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
		struct aga_conf_node* root;
		struct aga_conf_node* it;
		struct aga_conf_node* v;
		const char* str;
		struct aga_res* res;

		result = aga_mkconf(path, &conf);
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
				/* TODO: Fix leaky error states here once we remove "img". */
				result = aga_mkres(pack, str, &res);
				if(aga_script_aferr("aga_mkres", result)) return 0;

				result = aga_releaseres(res);
				if(aga_script_aferr("aga_releaseres", result)) return 0;

				result = aga_mkimg(&img, res);
				if(aga_script_aferr("aga_mkimg", result)) return 0;

				/* NOTE: Filter mode must appear above texture entry. */
				result = af_mkbuf(af, &tex, AF_BUF_TEX);
				if(aga_script_aferr("af_mkbuf", result)) return 0;

				tex.tex_width = img.width;
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
					aga_pyobject_t p;

					AGA_GETATTR(obj->transform, s, p);
					for(v = it->children; v < it->children + it->len; ++v) {
						for(i = 0; i < AF_ARRLEN(agan_xyz); ++i) {
							const char* c = agan_xyz[i];
							float f;

							if(aga_confvar(c, v, AGA_FLOAT, &f)) {
								AGA_NEWOBJ(flobj, float, (f));
								AGA_SETLISTITEM(p, i, flobj);
							}
						}
					}

					break;
				}
			}
		}

		result = aga_killconf(&conf);
		if(aga_script_aferr("aga_killconf", result)) return 0;
	}

	obj->drawlist = glGenLists(1);
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

	result = aga_killimg(&img);
	if(aga_script_aferr("aga_killimg", result)) return 0;

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

AGA_SCRIPTPROC(dumpobj) {
	enum af_err result;

	aga_pyobject_t path, o;
	struct aga_nativeptr* nativeptr;
	struct agan_object* obj;

	const char* s;
	FILE* f;
	af_size_t i;

	if(!AGA_ARGLIST(tuple) || !AGA_ARG(o, 0, nativeptr) ||
		!AGA_ARG(path, 1, string)) {

		AGA_ARGERR("dumpobj", "nativeptr and string");
	}

	nativeptr = (struct aga_nativeptr*) o;
	obj = nativeptr->ptr;

	AGA_SCRIPTVAL(s, path, string);
	if(!(f = fopen(s, "w+"))) {
		aga_af_patherrno(__FILE__, "fopen", s);
		return 0;
	}

	result = aga_fprintf(f, "<root>\n");
	if(aga_script_aferr("aga_fprintf", result)) return 0;

	/* TODO: Use res conf node to get this once implemented. */
	AGA_PUTITEM_STR(f, "Model", "AAAAAAAAAAAAAAAAAAAAA");
	AGA_PUTITEM_STR(f, "Texture", "AAAAAAAAAAAAAAAAAAAAAAAA");

	AGA_PUTITEM_INT(f, "Unlit", !!"AAAAAAAAAAAAAAAAAAAAAAAA");
	AGA_PUTITEM_INT(f, "ScaleTex", !!"AAAAAAAAAAAAAAAAAAAAAAAA");
	AGA_PUTITEM_INT(f, "Filter", !!"AAAAAAAAAAAAAAAAAAAAAAAA");

	for(i = 0; i < AF_ARRLEN(agan_conf_components); ++i) {
		aga_pyobject_t attr;
		af_size_t j;

		result = aga_fprintf(
			f, "\t<item name=\"%s\">\n", agan_conf_components[i]);
		if(aga_script_aferr("aga_fprintf", result)) return 0;

		AGA_GETATTR(obj->transform, agan_trans_components[i], attr);

		for(j = 0; j < AF_ARRLEN(agan_conf_components); ++j) {
			aga_pyobject_t flobj;
			float fl;

			AGA_GETLISTITEM(attr, j, flobj);
			AGA_SCRIPTVAL(fl, flobj, float);

			AGA_PUTITEM_FLOAT(f, agan_conf_components[j], fl);
		}

		result = aga_fprintf(f, "\t</item>\n");
		if(aga_script_aferr("aga_fprintf", result)) return 0;
	}

	result = aga_fprintf(f, "\t</root>\n");
	if(aga_script_aferr("aga_fprintf", result)) return 0;

	if(fclose(f) == EOF) {
		aga_af_errno(__FILE__, "fclose");
		return 0;
	}

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

AGA_SCRIPTPROC(glabi) {
#ifdef AF_GLXABI
	return newstringobject("x");
#elif defined(AF_WGL)
	return newstringobject("w");
#endif

	return 0;
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

	union aga_winenv* env;
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

enum af_err aga_mkmod(aga_pyobject_t* dict) {
#define _(name) { #name, agan_##name }
	struct methodlist methods[] = {
		/* Input */
		_(getkey),
		_(getmotion),
		_(setcursor),

		/* Drawing */
		_(setcam),
		_(text),
		_(glabi),
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
		_(dumpobj),
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

	*dict = agan_dict;

	return AF_ERR_NONE;
}

#endif
