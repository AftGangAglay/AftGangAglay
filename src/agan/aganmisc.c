/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/aganmisc.h>

#include <agascripthelp.h>
#include <agastartup.h>
#include <agalog.h>
#include <agascript.h>

#include <apro.h>

enum aga_result agan_misc_register(struct py_env* env) {
	enum aga_result result;

	(void) env;

	if((result = aga_insertstr("VERSION", AGA_VERSION))) return result;

#ifdef _WIN32
	if((result = aga_insertstr("PLATFORM", "win32"))) return result;
#else
	if((result = aga_insertstr("PLATFORM", "x"))) return result;
#endif

#ifdef _DEBUG
	if((result = aga_insertstr("MODE", "debug"))) return result;
#else
	if((result = aga_insertstr("MODE", "release"))) return result;
#endif

#ifdef _MSC_VER
	if((result = aga_insertstr("CENV", "vc"))) return result;
#elif defined(__GNUC__)
	if((result = aga_insertstr("CENV", "gnu"))) return result;
#else
	if((result = aga_insertstr("CENV", "std"))) return result;
#endif

	return AGA_RESULT_OK;
}

struct py_object* agan_getconf(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct aga_opts* opts;
	struct py_object* v;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_GETCONF);

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	/* getconf(list[...]) */
	if(!aga_arg_list(args, PY_TYPE_LIST)) {
		return aga_arg_error("getconf", "list");
	}

	v = agan_scriptconf(&opts->config, AGA_TRUE, args);

	apro_stamp_end(APRO_SCRIPTGLUE_GETCONF);

	return v;
}

static void agan_log_object(struct py_object* op, const char* loc) {
	switch(op->type) {
		default: {
			/* TODO: Re-implement `str()` for all objects. */
			aga_log(loc, "<object @%p>", (void*) op);
			break;
		}

		case PY_TYPE_STRING: {
			aga_log(loc, py_string_get(op));
			break;
		}

		/* TODO: Implement `py_string_cat' of non-string objects. */
		case PY_TYPE_FLOAT: {
			aga_log(loc, "%lf", py_float_get(op));
			break;
		}

		case PY_TYPE_INT: {
			aga_log(loc, "%llu", py_int_get(op));
			break;
		}
	}
}

struct py_object* agan_log(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	unsigned i;
	const char* loc;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_LOG);

	/* log(object...) */
	if(!args) {
		return aga_arg_error("log", "object...");
	}

	loc = py_string_get(env->current->code->filename);

	if(py_is_varobject(args) && args->type != PY_TYPE_STRING) {
		for(i = 0; i < py_varobject_size(args); ++i) {
			/*
			 * TODO: Single line logging instead of the series of logs that
			 *       This produces.
			 */
			if(args->type == PY_TYPE_LIST) {
				agan_log_object(py_list_get(args, i), loc);
			}
			else if(args->type == PY_TYPE_TUPLE) {
				agan_log_object(py_tuple_get(args, i), loc);
			}
		}
	}
	else agan_log_object(args, loc);

	apro_stamp_end(APRO_SCRIPTGLUE_LOG);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_die(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	aga_bool_t* die;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_DIE);

	if(args) return aga_arg_error("die", "none");

	if(!(die = aga_getscriptptr(AGA_SCRIPT_DIE))) return 0;
	*die = AGA_TRUE;

	apro_stamp_end(APRO_SCRIPTGLUE_DIE);

	return py_object_incref(PY_NONE);
}
