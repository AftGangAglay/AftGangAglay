# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

APRO = lib$(SEP)prof$(SEP)

APRO_SRC = $(APRO)apro.c
APRO_HDR = $(APRO)apro.h
APRO_OBJ = $(subst .c,$(OBJ),$(APRO_SRC))

APRO_OUT = lib$(SEP)$(LIB)apro$(A)

$(APRO_OBJ): $(APRO_HDR)

$(APRO_OUT): $(APRO_OBJ)
	$(AR) $@ $(ALL)

clean_apro:
	$(RM) $(APRO_OBJ) $(APRO_OUT)
