/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGAN_OBJECT_H
#define AGAN_OBJECT_H

/*
 * Defines the world-object type used by script glue. Game objects typically
 * Consist of a world-object and behaviours ascribed in script land. We should
 * Not attach meaningful autonomous behaviour to these, they're controlled by
 * The user.
 */

struct aga_vertex {
    float col[4];
    float uv[2];
    float norm[3];
    float pos[3];
};

struct agan_normcolor {
    float r, g, b, a;
};

enum agan_lighttype {
    AGAN_SPOT,
    AGAN_POINT,
    AGAN_DIRECTIONAL
};

struct agan_lightdata {
    struct agan_normcolor ambient;
    struct agan_normcolor diffuse;
    struct agan_normcolor specular;

    float constant_attenuation;
    float linear_attenuation;
    float quadratic_attenuation;

    float direction[3];

    enum agan_lighttype type;

    aga_uint8_t spot_exponent;
    aga_uint8_t light_index;
};

/* TODO: Central object/light registry and distribute handles. */
struct agan_object {
    aga_pyobject_t transform;
    struct aga_res* res;
    struct agan_lightdata* light_data;

    aga_uint_t drawlist;
    float min_extent[3];
    float max_extent[3];
};

#endif
