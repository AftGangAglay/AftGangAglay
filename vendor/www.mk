# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

WWW = vendor$(SEP)www$(SEP)Library$(SEP)Implementation$(SEP)
WWWH = $(WWW)

WWW_SRC1 = $(WWW)HTAccess.c $(WWW)HTAlert.c $(WWW)HTAnchor.c $(WWW)HTAtom.c
WWW_SRC2 = $(WWW)HTBTree.c $(WWW)HTChunk.c $(WWW)HTFile.c $(WWW)HTFormat.c
WWW_SRC3 = $(WWW)HTFTP.c $(WWW)HTFWriter.c $(WWW)HTGopher.c $(WWW)HTHistory.c
WWW_SRC4 = $(WWW)HTInit.c $(WWW)HTList.c $(WWW)HTMIME.c $(WWW)HTML.c
WWW_SRC5 = $(WWW)HTMLDTD.c $(WWW)HTMLGen.c $(WWW)HTNews.c $(WWW)HTParse.c
WWW_SRC6 = $(WWW)HTPlain.c $(WWW)HTRules.c $(WWW)HTString.c $(WWW)HTStyle.c
WWW_SRC7 = $(WWW)HTTCP.c $(WWW)HTTelnet.c $(WWW)HTTP.c $(WWW)HTWriter.c
WWW_SRC8 = $(WWW)HTWSRC.c $(WWW)SGML.c
WWW_SRC9 = $(WWW_SRC1) $(WWW_SRC2) $(WWW_SRC3) $(WWW_SRC4) $(WWW_SRC5)

WWW_HDR1 = $(WWWH)HTAccess.h $(WWWH)HTAlert.h $(WWWH)HTAnchor.h $(WWWH)HTAtom.h
WWW_HDR2 = $(WWWH)HTBTree.h $(WWWH)HTChunk.h $(WWWH)HText.h $(WWWH)HTFile.h
WWW_HDR3 = $(WWWH)HTFormat.h $(WWWH)HTFTP.h $(WWWH)HTFWriter.h
WWW_HDR4 = $(WWWH)HTGopher.h $(WWWH)HTHistory.h $(WWWH)HTInit.h $(WWWH)HTList.h
WWW_HDR5 = $(WWWH)HTMIME.h $(WWWH)HTML.h $(WWWH)HTMLDTD.h $(WWWH)HTMLGen.h
WWW_HDR6 = $(WWWH)HTNews.h $(WWWH)HTParse.h $(WWWH)HTPlain.h $(WWWH)HTRules.h
WWW_HDR7 = $(WWWH)HTStream.h $(WWWH)HTString.h $(WWWH)HTStyle.h $(WWWH)HTTCP.h
WWW_HDR8 = $(WWWH)HTTelnet.h $(WWWH)HTTP.h $(WWWH)HTUtils.h $(WWWH)HTWAIS.h
WWW_HDR9 = $(WWWH)HTWriter.h $(WWWH)HTWSRC.h $(WWWH)SGML.h $(WWWH)tcp.h
WWW_HDR10 = $(WWW_HDR1) $(WWW_HDR2) $(WWW_HDR3) $(WWW_HDR4) $(WWW_HDR5)

WWW_SRC = $(WWW_SRC6) $(WWW_SRC7) $(WWW_SRC8) $(WWW_SRC9)
WWW_HDR = $(WWW_HDR6) $(WWW_HDR7) $(WWW_HDR8) $(WWW_HDR9) $(WWW_HDR10)
WWW_OBJ = $(subst .c,$(OBJ),$(WWW_SRC))

WWW_OUT = vendor$(SEP)$(LIB)www$(A)

$(WWW_OBJ): $(WWW_HDR)

$(WWW_OUT): $(WWW_OBJ)
	$(AR) $@ $(ALL)

clean_www:
	$(RM) $(WWW_OBJ) $(WWW_OUT)
