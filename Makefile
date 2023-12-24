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
HEADERS = $(wildcard include/*.h)
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
	LDLIBS += -lgdi32 -lshell32
	LDFLAGS += -Wl,-subsystem,windows
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

ifdef WINDOWS
RCFILES = $(wildcard res/*.rc)
RESFILES = $(RCFILES:.rc=.res)
RESOBJECTS = $(RCFILES:.rc=.o)

%.res: %.rc
	$(WINDRES) -i $< -o $@

%.o: %.rc
	$(WINDRES) -i $< -o $@

OBJECTS += $(RESOBJECTS)

$(OUT):	$(RESFILES)

.PHONY: clean_res
clean: clean_res

clean_res:
	$(call PATHREM,$(RESFILES))
	$(call PATHREM,$(RESOBJECTS))

endif

.DEFAULT_GOAL := all
.PHONY: all
all: $(OUT)

$(OUT): $(OBJECTS) $(LIBDEPS)

$(OBJECTS): $(HEADERS)

src/agascript.o: src/agascriptglue.h

ifdef WINDOWS
src/agawin.o: src/agawwin.h
else
src/agawin.o: src/agaxwin.h
endif

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
