# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PY = vendor$(SEP)python$(SEP)src$(SEP)
PYO = $(PY)object$(SEP)
PYM = $(PY)module$(SEP)

PYI = vendor$(SEP)python$(SEP)include$(SEP)

PYH = $(PYI)python$(SEP)
PYOH = $(PYH)object$(SEP)
PYMH = $(PYH)module$(SEP)

PY1 = $(PY)acceler.c $(PY)ceval.c $(PY)compile.c $(PY)errors.c $(PY)graminit.c
PY2 = $(PY)import.c $(PY)listnode.c $(PY)state.c $(PY)types.c $(PY)node.c
PY3 = $(PY)object.c $(PY)parser.c $(PY)parsetok.c $(PY)tokenizer.c
PY4 = $(PY)traceback.c $(PY)pythonmain.c $(PY)evalops.c

PYO1 = $(PYO)list.c $(PYO)dict.c $(PYO)float.c $(PYO)frame.c $(PYO)func.c
PYO2 = $(PYO)class.c $(PYO)method.c $(PYO)module.c $(PYO)string.c $(PYO)tuple.c
PYO3 = $(PYO)int.c

PYM1 = $(PYM)builtin.c $(PYM)math.c

PYH1 = $(PYH)bitset.h $(PYH)ceval.h $(PYH)opcode.h $(PYH)import.h $(PYH)types.h
PYH2 = $(PYH)compile.h $(PYH)env.h $(PYH)errors.h $(PYH)state.h $(PYH)token.h
PYH3 = $(PYH)metagrammar.h $(PYH)node.h $(PYH)object.h $(PYH)grammar.h
PYH4 = $(PYH)parser.h $(PYH)pgen.h $(PYH)result.h $(PYH)std.h $(PYH)parsetok.h
PYH5 = $(PYH)tokenizer.h $(PYH)traceback.h
PYH6 = $(PYH1) $(PYH2) $(PYH3) $(PYH4) $(PYH5)

PYOH1 = $(PYOH)list.h $(PYOH)dict.h $(PYOH)float.h $(PYOH)frame.h $(PYOH)func.h
PYOH2 = $(PYOH)class.h $(PYOH)method.h $(PYOH)module.h $(PYOH)string.h
PYOH3 = $(PYOH)int.h $(PYOH)tuple.h

PYMH1 = $(PYMH)builtin.h $(PYMH)math.h

PY_SRC = $(PY1) $(PY2) $(PY3) $(PY4) $(PYO1) $(PYO2) $(PYO3) $(PYM1)
PY_HDR = $(PYH6) $(PYOH1) $(PYOH2) $(PYOH3) $(PYMH1)
PY_OBJ = $(subst .c,$(OBJ),$(PY_SRC))

PY_OUT = vendor$(SEP)$(LIB)python$(A)

$(PY_OBJ): $(PY_HDR)

$(PY_OUT): $(APRO_OUT)
$(PY_OUT): $(PY_OBJ)
	$(AR)

PGEN1 = $(PY)pgenmain.c $(PY)acceler.c $(PY)listnode.c $(PY)printgrammar.c
PGEN2 = $(PY)node.c $(PY)parser.c $(PY)parsetok.c $(PY)tokenizer.c $(PY)pgen.c
PGEN3 = $(PY)bitset.c $(PY)firstsets.c $(PY)grammar.c $(PY)metagrammar.c

PGEN_SRC = $(PGEN1) $(PGEN2) $(PGEN3)
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
	$(RM) $(PY_OBJ) $(PY_OUT) $(PGEN_OBJ) $(PYGRAM) $(PGEN_OUT)
