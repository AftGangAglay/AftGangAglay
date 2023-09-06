# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

# This is a reduced version of libwww as we aren't really running on a
# period-accurate UNIX workstation (probably).

WWW_ROOT = vendor/WWW

LIBWWW = vendor/libwww.a
WWW_IFLAGS = -I$(WWW_ROOT)/Library/Implementation

WWW_SOURCEROOT = $(WWW_ROOT)/Library/Implementation
WWW_SOURCES = $(wildcard $(WWW_SOURCEROOT)/*.c)
WWW_SOURCES := $(filter-out $(WWW_SOURCEROOT)/HTWAIS.c,$(WWW_SOURCES))
WWW_OBJECTS = $(WWW_SOURCES:.c=.o)

WWW_CFLAGS = -std=c89 -ansi -D_SVID_SOURCE -w

all: $(LIBWWW)

.NOTPARALLEL: $(LIBWWW)
$(LIBWWW): $(LIBWWW)($(WWW_OBJECTS))
	ranlib $@

$(WWW_OBJECTS): CFLAGS = $(WWW_CFLAGS)
$(WWW_OBJECTS): $(WWW_SOURCES)

clean: clean_www
.PHONY: clean_www
clean_www:
	rm -rf $(WWW_OBJECTS)
	rm -rf $(LIBWWW)
