# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PY = vendor$(SEP)python$(SEP)src$(SEP)
PYI = vendor$(SEP)python$(SEP)include$(SEP)
PYH = vendor$(SEP)python$(SEP)include$(SEP)python$(SEP)

PY_SRC1 = $(PY)acceler.c $(PY)bltinmodule.c $(PY)ceval.c $(PY)classobject.c
PY_SRC2 = $(PY)compile.c $(PY)dictobject.c $(PY)errors.c $(PY)floatobject.c
PY_SRC3 = $(PY)frameobject.c $(PY)funcobject.c $(PY)graminit.c $(PY)grammar1.c
PY_SRC4 = $(PY)import.c $(PY)intobject.c $(PY)listnode.c $(PY)listobject.c
PY_SRC5 = $(PY)mathmodule.c $(PY)methodobject.c $(PY)modsupport.c
PY_SRC6 = $(PY)moduleobject.c $(PY)node.c $(PY)object.c $(PY)parser.c
PY_SRC7 = $(PY)parsetok.c $(PY)stringobject.c $(PY)tokenizer.c $(PY)traceback.c
PY_SRC8 = $(PY)pythonmain.c $(PY)tupleobject.c $(PY)types.c
PY_SRC9 = $(PY_SRC1) $(PY_SRC2) $(PY_SRC3) $(PY_SRC4) $(PY_SRC5) $(PY_SRC6)
PY_SRC10 = $(PY_SRC7) $(PY_SRC8)

PY_HDR1 = $(PYH)bitset.h $(PYH)bltinmodule.h $(PYH)ceval.h $(PYH)classobject.h
PY_HDR2 = $(PYH)compile.h $(PYH)dictobject.h $(PYH)env.h $(PYH)errors.h
PY_HDR3 = $(PYH)floatobject.h $(PYH)frameobject.h $(PYH)funcobject.h
PY_HDR4 = $(PYH)grammar.h $(PYH)import.h $(PYH)intobject.h
PY_HDR5 = $(PYH)listobject.h $(PYH)mathmodule.h $(PYH)metagrammar.h
PY_HDR6 = $(PYH)methodobject.h $(PYH)modsupport.h $(PYH)moduleobject.h
PY_HDR7 = $(PYH)node.h $(PYH)object.h $(PYH)opcode.h $(PYH)parser.h
PY_HDR8 = $(PYH)parsetok.h $(PYH)pgen.h $(PYH)result.h $(PYH)std.h
PY_HDR9 = $(PYH)stringobject.h $(PYH)token.h $(PYH)tokenizer.h
PY_HDR10 = $(PYH)traceback.h $(PYH)tupleobject.h
PY_HDR11 = $(PY_HDR1) $(PY_HDR2) $(PY_HDR3) $(PY_HDR4) $(PY_HDR5) $(PY_HDR6)
PY_HDR12 = $(PY_HDR7) $(PY_HDR8) $(PY_HDR9) $(PY_HDR10)

PY_SRC = $(PY_SRC9) $(PY_SRC10)
PY_HDR = $(PY_HDR11) $(PY_HDR12)
PY_OBJ = $(subst .c,$(OBJ),$(PY_SRC))

PY_OUT = vendor$(SEP)$(LIB)python$(A)

$(PY_OBJ): $(PY_HDR)

$(PY_OUT): $(APRO_OUT)
$(PY_OUT): $(PY_OBJ)
	$(AR)

PGEN_SRC1 = $(PY)pgenmain.c $(PY)acceler.c $(PY)grammar1.c $(PY)listnode.c
PGEN_SRC2 = $(PY)node.c $(PY)parser.c $(PY)parsetok.c $(PY)tokenizer.c
PGEN_SRC3 = $(PY)bitset.c $(PY)firstsets.c $(PY)grammar.c $(PY)metagrammar.c
PGEN_SRC4 = $(PY)pgen.c $(PY)printgrammar.c

PGEN_SRC = $(PGEN_SRC1) $(PGEN_SRC2) $(PGEN_SRC3) $(PGEN_SRC4)
PGEN_OBJ = $(subst .c,$(OBJ),$(PGEN_SRC))

PGEN_OUT = $(PY)pgenmain$(EXE)

$(PGEN_SRC): $(PY_HDR)

$(PGEN_OUT): $(PGEN_OBJ)
	$(CCLD) $(WL) $(LDFLAGS) $(LDLIBS) $(SET_LDFLAGS) $(SET_LDLIBS)

PYGRAM = $(PY)graminit.c $(PYI)python$(SEP)graminit.h

PYGRAM_SOURCE = $(PY)gr$(SEP)Grammar

$(PYGRAM): $(PYGRAM_SOURCE) $(PGEN_OUT)
	$(CROSS_TOOL) $(PGEN_OUT) $(PYGRAM_SOURCE) $(PYGRAM)

$(PY)compile.c: $(PYGRAM)
$(PY)import.c: $(PYGRAM)

clean_python:
	$(RM) $(PY_OBJ) $(PGEN_OBJ) $(PYGRAM) $(PGEN_OUT)
