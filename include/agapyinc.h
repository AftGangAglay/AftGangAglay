/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PYINC_H
#define AGA_PYINC_H

#include <agaenv.h>

#include <allobjects.h>
#include <traceback.h>
#include <modsupport.h>
#include <node.h>
#include <compile.h>
#include <frameobject.h>
#include <pythonrun.h>
#include <import.h>
#include <grammar.h>
#include <pgen.h>
#include <errcode.h>
#include <parsetok.h>
#include <ceval.h>

struct aga_nativeptr {
    OB_HEAD
    void* ptr;
    aga_size_t len;
};

typedef object* aga_pyobject_t;

extern const typeobject aga_nativeptr_type;

#define is_nativeptrobject(o) ((o)->ob_type == &aga_nativeptr_type)
aga_pyobject_t newnativeptrobject(void);

int setpythonpath(char* path);
int setpythonargv(int argc, char** argv);
int flushline(void);
void donebuiltin(void);
void donesys(void);
void donedict(void);
void doneerrors(void);
void freeaccel(void);
aga_pyobject_t call_function(aga_pyobject_t func, aga_pyobject_t arg);

#endif
