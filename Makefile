# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

(%): %
%.a:
	$(AR) $(ARFLAGS) $@ $?
	ranlib $@

include vendor/www.mk
include vendor/python.mk

OUT = src/libafeirsa.a

SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard include/*.h)
OBJECTS = $(SOURCES:.c=.o)

OUT = src/main

CFLAGS += -Iinclude
CFLAGS += -std=c89 -Wall -Wextra -Werror -ansi -pedantic -pedantic-errors

CFLAGS += $(shell pkg-config --cflags afeirsa)
LDLIBS += $(shell pkg-config --libs afeirsa) -ltiff -lm

CFLAGS += $(WWW_IFLAGS) $(PYTHON_IFLAGS)
LDLIBS += $(LIBWWW) $(LIBPYTHON)

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

$(OUT): $(OBJECTS) $(LIBWWW) $(LIBPYTHON)

$(OBJECTS): $(HEADERS)

.PHONY: clean
clean:
	rm -f $(OBJECTS)
	rm -f $(OUT)
