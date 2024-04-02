/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/aganmisc.h>

#include <agascripthelp.h>
#include <agastartup.h>
#include <agalog.h>

#include <apro.h>

struct py_object* agan_getconf(struct py_object* self, struct py_object* arg) {
	struct aga_opts* opts;
	struct py_object* v;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_GETCONF);

	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(!aga_arg_list(arg, PY_TYPE_LIST)) {
		return aga_arg_error("getconf", "list");
	}

	v = agan_scriptconf(&opts->config, AGA_TRUE, arg);

	apro_stamp_end(APRO_SCRIPTGLUE_GETCONF);

	return v;
}

struct py_object* agan_log(struct py_object* self, struct py_object* arg) {
	const char* loc;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_LOG);

	if(!arg) {
		py_error_set_string(py_runtime_error, "log() takes one argument");
		return 0;
	}

	if(aga_script_string(py_frame_current->code->filename, &loc)) return 0;

	switch(arg->type) {
		default: {
			aga_log(loc, "<object @%p>", arg);

			break;
		}

		case PY_TYPE_STRING: {
			const char* str;

			if(aga_script_string(arg, &str)) return 0;
			aga_log(loc, str);

			break;
		}

			/* TODO: Implement `py_string_cat' of non-string objects. */
		case PY_TYPE_FLOAT: {
			double f;

			if(aga_script_float(arg, &f)) return 0;
			aga_log(loc, "%lf", f);

			break;
		}

		case PY_TYPE_INT: {
			py_value_t i;

			if(aga_script_int(arg, &i)) return 0;
			aga_log(loc, "%llu", i);

			break;
		}
	}

	apro_stamp_end(APRO_SCRIPTGLUE_LOG);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_die(struct py_object* self, struct py_object* arg) {
	aga_bool_t* die;

	(void) self;
	(void) arg;

	apro_stamp_start(APRO_SCRIPTGLUE_DIE);

	if(!(die = aga_getscriptptr(AGA_SCRIPT_DIE))) return 0;
	*die = AGA_TRUE;

	apro_stamp_end(APRO_SCRIPTGLUE_DIE);

	return py_object_incref(PY_NONE);
}
