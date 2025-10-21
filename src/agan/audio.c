/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/audio.h>

#include <aga/sound.h>
#include <aga/script.h>
#include <aga/pack.h>

#include <apro.h>

enum asys_result agan_audio_register(struct py_env* env) {
	(void) env;

	return ASYS_RESULT_OK;
}

struct py_object* agan_playsnd(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum asys_result result;

	struct aga_resource* resource;

	struct py_object* path;
	struct py_object* loop;
	struct py_object* index;

	asys_size_t index_value;

	(void) self;

	apro_stamp_start(APRO_SCRIPTGLUE_PLAYSND);

	/* playsnd(string, int) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
		!aga_arg(&path, args, 0, PY_TYPE_STRING) ||
		!aga_arg(&loop, args, 1, PY_TYPE_INT)) {

		return aga_arg_error("text", "string and int");
	}

	result = aga_resource_pack_lookup(
			AGA_GET_USERDATA(env)->resource_pack, py_string_get(path),
			&resource);

	if(aga_script_err(__FILE__, "aga_resource_pack_lookup", result)) return 0;

	result = aga_sound_play(
				AGA_GET_USERDATA(env)->sound_device, resource,
				py_int_get(loop), &index_value);

	if(aga_script_err(__FILE__, "aga_sound_play", result)) return 0;

	index = py_int_new((py_value_t) index_value);

	apro_stamp_end(APRO_SCRIPTGLUE_PLAYSND);

	return index;
}

struct py_object* agan_clrsnd(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum asys_result result;

	(void) self;
	(void) args;

	apro_stamp_start(APRO_SCRIPTGLUE_CLRSND);

	result = aga_sound_clear(AGA_GET_USERDATA(env)->sound_device);
	if(aga_script_err(__FILE__, "aga_sound_clear", result)) return 0;

	apro_stamp_end(APRO_SCRIPTGLUE_CLRSND);

	return py_object_incref(PY_NONE);
}
