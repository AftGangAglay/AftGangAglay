/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_OBJ_H
#define AGAN_OBJ_H

#include <agan/agan.h>

/*
 * Defines the world-object type used by script glue. Game objects typically
 * Consist of a world-object and behaviours ascribed in script land. We should
 * Not attach meaningful autonomous behaviour to these, they're controlled by
 * The user.
 */

struct aga_resource_pack;

struct aga_vertex {
	float col[4];
	float uv[2];
	float norm[3];
	float pos[3];
};

struct agan_lightdata {
	float ambient[4];
	float diffuse[4];
	float specular[4];

	float constant_attenuation;
	float linear_attenuation;
	float quadratic_attenuation;

	float direction[3];

	float exponent;
	float angle;

	asys_bool_t directional;

	asys_uchar_t index;
};

/*
 * TODO: Central object/light registry and distribute handles. Spatializing
 * 		 This makes it easier to do streaming/chunking and means we can ensure
 * 		 Sequential reads -- which is super important if we want to run off CD
 * 		 Media. Maybe add a separate file type in packs which represent
 * 		 Contiguous interleaved objects to avoid needing to seek over
 * 		 Intermediate structures or rely on pack ordering at an application
 * 		 Maintainer level.
 */
struct agan_object {
	struct py_object* transform;
	struct aga_resource* res;
	struct agan_lightdata* light_data;
	asys_uint_t ind;

#ifdef AGA_DEVBUILD
	char* modelpath;
#endif

	asys_uint_t drawlist;
	float min_extent[3];
	float max_extent[3];
};

enum asys_result agan_getobjconf(struct agan_object*, struct aga_config_node*);

enum asys_result agan_obj_register(struct py_env*);

asys_bool_t agan_mkobj_model(
		struct py_env*, struct agan_object*, struct aga_config_node*,
		struct aga_resource_pack*, const char*);

struct py_object* agan_mkobj(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_inobj(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_putobj(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_killobj(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_objtrans(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_objconf(
		struct py_env*, struct py_object*, struct py_object*);

struct py_object* agan_objind(
		struct py_env*, struct py_object*, struct py_object*);

#endif
