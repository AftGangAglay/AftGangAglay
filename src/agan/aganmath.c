/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/aganmath.h>

#include <agascripthelp.h>

#include <apro.h>

/* Python lacks native bitwise ops @-@ */
struct py_object* agan_bitand(struct py_object* self, struct py_object* arg) {
	struct py_object* a;
	struct py_object* b;
	struct py_object* v;
	py_value_t av, bv;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_BITAND);

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&a, arg, 0, PY_TYPE_INT) ||
	   !aga_arg(&b, arg, 1, PY_TYPE_INT)) {
		return aga_arg_error("bitand", "int and int");
	}

	if(aga_script_int(a, &av)) return 0;
	if(aga_script_int(b, &bv)) return 0;

	v = py_int_new(av & bv);

	apro_stamp_end(APRO_SCRIPTGLUE_BITAND);

	return v;
}

struct py_object* agan_bitshl(struct py_object* self, struct py_object* arg) {
	struct py_object* a;
	struct py_object* b;
	struct py_object* v;
	py_value_t av, bv;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_BITSHL);

	if(!aga_arg_list(arg, PY_TYPE_TUPLE) ||
	   !aga_arg(&a, arg, 0, PY_TYPE_INT) ||
	   !aga_arg(&b, arg, 1, PY_TYPE_INT)) {
		return aga_arg_error("bitshl", "int and int");
	}

	if(aga_script_int(a, &av)) return 0;
	if(aga_script_int(b, &bv)) return 0;

	v = py_int_new(av << bv);

	apro_stamp_end(APRO_SCRIPTGLUE_BITSHL);

	return v;
}

struct py_object* agan_randnorm(
		struct py_object* self, struct py_object* arg) {

	struct py_object* v;

	(void) self;
	(void) arg;

	apro_stamp_start(APRO_SCRIPTGLUE_RANDNORM);

	v = py_float_new((double) rand() / (double) RAND_MAX);

	apro_stamp_end(APRO_SCRIPTGLUE_RANDNORM);

	return v;
}
