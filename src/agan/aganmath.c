/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/aganmath.h>

#include <agascripthelp.h>
#include <agalog.h>
#include <agadiag.h>

#include <apro.h>

/*
 * NOTE: The `aganmath' module is **DEPRECATED** in favour of placing relevant
 * 		 Procs. in `builtinmodule' or `mathmodule'.
 */

enum aga_result agan_math_register(struct py_env* env) {
	static const double pi = 3.14159265358979323846;
	static const double e = 2.71828182845904523536;

	enum aga_result result;

	(void) env;

	if((result = aga_insertfloat("PI", pi))) return result;

	if((result = aga_insertfloat("RADS", pi / 180.0))) return result;

	if((result = aga_insertfloat("E", e))) return result;

	return AGA_RESULT_OK;
}

/* Python lacks native bitwise ops @-@ */
struct py_object* agan_bitand(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* a;
	struct py_object* b;
	struct py_object* v;

	AGA_DEPRCALL("agan.bitand", "math.andb");

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_BITAND);

	/* bitand(int, int) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
		!aga_arg(&a, args, 0, PY_TYPE_INT) ||
		!aga_arg(&b, args, 1, PY_TYPE_INT)) {

		return aga_arg_error("bitand", "int and int");
	}

	if(!(v = py_int_new(py_int_get(a) & py_int_get(b)))) {
		py_error_set_nomem();
	}

	apro_stamp_end(APRO_SCRIPTGLUE_BITAND);

	return v;
}

struct py_object* agan_bitshl(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* a;
	struct py_object* b;
	struct py_object* v;

	AGA_DEPRCALL("agan.bitshl", "math.shl");

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_BITSHL);

	/* bitshl(int, int) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
		!aga_arg(&a, args, 0, PY_TYPE_INT) ||
		!aga_arg(&b, args, 1, PY_TYPE_INT)) {

		return aga_arg_error("bitshl", "int and int");
	}

	if(!(v = py_int_new(py_int_get(a) << py_int_get(b)))) {
		py_error_set_nomem();
	}

	apro_stamp_end(APRO_SCRIPTGLUE_BITSHL);

	return v;
}

struct py_object* agan_randnorm(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* v;

	AGA_DEPRCALL("agan.randnorm", "math.randf");

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_RANDNORM);

	if(args) return aga_arg_error("randnorm", "none");

	if(!(v = py_float_new((double) rand() / (double) RAND_MAX))) {
		py_error_set_nomem();
	}

	apro_stamp_end(APRO_SCRIPTGLUE_RANDNORM);

	return v;
}

struct py_object* agan_bitor(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	py_value_t res = 0;
	unsigned i;

	AGA_DEPRCALL("agan.bitor", "math.orb");

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_BITOR);

	/* bitor(int...) */
	if(!aga_arg_list(args, PY_TYPE_LIST)) {
		return aga_arg_error("bitor", "int...");
	}

	for(i = 0; i < py_varobject_size(args); ++i) {
		struct py_object* op = py_list_get(args, i);

		if(op->type != PY_TYPE_INT) {
			py_error_set_badarg();
			return 0;
		}

		res |= py_int_get(py_list_get(args, i));
	}

	apro_stamp_end(APRO_SCRIPTGLUE_BITOR);

	return py_int_new(res);
}
