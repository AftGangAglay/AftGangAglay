/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023-2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_USERDATA_H
#define AGA_USERDATA_H

#include <mil/base.h>

struct aga_script_engine;
struct aga_resource_pack;
struct aga_sound_device;
struct aga_settings;
struct aga_graph;
struct aga_input_pack;

struct py_object;

struct aga_mil_userdata {
	mil_widget_t gl_area;

	struct aga_script_engine* script_engine;
	struct py_object* script_instance;
	struct py_object* script_update;

#ifdef AGA_DEVBUILD
	struct py_object* ui_instance;
	struct py_object* ui_activate;
#endif

	struct aga_resource_pack* resource_pack;

	struct aga_sound_device* sound_device;

	struct aga_settings* settings;
	asys_bool_t* die;

	struct aga_graph* profile_graph;

	struct aga_input_pack* input;

	asys_bool_t frame_zero;
};

#endif
