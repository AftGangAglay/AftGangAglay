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

OUT = src/libafeirsa.a

SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard include/*.h) $(wildcard src/*.h)
OBJECTS = $(SOURCES:.c=.o)

OUT = src/main

CFLAGS += -Iinclude
CFLAGS += -std=c89 -Wall -Wextra -Werror -ansi -pedantic -pedantic-errors

LDLIBS += -ltiff -lm -lX11

CFLAGS += $(WWW_IFLAGS) $(PYTHON_IFLAGS) $(AFEIRSA_IFLAGS) $(LIBTIFF_IFLAGS)
LDLIBS += $(LIBWWW) $(LIBPYTHON) $(LIBAFEIRSA) $(LIBTIFF)

# glabi appends its own ldlibs
CFLAGS += $(GLABI)

ifndef WINDOWS
	CFLAGS += -D_POSIX_SOURCE
endif

ifdef DEBUG
	CFLAGS += -g -O0 -D_DEBUG
else
	CFLAGS += -DNDEBUG -Ofast
endif

.DEFAULT_GOAL := all
.PHONY: all
all: $(OUT)

$(OUT): $(OBJECTS) $(LIBWWW) $(LIBPYTHON) $(LIBAFEIRSA) $(LIBTIFF)

$(OBJECTS): $(HEADERS)

.PHONY: clean
clean:
	rm -f $(OBJECTS)
	rm -f $(OUT)
