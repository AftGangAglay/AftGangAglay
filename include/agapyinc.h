/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PYINC_H
#define AGA_PYINC_H

#include <agaenv.h>

#include <python/errors.h>
#include <python/result.h>
#include <python/ceval.h>
#include <python/traceback.h>
#include <python/compile.h>
#include <python/import.h>
#include <python/parsetok.h>
#include <python/fgetsintr.h>
#include <python/pgen.h>
#include <python/modsupport.h>

#include <python/bltinmodule.h>
#include <python/sysmodule.h>

/* TODO: `object' include subdir. */
#include <python/object.h>
#include <python/floatobject.h>
#include <python/stringobject.h>
#include <python/moduleobject.h>
#include <python/intobject.h>
#include <python/listobject.h>
#include <python/tupleobject.h>
#include <python/dictobject.h>
#include <python/classobject.h>
#include <python/methodobject.h>

#endif
