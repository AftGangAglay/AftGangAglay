# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

TIF = vendor$(SEP)libtiff$(SEP)libtiff$(SEP)
TIFI = $(TIF)
TIFS = $(TIF)tif_
TIFH = $(TIFS)

TIF_SRC1 = $(TIFS)aux.c $(TIFS)close.c $(TIFS)compress.c $(TIFS)codec.c
TIF_SRC2 = $(TIFS)dir.c $(TIFS)dirinfo.c $(TIFS)dirread.c $(TIFS)dirwrite.c
TIF_SRC3 = $(TIFS)dumpmode.c $(TIFS)error.c
TIF_SRC4 = $(TIFS)getimage.c $(TIFS)flush.c $(TIFS)lzw.c
TIF_SRC5 = $(TIFS)next.c $(TIFS)open.c $(TIFS)packbits.c $(TIFS)predict.c
TIF_SRC6 = $(TIFS)read.c $(TIFS)swab.c $(TIFS)strip.c
TIF_SRC7 = $(TIFS)thunder.c $(TIFS)tile.c $(TIFS)platform.c $(TIFS)version.c
TIF_SRC8 = $(TIFS)warning.c $(TIFS)write.c
TIF_SRC9 = $(TIF_SRC1) $(TIF_SRC2) $(TIF_SRC3) $(TIF_SRC4) $(TIF_SRC5)

TIF_SRC = $(TIF_SRC6) $(TIF_SRC7) $(TIF_SRC8) $(TIF_SRC9)
# TODO: HDR.
TIF_HDR =
TIF_OBJ = $(subst .c,$(OBJ),$(TIF_SRC))

TIF_OUT = vendor$(SEP)$(LIB)tiff$(A)

$(TIF_OBJ): $(TIF_HDR)

$(TIF_OUT): $(TIF_OBJ)
	$(AR)

# TODO: This doesn't seem to be working?
$(TIFS)platform.c: $(TIFS)win3.c
$(TIFS)platform.c: $(TIFS)unix.c

clean_tiff:
	$(RM) $(TIF_OBJ) $(TIF_OUT)
