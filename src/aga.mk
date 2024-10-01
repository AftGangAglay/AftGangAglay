# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

AGA = src$(SEP)aga$(SEP)
AGAH = include$(SEP)aga$(SEP)

AGAN = src$(SEP)agan$(SEP)
AGANH = include$(SEP)agan$(SEP)

# aga
AGA1 = $(AGA)config.c $(AGA)draw.c $(AGA)midi.c $(AGA)utility.c $(AGA)pack.c
AGA2 = $(AGA)log.c $(AGA)python.c $(AGA)script.c $(AGA)startup.c
AGA3 = $(AGA)sound.c $(AGA)win32.c $(AGA)aga.c $(AGA)window.c $(AGA)error.c
AGA4 = $(AGA)render.c $(AGA)result.c $(AGA)io.c $(AGA)build.c $(AGA)graph.c
# agan
AGA5 = $(AGAN)draw.c $(AGAN)utility.c $(AGAN)agan.c $(AGAN)object.c
AGA6 = $(AGAN)math.c $(AGAN)editor.c $(AGAN)io.c

# aga
AGAH1 = $(AGAH)config.h $(AGAH)environment.h $(AGAH)error.h $(AGAH)utility.h
AGAH2 = $(AGAH)gl.h $(AGAH)io.h $(AGAH)log.h $(AGAH)result.h $(AGAH)script.h
AGAH3 = $(AGAH)python.h $(AGAH)sound.h $(AGAH)startup.h $(AGAH)render.h
AGAH4 = $(AGAH)std.h $(AGAH)win32.h $(AGAH)window.h $(AGAH)pack.h $(AGAH)draw.h
AGAH5 = $(AGAH)graph.h
# agan
AGAH6 = $(AGANH)agan.h $(AGANH)object.h $(AGANH)draw.h $(AGAH)render.h
AGAH7 = $(AGANH)utility.h $(AGANH)io.h

AGA_SRC = $(AGA1) $(AGA2) $(AGA3) $(AGA4) $(AGA5) $(AGA6)
AGA_HDR = $(AGAH1) $(AGAH2) $(AGAH3) $(AGAH4) $(AGAH5) $(AGAH6) $(AGAH7)
AGA_OBJ = $(subst .c,$(OBJ),$(AGA_SRC))

AGA_OUT = $(AGA)aga$(EXE)

$(AGA_OBJ): $(APRO_HDR) $(PY_HDR) $(WWW_HDR) $(AGA_HDR)

$(AGA_OUT): $(APRO_OUT) $(PY_OUT) $(WWW_OUT)
$(AGA_OUT): $(AGA_OBJ)
	$(CCLD) $(WL) $(LDFLAGS) $(LDLIBS) $(SET_LDFLAGS) $(SET_LDLIBS) $(GL_LDFLAGS) $(GL_LDLIBS)

$(AGA)script.c: $(PYGRAM)

$(AGA)window$(OBJ): $(AGA)xwindow.h
$(AGA)window$(OBJ): $(AGA)win32window.h

$(AGA)midi$(OBJ): $(AGA)win32midi.h

clean_aga:
	$(RM) $(AGA_OBJ) $(AGA_OUT)
