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

int setpythonpath(char* path);
int setpythonargv(int argc, char** argv);
int flushline(void);
void donebuiltin(void);
void donesys(void);
void donedict(void);
void doneerrors(void);
void freeaccel(void);
object* call_function(object* func, object* arg);

#endif
