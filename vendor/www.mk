# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

WWW_ROOT = vendor/www

LIBWWW = vendor/libwww.a
WWW_IFLAGS = -isystem $(WWW_ROOT)/Library/Implementation

WWW_SOURCEROOT = $(WWW_ROOT)/Library/Implementation
WWW_SOURCES = $(wildcard $(WWW_SOURCEROOT)/*.c)
WWW_DISABLED = $(WWW_SOURCEROOT)/HTWAIS.c
WWW_SOURCES := $(filter-out $(WWW_DISABLED),$(WWW_SOURCES))
WWW_OBJECTS = $(WWW_SOURCES:.c=.o)

WWW_CFLAGS = -std=c89 -ansi -D_SVID_SOURCE -w
ifdef WINDOWS
	WWW_CFLAGS += -D_WINDOWS -DTCP_INCLUDES_DONE -DNO_GROUPS -include winsock.h
endif

# `clang' treats this as an error and doesn't disable it with `-w'.
WWW_CFLAGS += -Wno-incompatible-function-pointer-types

ifdef APPLE
	WWW_CFLAGS += -DNeXT
endif

ifdef DEBUG
	WWW_CFLAGS += -g -D_DEBUG -DDEBUG
else
	WWW_CFLAGS += -DNDEBUG -O
endif

all: $(LIBWWW)

$(LIBWWW): CFLAGS = $(WWW_CFLAGS)
$(LIBWWW): $(WWW_OBJECTS)

clean: clean_www
.PHONY: clean_www
clean_www:
	$(call PATHREM,$(WWW_OBJECTS))
	$(call PATHREM,$(LIBWWW))
