/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/utility.h>

#include <aga/startup.h>
#include <aga/script.h>
#include <aga/pack.h>

#include <asys/log.h>
#include <asys/string.h>

#include <apro.h>

enum asys_result agan_misc_register(struct py_env* env) {
	enum asys_result result;

	(void) env;

	if((result = aga_insert_string("VERSION", AGA_VERSION))) return result;

#ifdef ASYS_WIN32
	if((result = aga_insert_string("PLATFORM", "win32"))) return result;
#else
	if((result = aga_insert_string("PLATFORM", "x"))) return result;
#endif

	/* TODO: Var to detect dev builds. */
#ifndef NDEBUG
	if((result = aga_insert_string("MODE", "debug"))) return result;
#else
	if((result = aga_insertstr("MODE", "release"))) return result;
#endif

	/* TODO: BSD-y builds some day -- `bmake' support etc. */
	/* TODO: IRIX-y builds some day -- IRIX `make' support etc. */
#ifdef _MSC_VER
	if((result = aga_insertstr("CENV", "vc"))) return result;
#elif defined(__GNUC__)
	if((result = aga_insert_string("CENV", "gnu"))) return result;
#else
	if((result = aga_insertstr("CENV", "std"))) return result;
#endif

	return ASYS_RESULT_OK;
}

struct py_object* agan_getconf(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct aga_settings* opts = AGA_GET_USERDATA(env)->opts;
	struct py_object* v;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_GETCONF);

	/* getconf(list[...]) */
	if(!aga_arg_list(args, PY_TYPE_LIST)) {
		return aga_arg_error("getconf", "list");
	}

	v = agan_scriptconf(&opts->config, ASYS_TRUE, args);

	apro_stamp_end(APRO_SCRIPTGLUE_GETCONF);

	return v;
}

/* TODO: Only create once and cache until pack reload. */
struct py_object* agan_packlist(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct aga_resource_pack* pack = AGA_GET_USERDATA(env)->resource_pack;

	struct py_object* retval;

	asys_size_t i;

	(void) env;
	(void) self;

	/* TODO: Add profile tags. */

	if(args) return aga_arg_error("packlist", "none");

	if(!(retval = py_list_new(pack->count))) {
		py_error_set_nomem();
		return 0;
	}

	for(i = 0; i < pack->count; ++i) {
		struct py_object* string;

		string = py_string_new(pack->resources[i].config->name);
		if(!string) {
			py_error_set_nomem();
			goto cleanup;
		}

		py_list_set(retval, i, string);
	}

	return retval;

	cleanup: {
		for(i = 0; i < pack->count; ++i) {
			py_object_decref(py_list_get(retval, i));
		}

		py_object_decref(retval);

		return 0;
	}
}

static const char* agan_object_type_name(const struct py_object* op) {
	switch(op->type) {
		default: return "object";

		case PY_TYPE_TYPE: return "type";
		case PY_TYPE_NONE: return "none";
		case PY_TYPE_CLASS: return "class";
		case PY_TYPE_CLASS_MEMBER: return "class_member";
		case PY_TYPE_CLASS_METHOD: return "class_method";
		case PY_TYPE_CODE: return "code";
		case PY_TYPE_FRAME: return "frame";
		case PY_TYPE_TRACEBACK: return "traceback";
		case PY_TYPE_FUNC: return "func";
		case PY_TYPE_METHOD: return "method";
		case PY_TYPE_MODULE: return "module";
		case PY_TYPE_TUPLE: return "tuple";
		case PY_TYPE_LIST: return "list";
		case PY_TYPE_STRING: return "string";
		case PY_TYPE_DICT: return "dict";
		case PY_TYPE_INT: return "int";
		case PY_TYPE_FLOAT: return "float";
	}
}

static enum asys_result agan_log_object(
		struct py_object* op, asys_fixed_buffer_t* buffer, asys_bool_t top) {

	enum asys_result result;

	asys_fixed_buffer_t inter;

	/* TODO: Re-implement `str()` for all objects. */
	/* TODO: Implement `py_string_cat' of non-string objects. */
	/* TODO: Cannot directly print float values under `wsprintf'. */
	/*
	 * TODO: Can improve performance here by appending directly and
	 * 		 Keeping a running end point from the size out param.
	 */

	switch(op->type) {
		default: {
			result = asys_string_format(
					&inter, 0, "<%s @%p>",
					agan_object_type_name(op), (void*) op);

			if(result) return result;

			asys_string_concatenate(*buffer, inter);
			break;
		}

		case PY_TYPE_STRING: {
			if(!top) asys_string_concatenate(*buffer, "'");
			asys_string_concatenate(*buffer, py_string_get(op));
			if(!top) asys_string_concatenate(*buffer, "'");

			break;
		}

		case PY_TYPE_INT: {
			result = asys_string_format(&inter, 0, "%llu", py_int_get(op));
			if(result) return result;

			asys_string_concatenate(*buffer, inter);
			break;
		}

		case PY_TYPE_LIST: {
			asys_size_t i, len;

			len = py_varobject_size(op);

			asys_string_concatenate(*buffer, "[ ");
			for(i = 0; i < len; ++i) {
				result = agan_log_object(
						py_list_get(op, i), buffer, ASYS_FALSE);

				if(result) return result;

				if(i != len - 1) asys_string_concatenate(*buffer, ", ");
			}
			asys_string_concatenate(*buffer, " ]");

			break;
		}

		case PY_TYPE_TUPLE: {
			asys_size_t i, len;

			len = py_varobject_size(op);

			asys_string_concatenate(*buffer, "( ");
			for(i = 0; i < len; ++i) {
				result = agan_log_object(
						py_tuple_get(op, i), buffer, ASYS_FALSE);

				if(result) return result;

				if(i != len - 1) asys_string_concatenate(*buffer, ", ");
			}
			asys_string_concatenate(*buffer, " )");

			break;
		}

		case PY_TYPE_DICT: {
			asys_size_t i;
			struct py_dict* dp = (void*) op;

			asys_string_concatenate(*buffer, "{ ");
			for(i = 0; i < dp->size; ++i) {
				struct py_dictentry e = dp->table[i];

				if(!e.key) continue;

				result = agan_log_object(e.key, buffer, ASYS_FALSE);
				if(result) return result;

				asys_string_concatenate(*buffer, ": ");

				result = agan_log_object(e.value, buffer, ASYS_FALSE);
				if(result) return result;

				if(i != dp->size - 1) asys_string_concatenate(*buffer, ", ");
			}
			asys_string_concatenate(*buffer, " }");

			break;
		}
	}

	return ASYS_RESULT_OK;
}

struct py_object* agan_log(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	static asys_fixed_buffer_t buffer;

	const char* file;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_LOG);

	buffer[0] = 0;

	/* log(object...) */
	if(!args) {
		return aga_arg_error("log", "object...");
	}

	file = py_string_get(env->current->code->filename);

	agan_log_object(args, &buffer, ASYS_TRUE);

	asys_log(file, buffer);

	apro_stamp_end(APRO_SCRIPTGLUE_LOG);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_die(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	asys_bool_t* die = AGA_GET_USERDATA(env)->die;

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_DIE);

	if(args) return aga_arg_error("die", "none");

	*die = ASYS_TRUE;

	apro_stamp_end(APRO_SCRIPTGLUE_DIE);

	return py_object_incref(PY_NONE);
}

struct py_object* agan_dt(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	(void) env;
	(void) self;

	if(args) return aga_arg_error("dt", "none");

	return py_int_new(*AGA_GET_USERDATA(env)->dt);
}

/* TODO: Add to builtin on next major release. */
struct py_object* agan_strsplit(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* retval;
	struct py_object* string;
	struct py_object* separator;

	struct py_object* ob;

	const char* s;
	const char* mark;
	const char* prev;
	char sep;
	asys_size_t i;

	(void) env;
	(void) self;

	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
		!aga_arg(&string, args, 0, PY_TYPE_STRING) ||
		!aga_vararg(&separator, args, 1, PY_TYPE_STRING, 1)) {

		return aga_arg_error("strsplit", "string and string[1]");
	}

	s = py_string_get(string);
	sep = *py_string_get(separator);

	if(!(retval = py_list_new(0))) {
		py_error_set_nomem();
		return 0;
	}

	mark = prev = s;
	while((mark = asys_string_find_const(mark, sep))) {
		asys_size_t len;

		if(*prev == sep) prev++;

		len = mark - prev;

		if(!(ob = py_string_new_size(prev, len))) {
			py_error_set_nomem();
			goto cleanup;
		}

		if(py_list_add(retval, ob) == -1) {
			py_error_set_nomem();
			goto cleanup;
		}

		prev = mark;
		mark++;
	}

	if(*prev == sep) prev++;

	if(!(ob = py_string_new(prev))) {
		py_error_set_nomem();
		goto cleanup;
	}

	if(py_list_add(retval, ob) == -1) {
		py_error_set_nomem();
		goto cleanup;
	}

	return retval;

	cleanup: {
		for(i = 0; i < py_varobject_size(retval); ++i) {
			py_object_decref(py_list_get(retval, i));
		}

		py_object_decref(retval);

		return 0;
	}
}
