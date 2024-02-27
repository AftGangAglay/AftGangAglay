# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PYTHON = vendor$(SEP)python$(SEP)src$(SEP)

PYTHON_SOURCES1 = $(PYTHON)acceler.c $(PYTHON)bltinmodule.c $(PYTHON)ceval.c
PYTHON_SOURCES2 = $(PYTHON)classobject.c $(PYTHON)compile.c
PYTHON_SOURCES3 = $(PYTHON)dictobject.c $(PYTHON)errors.c $(PYTHON)fgetsintr.c
PYTHON_SOURCES4 = $(PYTHON)fileobject.c $(PYTHON)floatobject.c
PYTHON_SOURCES5 = $(PYTHON)frameobject.c $(PYTHON)funcobject.c
PYTHON_SOURCES6 = $(PYTHON)graminit.c $(PYTHON)grammar1.c $(PYTHON)import.c
PYTHON_SOURCES7 = $(PYTHON)intobject.c $(PYTHON)intrcheck.c $(PYTHON)listnode.c
PYTHON_SOURCES8 = $(PYTHON)listobject.c $(PYTHON)mathmodule.c
PYTHON_SOURCES9 = $(PYTHON)methodobject.c $(PYTHON)modsupport.c
PYTHON_SOURCES10 = $(PYTHON)moduleobject.c $(PYTHON)node.c $(PYTHON)object.c
PYTHON_SOURCES11 = $(PYTHON)parser.c $(PYTHON)parsetok.c $(PYTHON)posixmodule.c
PYTHON_SOURCES12 = $(PYTHON)regexp.c $(PYTHON)regexpmodule.c $(PYTHON)strdup.c
PYTHON_SOURCES13 = $(PYTHON)stringobject.c $(PYTHON)structmember.c
PYTHON_SOURCES14 = $(PYTHON)sysmodule.c $(PYTHON)timemodule.c
PYTHON_SOURCES15 = $(PYTHON)tokenizer.c $(PYTHON)traceback.c
PYTHON_SOURCES16 = $(PYTHON)tupleobject.c $(PYTHON)typeobject.c

PYTHON_OBJECTS1 = $(subst .c,$(OBJ),$(PYTHON_SOURCES1)) $(subst .c,$(OBJ),$(PYTHON_SOURCES2))
PYTHON_OBJECTS2 = $(subst .c,$(OBJ),$(PYTHON_SOURCES3)) $(subst .c,$(OBJ),$(PYTHON_SOURCES4))
PYTHON_OBJECTS3 = $(subst .c,$(OBJ),$(PYTHON_SOURCES5)) $(subst .c,$(OBJ),$(PYTHON_SOURCES6))
PYTHON_OBJECTS4 = $(subst .c,$(OBJ),$(PYTHON_SOURCES7)) $(subst .c,$(OBJ),$(PYTHON_SOURCES8))
PYTHON_OBJECTS5 = $(subst .c,$(OBJ),$(PYTHON_SOURCES9)) $(subst .c,$(OBJ),$(PYTHON_SOURCES10))
PYTHON_OBJECTS6 = $(subst .c,$(OBJ),$(PYTHON_SOURCES11)) $(subst .c,$(OBJ),$(PYTHON_SOURCES12))
PYTHON_OBJECTS7 = $(subst .c,$(OBJ),$(PYTHON_SOURCES13)) $(subst .c,$(OBJ),$(PYTHON_SOURCES14))
PYTHON_OBJECTS8 = $(subst .c,$(OBJ),$(PYTHON_SOURCES15)) $(subst .c,$(OBJ),$(PYTHON_SOURCES16))

PYTHON_OUT = vendor$(SEP)$(LIB)python$(A)

$(PYTHON_OUT): $(PYTHON_OBJECTS1) $(PYTHON_OBJECTS2) $(PYTHON_OBJECTS3)
$(PYTHON_OUT): $(PYTHON_OBJECTS4) $(PYTHON_OBJECTS5) $(PYTHON_OBJECTS6)
$(PYTHON_OUT): $(PYTHON_OBJECTS7) $(PYTHON_OBJECTS8)
	$(AR) $@ $(ALL)

PGEN_SOURCES1 = $(PYTHON)pgenmain.c $(PYTHON)acceler.c $(PYTHON)fgetsintr.c
PGEN_SOURCES2 = $(PYTHON)grammar1.c $(PYTHON)intrcheck.c $(PYTHON)listnode.c
PGEN_SOURCES3 = $(PYTHON)node.c $(PYTHON)parser.c $(PYTHON)parsetok.c
PGEN_SOURCES4 = $(PYTHON)strdup.c $(PYTHON)tokenizer.c $(PYTHON)bitset.c
PGEN_SOURCES5 = $(PYTHON)firstsets.c $(PYTHON)grammar.c $(PYTHON)metagrammar.c
PGEN_SOURCES6 = $(PYTHON)pgen.c $(PYTHON)printgrammar.c

PGEN_OBJECTS1 = $(subst .c,$(OBJ),$(PGEN_SOURCES1)) $(subst .c,$(OBJ),$(PGEN_SOURCES2))
PGEN_OBJECTS2 = $(subst .c,$(OBJ),$(PGEN_SOURCES3)) $(subst .c,$(OBJ),$(PGEN_SOURCES4))
PGEN_OBJECTS3 = $(subst .c,$(OBJ),$(PGEN_SOURCES5)) $(subst .c,$(OBJ),$(PGEN_SOURCES6))

PGEN_OUT = $(PYTHON)pgenmain$(EXE)

$(PGEN_OUT): $(PGEN_OBJECTS1) $(PGEN_OBJECTS2) $(PGEN_OBJECTS3)
	$(CC) $(O) $(ALL)

PYGRAM = $(PYTHON)$(SEP)graminit.c $(PYTHON)$(SEP)graminit.h

$(PYGRAM): $(PYTHON)/gr/Grammar $(PGEN_OUT)
	$(CROSS_TOOL) $(PGEN_OUT) $(PYTHON)/gr/Grammar $(PYGRAM)

$(PYTHON)bltinmodule.c: $(PYGRAM)

clean_python:
	$(RM) $(PYTHON_OBJECTS1) $(PYTHON_OBJECTS2) $(PYTHON_OBJECTS3)
	$(RM) $(PYTHON_OBJECTS4) $(PYTHON_OBJECTS5) $(PYTHON_OBJECTS6)
	$(RM) $(PYTHON_OBJECTS7) $(PYTHON_OBJECTS8) $(PGEN_OUT)
	$(RM) $(PGEN_OBJECTS1) $(PGEN_OBJECTS2) $(PGEN_OBJECTS3) $(PYGRAM)
