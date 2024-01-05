# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PYTHON_ROOT = vendor/python
PYTHON_SOURCEROOT = $(PYTHON_ROOT)/src

PYTHON_IFLAGS = -isystem $(PYTHON_SOURCEROOT)

PYTHON_GRAMINIT_H = $(PYTHON_SOURCEROOT)/graminit.h
PYTHON_GRAMINIT = $(PYTHON_SOURCEROOT)/graminit.c $(PYTHON_GRAMINIT_H)                
PYTHON_GRAMINIT_OBJECT = $(PYTHON_SOURCEROOT)/graminit.o

LIBPYTHON_SOURCES = acceler.c bltinmodule.c ceval.c classobject.c \
               compile.c dictobject.c errors.c fgetsintr.c \
               fileobject.c floatobject.c frameobject.c \
               funcobject.c graminit.c grammar1.c import.c \
               intobject.c intrcheck.c listnode.c listobject.c \
               mathmodule.c methodobject.c modsupport.c \
               moduleobject.c node.c object.c parser.c \
               parsetok.c posixmodule.c regexp.c regexpmodule.c \
               strdup.c \
               stringobject.c structmember.c \
               sysmodule.c timemodule.c tokenizer.c traceback.c \
               tupleobject.c typeobject.c
LIBPYTHON_SOURCES := $(addprefix $(PYTHON_SOURCEROOT)/,$(LIBPYTHON_SOURCES))
LIBPYTHON_OBJECTS = $(LIBPYTHON_SOURCES:.c=.o)

LIBPYTHON = vendor/libpython.a

LIBPYTHON_CFLAGS = -w -DSYSV
ifdef WINDOWS
	LIBPYTHON_CFLAGS += -DNO_LSTAT
endif

PYTHONGEN_SOURCES = pgenmain.c acceler.c fgetsintr.c grammar1.c \
                           intrcheck.c listnode.c node.c parser.c \
                           parsetok.c strdup.c tokenizer.c bitset.c \
                           firstsets.c grammar.c metagrammar.c pgen.c \
                           printgrammar.c
PYTHONGEN_SOURCES := $(addprefix $(PYTHON_SOURCEROOT)/,$(PYTHONGEN_SOURCES))
PYTHONGEN_OBJECTS = $(PYTHONGEN_SOURCES:.c=.o)

PYTHONGEN = $(PYTHON_SOURCEROOT)/pgenmain$(EXE)

$(LIBPYTHON): CFLAGS += $(LIBPYTHON_CFLAGS)
$(LIBPYTHON): $(LIBPYTHON_OBJECTS) $(PYTHON_GRAMINIT_OBJECT)

$(filter-out $(PYTHONGEN_OBJECTS),$(LIBPYTHON_OBJECTS)): $(PYTHON_GRAMINIT)

$(PYTHONGEN): LDLIBS = -lm
$(PYTHONGEN): CFLAGS += $(LIBPYTHON_CFLAGS)
$(PYTHONGEN): $(PYTHONGEN_OBJECTS)

$(PYTHON_GRAMINIT): $(PYTHON_SOURCEROOT)/gr/Grammar $(PYTHONGEN)
	-$(CROSS_TOOL) $(PYTHONGEN) $< $(PYTHON_GRAMINIT)

clean: clean_python
.PHONY: clean_python
clean_python:
	$(call PATHREM,$(LIBPYTHON))
	$(call PATHREM,$(PYTHONGEN))
	$(call PATHREM,$(PYTHONGEN_OBJECTS))
	$(call PATHREM,$(LIBPYTHON_OBJECTS))