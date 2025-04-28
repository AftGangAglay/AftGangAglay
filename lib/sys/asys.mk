# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

# NOTE: See Win3 Guide section 14.5 for useful implementation info:
#		http://www.bitsavers.org/pdf/microsoft/windows_3.1
#		Windows_3.1_Guide_to_Programming_1992.pdf, Page 331
#		Also see here: https://winasm.tripod.com/Clib.html

ASYS = lib$(SEP)sys$(SEP)
ASYS_INCLUDE = $(ASYS)include$(SEP)

ASYSH = $(ASYS_INCLUDE)asys$(SEP)

ASYS1 = $(ASYS)stream.c $(ASYS)result.c $(ASYS)string.c $(ASYS)memory.c
ASYS2 = $(ASYS)error.c $(ASYS)log.c $(ASYS)file.c $(ASYS)detail.c
ASYS3 = $(ASYS)getopt.c $(ASYS)math.c

ASYSH1 = $(ASYSH)base.h $(ASYSH)stream.h $(ASYSH)result.h $(ASYSH)system.h
ASYSH2 = $(ASYSH)string.h $(ASYSH)memory.h $(ASYSH)error.h $(ASYSH)log.h
ASYSH3 = $(ASYSH)varargs.h $(ASYSH)file.h $(ASYSH)getopt.h $(ASYSH)main.h

ASYS_SRC = $(ASYS1) $(ASYS2) $(ASYS3)
ASYS_HDR = $(ASYSH1) $(ASYSH2) $(ASYSH3)
ASYS_OBJ = $(subst .c,$(OBJ),$(ASYS_SRC))

ASYS_OUT = lib$(SEP)$(LIB)asys$(A)

$(ASYS_OBJ): $(ASYS_HDR)

$(ASYS_OUT): $(ASYS_OBJ)
	$(STATIC)

clean_asys:
	$(RM) $(ASYS_OBJ) $(ASYS_OUT)
