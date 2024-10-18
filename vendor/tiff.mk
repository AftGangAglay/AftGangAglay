# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

TIF = vendor$(SEP)libtiff$(SEP)libtiff$(SEP)
TIFS = $(TIF)tif_
TIFH = $(TIFS)

TIF_SRC1 = $(TIFS)aux.c $(TIFS)close.c $(TIFS)codec.c $(TIFS)compress.c
TIF_SRC2 = $(TIFS)dir.c $(TIFS)dirinfo.c $(TIFS)dirread.c $(TIFS)dirwrite.c
TIF_SRC3 = $(TIFS)dumpmode.c $(TIFS)error.c $(TIFS)fax3.c $(TIFS)fax3sm.c
TIF_SRC4 = $(TIFS)getimage.c $(TIFS)jpeg.c $(TIFS)flush.c $(TIFS)lzw.c
TIF_SRC5 = $(TIFS)next.c $(TIFS)open.c $(TIFS)packbits.c $(TIFS)predict.c
TIF_SRC6 = $(TIFS)print.c $(TIFS)read.c $(TIFS)swab.c $(TIFS)strip.c
# TODO: Does `unix.c' need to be swapped out based on target? (probably).
TIF_SRC7 = $(TIFS)thunder.c $(TIFS)tile.c $(TIFS)unix.c $(TIFS)version.c
TIF_SRC8 = $(TIFS)warning.c $(TIFS)write.c $(TIFS)zip.c
TIF_SRC9 = $(TIF_SRC1) $(TIF_SRC2) $(TIF_SRC3) $(TIF_SRC4) $(TIF_SRC5)

TIF_SRC = $(TIF_SRC6) $(TIF_SRC7) $(TIF_SRC8) $(TIF_SRC9)
# TODO: HDR.
TIF_HDR =
TIF_OBJ = $(subst .c,$(OBJ),$(TIF_SRC))

TIF_OUT = vendor$(SEP)$(LIB)tiff$(A)

$(TIF_OBJ): $(TIF_HDR)

$(TIF_OUT): $(TIF_OBJ)
	$(AR)

# TODO: Make depend on appropriate `TIF_HDR' subset.
MKG3_SRC = $(TIF)mkg3states.c
MKG3_OBJ = $(subst .c,$(OBJ),$(MKG3_SRC))

MKG3_OUT = $(TIF)mkg3states$(EXE)

$(MKG3_OUT): $(MKG3_OBJ)
	$(CCLD)

# TODO: Is the `$(RM)' neccesary?
$(TIFS)fax3sm.c: $(MKG3_OUT)
	$(RM) $(TIFS)fax3sm.c
	$(CROSS_TOOL) $(MKG3_OUT) -c const $(TIFS)fax3sm.c

clean_tiff:
	$(RM) $(TIF_OBJ) $(TIF_OUT) $(MKG3_OBJ) $(MKG3_OUT) $(TIFS)fax3sm.c
