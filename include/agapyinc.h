/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_PYINC_H
#define AGA_PYINC_H

/*
 * NOTE: You probably shouldn't be including this unless you're creating a new
 *       Scriptglue source. For the non-polluting script API use `agascript.h'.
 */

/* TODO: Sort out pyinc vs. agan general decls. */

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
#include <graminit.h>
#include <errcode.h>
#include <parsetok.h>
#include <ceval.h>

#define AGA_SCRIPTPROC(name) \
	aga_pyobject_t agan_##name(AGA_UNUSED aga_pyobject_t self, \
           AGA_UNUSED aga_pyobject_t arg)

struct aga_conf_node;

struct aga_nativeptr {
    OB_HEAD
    void* ptr;
    aga_size_t len;
};

typedef object* aga_pyobject_t;

extern aga_pyobject_t agan_dict;

extern const typeobject aga_nativeptr_type;

extern const char* agan_trans_components[3];
extern const char* agan_conf_components[3];
extern const char* agan_xyz[3];

#define is_nativeptrobject(o) ((o)->ob_type == &aga_nativeptr_type)
aga_pyobject_t newnativeptrobject(void);

aga_bool_t agan_settransmat(aga_pyobject_t trans, aga_bool_t inv);
aga_pyobject_t agan_scriptconf(
        struct aga_conf_node* node, aga_bool_t root, aga_pyobject_t list);

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
