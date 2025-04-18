# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

TIFF = vendor$(SEP)libtiff$(SEP)libtiff$(SEP)
TIFF_INCLUDE = $(TIFF)
TIFFS = $(TIFF)tif_

TIFF_SRC1 = $(TIFFS)close.c $(TIFFS)compress.c $(TIFFS)codec.c $(TIFFS)strip.c
TIFF_SRC2 = $(TIFFS)dirinfo.c $(TIFFS)dirread.c $(TIFFS)warning.c $(TIFFS)aux.c
TIFF_SRC3 = $(TIFFS)dumpmode.c $(TIFFS)error.c $(TIFFS)dir.c $(TIFFS)write.c
TIFF_SRC4 = $(TIFFS)getimage.c $(TIFFS)flush.c $(TIFFS)lzw.c $(TIFFS)dirwrite.c
TIFF_SRC5 = $(TIFFS)next.c $(TIFFS)open.c $(TIFFS)packbits.c $(TIFFS)predict.c
TIFF_SRC6 = $(TIFFS)read.c $(TIFFS)swab.c $(TIFFS)version.c $(TIFFS)platform.c
TIFF_SRC7 = $(TIFFS)thunder.c $(TIFFS)tile.c
TIFF_SRC8 = $(TIFF_SRC1) $(TIFF_SRC2) $(TIFF_SRC3) $(TIFF_SRC4) $(TIFF_SRC5)

TIFF_SRC = $(TIFF_SRC6) $(TIFF_SRC7) $(TIFF_SRC8)
# TODO: HDR.
TIFF_HDR =
TIFF_OBJ = $(subst .c,$(OBJ),$(TIFF_SRC))

TIFF_OUT = vendor$(SEP)$(LIB)tiff$(A)

$(TIFF_OBJ): $(TIFF_HDR)

$(TIFF_OUT): $(TIFF_OBJ)
	$(STATIC)

# TODO: This doesn't seem to be working?
$(TIFFS)platform$(OBJ): $(TIFFS)win3.c
$(TIFFS)platform$(OBJ): $(TIFFS)unix.c
$(TIFFS)platform$(OBJ): $(TIFFS)stdc.c

clean_tiff:
	$(RM) $(TIFF_OBJ) $(TIFF_OUT)
