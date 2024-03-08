# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

AGA = src$(SEP)
AGAH = include$(SEP)

AGAN = src$(SEP)agan$(SEP)
AGANH = include$(SEP)agan$(SEP)

AGA_SRC1 = $(AGA)agaconf.c $(AGA)agadraw.c $(AGA)agaerr.c $(AGA)agaio.c
AGA_SRC2 = $(AGA)agalog.c $(AGA)agapack.c $(AGA)agapy.c $(AGA)agascript.c
AGA_SRC3 = $(AGA)agasnd.c $(AGA)agastartup.c $(AGA)agautil.c $(AGA)agaw32.c
AGA_SRC4 = $(AGA)agawin.c $(AGA)aga.c $(AGAN)agan.c $(AGAN)aganobj.c
AGA_SRC5 = $(AGAN)agascriptglue.c

AGA_HDR1 = $(AGAH)agaconf.h $(AGAH)agadraw.h $(AGAH)agaenv.h $(AGAH)agaerr.h
AGA_HDR2 = $(AGAH)agagl.h $(AGAH)agaio.h $(AGAH)agalog.h $(AGAH)agapack.h
AGA_HDR3 = $(AGAH)agapyinc.h $(AGAH)agaresult.h $(AGAH)agascript.h
AGA_HDR4 = $(AGAH)agascripthelp.h $(AGAH)agasnd.h $(AGAH)agastartup.h
AGA_HDR5 = $(AGAH)agastd.h $(AGAH)agautil.h $(AGAH)agaw32.h $(AGAH)agawin.h
AGA_HDR6 = $(AGANH)agan.h $(AGANH)aganobj.h
AGA_HDR7 = $(AGA_HDR1) $(AGA_HDR2) $(AGA_HDR3) $(AGA_HDR4) $(AGA_HDR5)

AGA_SRC = $(AGA_SRC1) $(AGA_SRC2) $(AGA_SRC3) $(AGA_SRC4) $(AGA_SRC5)
AGA_HDR = $(AGA_HDR6) $(AGA_HDR7)
AGA_OBJ = $(subst .c,$(OBJ),$(AGA_SRC))

AGA_OUT = src$(SEP)aga$(EXE)

$(AGA_OBJ): $(APRO_HDR) $(PY_HDR) $(WWW_OUT) $(AGA_HDR)

$(AGA_OUT): $(PY_OUT) $(WWW_OUT) $(APRO_OUT)
$(AGA_OUT): $(AGA_OBJ)
	$(CCLD) $(WL) $(LDFLAGS) $(LDLIBS) $(SET_LDFLAGS) $(SET_LDLIBS) $(GL_LDFLAGS) $(GL_LDLIBS)

$(AGA)agascript.c: $(PYGRAM)

$(AGA)agawin$(OBJ): $(AGA)agaxwin.h
$(AGA)agawin$(OBJ): $(AGA)agawwin.h

clean_aga:
	$(RM) $(AGA_OBJ) $(AGA_OUT)
