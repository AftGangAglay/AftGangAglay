# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

include VERSION
include build/env.mk
include build/winres.mk
include build/glabi.mk

include vendor/www.mk
include vendor/python.mk

AGA_SOURCES = $(wildcard src/*.c)
AGA_HEADERS = $(wildcard include/*.h)
AGA_OBJECTS = $(AGA_SOURCES:.c=.o) $(AGA_RESOBJECTS)

AGA_OUT = src/main$(EXE)

CFLAGS = -std=c89
ifdef DEBUG
	CFLAGS += -g -D_DEBUG
else
	CFLAGS += -DNDEBUG -O
endif

DIAGNOSTICS = -Wall -Wextra -Werror -ansi -pedantic -pedantic-errors

AGA_CFLAGS += -Iinclude $(DIAGNOSTICS) $(GLABI_CFLAGS)
AGA_CFLAGS += -DAGA_VERSION=\"$(VERSION)\"

AGA_CFLAGS += $(WWW_IFLAGS) $(PYTHON_IFLAGS)

AGA_LIBDEPS = $(LIBWWW) $(LIBPYTHON)
AGA_LDLIBS += $(AGA_LIBDEPS) $(GLABI_LDLIBS)

ifdef WINDOWS
	ifndef DEBUG
		AGA_LDFLAGS += -Wl,-subsystem,windows
	endif
else
	# From `feature_test_macros(7)' on `_POSIX_C_SOURCE':
	# """
	# The value 1 exposes definitions conforming to
	# POSIX.1-1990 and ISO C (1990).
	# """
	AGA_CFLAGS += -D_POSIX_C_SOURCE=1
endif

.DEFAULT_GOAL := all
.PHONY: all
all: $(AGA_OUT)

$(AGA_OUT): LDFLAGS += $(AGA_LDFLAGS)
$(AGA_OUT): LDLIBS += $(AGA_LDLIBS)
$(AGA_OUT): $(AGA_OBJECTS) $(AGA_LIBDEPS)

$(AGA_OBJECTS): CFLAGS += $(AGA_CFLAGS)
$(AGA_OBJECTS): $(AGA_HEADERS)

src/agascript.o: src/agascriptglue.c

ifdef WGL
src/agawin.o: src/agawwin.h
else
	ifdef GLX
src/agawin.o: src/agaxwin.h
	endif
endif

.PHONY: clean
clean:
	$(call PATHREM,$(AGA_OBJECTS))
	$(call PATHREM,$(AGA_OUT))

PREFIX = /usr/local

.PHONY: install
install: $(INSTALLFILES)
	install -d $(PREFIX)/bin
	install $(OUT) $(PREFIX)/bin/aftgangaglay
	install script/vertgen.py $(PREFIX)/bin/aga-vertgen
	install script/sndgen.py $(PREFIX)/bin/aga-sndgen
	install script/imggen.py $(PREFIX)/bin/aga-imggen
	install script/packgen.py $(PREFIX)/bin/aga-packgen
