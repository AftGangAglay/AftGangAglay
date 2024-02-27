# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

AGA_SOURCES1 = agaconf.c agadraw.c agaerr.c agaio.c agalog.c agapack.c agapy.c
AGA_SOURCES2 = agascript.c agasnd.c agastartup.c agastd.c agautil.c agaw32.c
AGA_SOURCES3 = agawin.c main.c

AGAN_SOURCES1 = agan.c aganobj.c agascriptglue.c

AGA_OBJECTS1 = $(AGA_SOURCES1:.c=$(OBJ)) $(AGA_SOURCES2:.c=$(OBJ))
AGA_OBJECTS2 = $(AGA_SOURCES3:.c=$(OBJ)) $(AGAN_SOURCES1:.c=$(OBJ))

AGA_OUT = src
