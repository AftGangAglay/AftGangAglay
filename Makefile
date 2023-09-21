# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

(%): %
%.a:
	$(AR) $(ARFLAGS) $@ $?
	ranlib $@

.PHONY: SUBMAKE
SUBMAKE:

GLXABI = 1

include vendor/www.mk
include vendor/python.mk
include vendor/afeirsa.mk
include vendor/libtiff.mk
include res/res.mk

SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard include/*.h) $(wildcard src/*.h)
OBJECTS = $(SOURCES:.c=.o)

OUT = src/main

CFLAGS += -Iinclude
CFLAGS += -std=c89 -Wall -Wextra -Werror -ansi -pedantic -pedantic-errors

LDLIBS += -lm -lX11

CFLAGS += $(WWW_IFLAGS) $(PYTHON_IFLAGS) $(AFEIRSA_IFLAGS) $(LIBTIFF_IFLAGS)
LDLIBS += $(LIBWWW) $(LIBPYTHON) $(LIBAFEIRSA) $(LIBTIFF)

# glabi appends its own ldlibs
CFLAGS += $(GLABI)

ifndef WINDOWS
	CFLAGS += -D_POSIX_SOURCE -DAGA_HAVE_UNIX
endif

ifdef APPLE
	NOSND = 1
endif

ifdef DEBUG
	CFLAGS += -g -O0 -D_DEBUG
else
	CFLAGS += -DNDEBUG -Ofast
endif

ifdef NOSND
	CFLAGS += -DAGA_NOSND
endif

.DEFAULT_GOAL := all
.PHONY: all
all: $(OUT)

$(OUT): $(OBJECTS) $(LDLIBS)

$(OBJECTS): $(HEADERS)

.PHONY: clean
clean:
	rm -f $(OBJECTS)
	rm -f $(OUT)
