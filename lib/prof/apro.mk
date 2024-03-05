# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

APRO = lib$(SEP)prof$(SEP)

APRO_SOURCES1 = $(APRO)apro.c

APRO_OBJECTS1 = $(subst .c,$(OBJ),$(APRO_SOURCES1))

APRO_OUT = lib$(SEP)$(LIB)apro$(A)

$(APRO_OUT): $(APRO_OBJECTS1)
	$(AR) $@ $(ALL)

clean_apro:
	$(RM) $(APRO_OBJECTS1) $(APRO_OUT)
