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

ifdef WINDOWS_SHELL
	RM = del
define PATHREM
	$(RM) $(subst /,\\,$(1))
endef
else
define PATHREM
	$(RM) $(1)
endef
endif

ifdef WINDOWS
	EXE = .exe
%.exe: %.o
	$(CC) -o $@ $(LDFLAGS) $(filter %.o,$^) $(LOADLIBES) $(LDLIBS)
%: %.o
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

SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard include/*.h) $(wildcard src/*.h)
OBJECTS = $(SOURCES:.c=.o)

OUT = src/main$(EXE)

CFLAGS += -Iinclude
CFLAGS += -std=c89 -Wall -Wextra -Werror -ansi -pedantic -pedantic-errors

LDLIBS += -lm

CFLAGS += $(WWW_IFLAGS) $(PYTHON_IFLAGS) $(AFEIRSA_IFLAGS) $(LIBTIFF_IFLAGS)
LIBDEPS = $(LIBWWW) $(LIBPYTHON) $(LIBAFEIRSA) $(LIBTIFF)
LDLIBS += $(LIBDEPS)

include $(AFEIRSA_ROOT)/build/glabi.mk

# glabi appends its own ldlibs
CFLAGS += $(GLABI)

ifdef WINDOWS
	CFLAGS += -D_WINDOWS
	LDLIBS += -lgdi32
endif

ifdef GLXABI
	LDLIBS += -lX11
endif

ifdef DEBUG
	CFLAGS += -g -D_DEBUG
else
	CFLAGS += -DNDEBUG -O
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
	$(call PATHREM,$(OBJECTS))
	$(call PATHREM,$(OUT))

PREFIX = /usr/local

.PHONY: install
install: $(INSTALLFILES)
	install -d $(PREFIX)/bin
	install $(OUT) $(PREFIX)/bin/aftgangaglay
	install script/vertgen.py $(PREFIX)/bin/aga-vertgen
	install script/sndgen.py $(PREFIX)/bin/aga-sndgen
