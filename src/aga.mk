# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

AGA = src$(SEP)

AGA_SOURCES1 = $(AGA)agaconf.c $(AGA)agadraw.c $(AGA)agaerr.c $(AGA)agaio.c
AGA_SOURCES2 = $(AGA)agalog.c $(AGA)agapack.c $(AGA)agapy.c $(AGA)agascript.c
AGA_SOURCES3 = $(AGA)agasnd.c $(AGA)agastartup.c $(AGA)agastd.c $(AGA)agautil.c
AGA_SOURCES4 = $(AGA)agaw32.c $(AGA)agawin.c $(AGA)main.c

AGA_OBJECTS1 = $(subst .c,$(OBJ),$(AGA_SOURCES1)) $(subst .c,$(OBJ),$(AGA_SOURCES2))
AGA_OBJECTS2 = $(subst .c,$(OBJ),$(AGA_SOURCES3)) $(subst .c,$(OBJ),$(AGA_SOURCES4))

AGAN = src$(SEP)agan$(SEP)

AGAN_SOURCES1 = $(AGAN)agan.c $(AGAN)aganobj.c $(AGAN)agascriptglue.c

AGAN_OBJECTS1 = $(subst .c,$(OBJ),$(AGAN_SOURCES1))

AGARC_SOURCES1 = res$(SEP)aga.rc

AGARC_OBJECTS1 = $(subst .c,$(OBJ),$(AGARC_SOURCES))

AGA_OUT = src$(SEP)main$(EXE)

$(AGA_OUT): $(PYTHON_OUT) $(WWW_OUT) $(APRO_OUT)
$(AGA_OUT): $(AGA_OBJECTS1) $(AGA_OBJECTS2) $(AGAN_OBJECTS1) $(AGARC_OBJECTS1)
	$(CC) $(O) $(ALL) $(WL) $(LDFLAGS) $(LDLIBS) $(GL_LDFLAGS) $(GL_LDLIBS)

$(AGA)agan$(SEP)agan.c: $(PYGRAM)
$(AGA)agan$(SEP)aganobj.c: $(PYGRAM)
$(AGA)agan$(SEP)agascriptglue.c: $(PYGRAM)

$(AGA)agapy.c: $(PYGRAM)
$(AGA)agascript.c: $(PYGRAM)

$(AGA)agawin$(OBJ): $(AGA)agaxwin.h
$(AGA)agawin$(OBJ): $(AGA)agawwin.h

clean_aga:
	$(RM) $(AGA_OBJECTS1) $(AGA_OBJECTS2) $(AGAN_OBJECTS1) $(AGA_OUT)
