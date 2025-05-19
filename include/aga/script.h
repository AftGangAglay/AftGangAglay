/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_H
#define AGA_SCRIPT_H

#include <asys/base.h>
#include <asys/result.h>

#include <apro.h>

#include <mil/base.h>

struct aga_settings;

struct aga_resource;
struct aga_resource_pack;

struct aga_input_pack;

struct aga_sound_device;

struct aga_script_userdata {
	struct mil_ctx* mil;
	mil_widget_t gl_area;

	struct aga_settings* opts;

	struct aga_sound_device* sound_device;

	struct aga_resource_pack* resource_pack;

	struct aga_input_pack* input;

	asys_bool_t* die;
	apro_unit_t* dt;
};

struct py_object;
struct py_module;

struct aga_script_engine {
	struct py* py;
	struct py_env* env; /* TODO: This is temporary. */

	struct py_object* global;
	struct py_module* agan;
};

enum asys_result aga_script_engine_new(
		struct aga_script_engine*, const char*, struct aga_resource_pack*,
		const char*, void*);

enum asys_result aga_script_engine_delete(struct aga_script_engine*);

void aga_script_engine_trace(void);

#endif
