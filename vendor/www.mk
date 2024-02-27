# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

WWW = vendor$(SEP)www$(SEP)Library$(SEP)Implementation$(SEP)

WWW_SOURCES1 = $(WWW)HTAccess.c $(WWW)HTAlert.c $(WWW)HTAnchor.c
WWW_SOURCES2 = $(WWW)HTAtom.c $(WWW)HTBTree.c $(WWW)HTChunk.c $(WWW)HTFile.c
WWW_SOURCES3 = $(WWW)HTFormat.c $(WWW)HTFTP.c $(WWW)HTFWriter.c
WWW_SOURCES4 = $(WWW)HTGopher.c $(WWW)HTHistory.c $(WWW)HTInit.c
WWW_SOURCES5 = $(WWW)HTList.c $(WWW)HTMIME.c $(WWW)HTML.c $(WWW)HTMLDTD.c
WWW_SOURCES6 = $(WWW)HTMLGen.c $(WWW)HTNews.c $(WWW)HTParse.c $(WWW)HTPlain.c
WWW_SOURCES7 = $(WWW)HTRules.c $(WWW)HTString.c $(WWW)HTStyle.c $(WWW)HTTCP.c
WWW_SOURCES8 = $(WWW)HTTelnet.c $(WWW)HTTP.c $(WWW)HTWriter.c $(WWW)HTWSRC.c
WWW_SOURCES9 = $(WWW)SGML.c

WWW_OBJECTS1 = $(subst .c,$(OBJ),$(WWW_SOURCES1)) $(subst .c,$(OBJ),$(WWW_SOURCES2))
WWW_OBJECTS2 = $(subst .c,$(OBJ),$(WWW_SOURCES3)) $(subst .c,$(OBJ),$(WWW_SOURCES4))
WWW_OBJECTS3 = $(subst .c,$(OBJ),$(WWW_SOURCES5)) $(subst .c,$(OBJ),$(WWW_SOURCES6))
WWW_OBJECTS4 = $(subst .c,$(OBJ),$(WWW_SOURCES7)) $(subst .c,$(OBJ),$(WWW_SOURCES8))
WWW_OBJECTS5 = $(subst .c,$(OBJ),$(WWW_SOURCES9))

WWW_OUT = vendor$(SEP)$(LIB)www$(A)

$(WWW_OUT): $(WWW_OBJECTS1) $(WWW_OBJECTS2) $(WWW_OBJECTS3) $(WWW_OBJECTS4)
$(WWW_OUT): $(WWW_OBJECTS5)
	$(AR) $@ $(ALL)

clean_www:
	$(RM) $(WWW_OBJECTS1) $(WWW_OBJECTS2) $(WWW_OBJECTS3) $(WWW_OBJECTS4)
	$(RM) $(WWW_OBJECTS5) $(WWW_OUT)
