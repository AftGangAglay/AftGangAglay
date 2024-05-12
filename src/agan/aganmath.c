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

enum aga_result agan_math_register(void) {
	static const double pi = 3.14159265358979323846;
	static const double e = 2.71828182845904523536;

	enum aga_result result;

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
	py_value_t av, bv;

	AGA_DEPRCALL("agan.bitand", "math.andb");

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_BITAND);

	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
	   !aga_arg(&a, args, 0, PY_TYPE_INT) ||
	   !aga_arg(&b, args, 1, PY_TYPE_INT)) {
		return aga_arg_error("bitand", "int and int");
	}

	if(aga_script_int(a, &av)) return 0;
	if(aga_script_int(b, &bv)) return 0;

	v = py_int_new(av & bv);

	apro_stamp_end(APRO_SCRIPTGLUE_BITAND);

	return v;
}

struct py_object* agan_bitshl(
		struct py_env* env, struct py_object* self, struct py_object* args) {
	struct py_object* a;
	struct py_object* b;
	struct py_object* v;
	py_value_t av, bv;

	AGA_DEPRCALL("agan.bitshl", "math.shl");

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_BITSHL);

	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
	   !aga_arg(&a, args, 0, PY_TYPE_INT) ||
	   !aga_arg(&b, args, 1, PY_TYPE_INT)) {
		return aga_arg_error("bitshl", "int and int");
	}

	if(aga_script_int(a, &av)) return 0;
	if(aga_script_int(b, &bv)) return 0;

	v = py_int_new(av << bv);

	apro_stamp_end(APRO_SCRIPTGLUE_BITSHL);

	return v;
}

struct py_object* agan_randnorm(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* v;

	AGA_DEPRCALL("agan.randnorm", "math.randf");

	(void) env;
	(void) self;
	(void) args;

	apro_stamp_start(APRO_SCRIPTGLUE_RANDNORM);

	v = py_float_new((double) rand() / (double) RAND_MAX);

	apro_stamp_end(APRO_SCRIPTGLUE_RANDNORM);

	return v;
}

struct py_object* agan_bitor(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct py_object* ob;
	py_value_t v;
	py_value_t res = 0;
	unsigned i;

	AGA_DEPRCALL("agan.bitor", "math.orb");

	(void) env;
	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_BITOR);

	if(!aga_arg_list(args, PY_TYPE_LIST)) {
		return aga_arg_error("bitor", "list");
	}

	for(i = 0; i < py_varobject_size(args); ++i) {
		if(aga_list_get(args, i, &ob)) return 0;
		if(aga_script_int(ob, &v)) return 0;

		res |= v;
	}

	apro_stamp_end(APRO_SCRIPTGLUE_BITOR);

	return py_int_new(res);
}
