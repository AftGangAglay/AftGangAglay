# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PYINC = vendor$(SEP)python$(SEP)include$(SEP)
PY = vendor$(SEP)python$(SEP)src$(SEP)

# TODO: Reformat this list.
PYTHON_SOURCES1 = $(PY)acceler.c $(PY)bltinmodule.c $(PY)ceval.c
PYTHON_SOURCES2 = $(PY)classobject.c $(PY)compile.c
PYTHON_SOURCES3 = $(PY)dictobject.c $(PY)errors.c $(PY)fgetsintr.c
PYTHON_SOURCES4 = $(PY)fileobject.c $(PY)floatobject.c
PYTHON_SOURCES5 = $(PY)frameobject.c $(PY)funcobject.c
PYTHON_SOURCES6 = $(PY)graminit.c $(PY)grammar1.c $(PY)import.c
PYTHON_SOURCES7 = $(PY)intobject.c $(PY)intrcheck.c $(PY)listnode.c
PYTHON_SOURCES8 = $(PY)listobject.c $(PY)mathmodule.c
PYTHON_SOURCES9 = $(PY)methodobject.c $(PY)modsupport.c
PYTHON_SOURCES10 = $(PY)moduleobject.c $(PY)node.c $(PY)object.c
PYTHON_SOURCES11 = $(PY)parser.c $(PY)parsetok.c $(PY)posixmodule.c
PYTHON_SOURCES12 =
PYTHON_SOURCES13 = $(PY)stringobject.c $(PY)structmember.c
PYTHON_SOURCES14 = $(PY)sysmodule.c $(PY)timemodule.c $(PY)config.c
PYTHON_SOURCES15 = $(PY)tokenizer.c $(PY)traceback.c $(PY)pythonmain.c
PYTHON_SOURCES16 = $(PY)tupleobject.c $(PY)typeobject.c

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

PGEN_SOURCES1 = $(PY)pgenmain.c $(PY)acceler.c $(PY)fgetsintr.c
PGEN_SOURCES2 = $(PY)grammar1.c $(PY)intrcheck.c $(PY)listnode.c
PGEN_SOURCES3 = $(PY)node.c $(PY)parser.c $(PY)parsetok.c
PGEN_SOURCES4 = $(PY)tokenizer.c $(PY)bitset.c
PGEN_SOURCES5 = $(PY)firstsets.c $(PY)grammar.c $(PY)metagrammar.c
PGEN_SOURCES6 = $(PY)pgen.c $(PY)printgrammar.c

PGEN_OBJECTS1 = $(subst .c,$(OBJ),$(PGEN_SOURCES1)) $(subst .c,$(OBJ),$(PGEN_SOURCES2))
PGEN_OBJECTS2 = $(subst .c,$(OBJ),$(PGEN_SOURCES3)) $(subst .c,$(OBJ),$(PGEN_SOURCES4))
PGEN_OBJECTS3 = $(subst .c,$(OBJ),$(PGEN_SOURCES5)) $(subst .c,$(OBJ),$(PGEN_SOURCES6))

PGEN_OUT = $(PY)pgenmain$(EXE)

$(PGEN_OUT): $(PGEN_OBJECTS1) $(PGEN_OBJECTS2) $(PGEN_OBJECTS3)
	$(CC) $(O) $(ALL)

PYGRAM = $(PY)graminit.c $(PYINC)python$(SEP)graminit.h

PYGRAM_SOURCE = $(PY)gr$(SEP)Grammar

$(PYGRAM): $(PYGRAM_SOURCE) $(PGEN_OUT)
	$(CROSS_TOOL) $(PGEN_OUT) $(PYGRAM_SOURCE) $(PYGRAM)

$(PY)bltinmodule.c: $(PYGRAM)
$(PY)compile.c: $(PYGRAM)

clean_python:
	$(RM) $(PYTHON_OBJECTS1) $(PYTHON_OBJECTS2) $(PYTHON_OBJECTS3)
	$(RM) $(PYTHON_OBJECTS4) $(PYTHON_OBJECTS5) $(PYTHON_OBJECTS6)
	$(RM) $(PYTHON_OBJECTS7) $(PYTHON_OBJECTS8) $(PGEN_OUT)
	$(RM) $(PGEN_OBJECTS1) $(PGEN_OBJECTS2) $(PGEN_OBJECTS3) $(PYGRAM)
