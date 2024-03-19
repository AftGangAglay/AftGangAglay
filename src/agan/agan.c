/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/agan.h>

/*
 * NOTE: We need a bit of global state here to get engine system contexts etc.
 * 		 Into script land because this version of Python's state is spread
 * 		 Across every continent.
 */
struct py_object* agan_dict = 0;

const char* agan_trans_components[3] = { "pos", "rot", "scale" };
const char* agan_conf_components[3] = { "Position", "Rotation", "Scale" };
const char* agan_xyz[3] = { "X", "Y", "Z" };
const char* agan_rgb[3] = { "R", "G", "B" };
