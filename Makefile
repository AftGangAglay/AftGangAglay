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

ifndef WINDRES
	WINDRES = windres
endif

ifdef WINDOWS_SHELL
	RM = del
	CAT = type
define PATHREM
	$(RM) $(subst /,\\,$(1))
endef
else
	CAT = cat
define PATHREM
	$(RM) $(1)
endef
endif

ifdef WINDOWS
	EXE = .exe
%.exe: %.o
	$(CC) -o $@ $(LDFLAGS) $(filter %.o,$^) $(LOADLIBES) $(LDLIBS)
%: %.o
%: %.c
endif

ARFLAGS = -rc

(%): %
%.a:
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $@

include vendor/www.mk
include vendor/python.mk
include vendor/afeirsa.mk

AGA_SOURCES = $(wildcard src/*.c)
AGA_HEADERS = $(wildcard include/*.h)
AGA_OBJECTS = $(AGA_SOURCES:.c=.o)

AGA_OUT = src/main$(EXE)

CFLAGS = -std=c89
ifdef DEBUG
	CFLAGS += -g -D_DEBUG
else
	CFLAGS += -DNDEBUG -O
endif

DIAGNOSTICS = -Wall -Wextra -Werror -ansi -pedantic -pedantic-errors

AGA_CFLAGS += -Iinclude $(DIAGNOSTICS) $(GLABI_CFLAGS)
AGA_CFLAGS += -DAGA_VERSION=\"$(shell $(CAT) VERSION)\"

AGA_CFLAGS += $(WWW_IFLAGS) $(PYTHON_IFLAGS) $(AFEIRSA_IFLAGS)

AGA_LIBDEPS = $(LIBWWW) $(LIBPYTHON) $(LIBAFEIRSA)
AGA_LDLIBS += $(AGA_LIBDEPS) $(GLABI_LDLIBS) -lm

ifdef WINDOWS
	CFLAGS += -D_WINDOWS
	AGA_LDLIBS += -lgdi32 -lshell32
ifndef DEBUG
	AGA_LDFLAGS += -Wl,-subsystem,windows
endif
endif

ifdef GLXABI
	AGA_LDLIBS += -lX11
endif

ifdef NOSND
	AGA_CFLAGS += -DAGA_NOSND
endif

ifdef WINDOWS
	AGA_RCFILES = $(wildcard res/*.rc)
	AGA_RESFILES = $(AGA_RCFILES:.rc=.res)
	AGA_RESOBJECTS = $(AGA_RCFILES:.rc=.o)

	AGA_OBJECTS += $(AGA_RESOBJECTS)

%.res: %.rc
	$(WINDRES) -i $< -o $@

%.o: %.rc
	$(WINDRES) -i $< -o $@

$(AGA_OUT):	$(AGA_RESFILES)

clean: clean_res
.PHONY: clean_res
clean_res:
	$(call PATHREM,$(AGA_RESFILES))
	$(call PATHREM,$(AGA_RESOBJECTS))
endif

.DEFAULT_GOAL := all
.PHONY: all
all: $(AGA_OUT)

$(AGA_OUT): LDFLAGS += $(AGA_LDFLAGS)
$(AGA_OUT): LDLIBS += $(AGA_LDLIBS)
$(AGA_OUT): $(PYTHON_GRAMINIT_H) $(AGA_OBJECTS) $(AGA_LIBDEPS)

$(AGA_OBJECTS): CFLAGS += $(AGA_CFLAGS)
$(AGA_OBJECTS): $(AGA_HEADERS)

src/agascript.o: src/agascriptglue.h

ifdef WINDOWS
src/agawin.o: src/agawwin.h
else
src/agawin.o: src/agaxwin.h
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
