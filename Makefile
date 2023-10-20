# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

ifndef WINDOWS
	GLXABI = 1
endif

ifdef APPLE
	NOSND = 1
endif
ifdef WINDOWS
	NOSND = 1
endif

ifndef RANLIB
	RANLIB = ranlib
endif

(%): %
%.a:
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $@

.PHONY: SUBMAKE
SUBMAKE:

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

LDLIBS += -lm

CFLAGS += $(WWW_IFLAGS) $(PYTHON_IFLAGS) $(AFEIRSA_IFLAGS) $(LIBTIFF_IFLAGS)
LIBDEPS = $(LIBWWW) $(LIBPYTHON) $(LIBAFEIRSA) $(LIBTIFF)
LDLIBS += $(LIBDEPS)

# glabi appends its own ldlibs
CFLAGS += $(GLABI)

ifndef WINDOWS
	CFLAGS += -D_POSIX_SOURCE -DAGA_HAVE_UNIX
else
	CFLAGS += -D_WINDOWS
	LDLIBS += -lgdi32
endif

ifdef GLXABI
	LDLIBS += -lX11
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

$(OUT): $(OBJECTS) $(LIBDEPS)

$(OBJECTS): $(HEADERS)

.PHONY: clean
clean:
	rm -f $(OBJECTS)
	rm -f $(OUT)
