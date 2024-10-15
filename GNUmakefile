# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

# TODO: Revisit `INSTALL' and create a proper catalogue of build requirements.

include VERSION

ifdef OS
	ifndef CROSS
		WINDOWS = 1
	endif

	RM = del
	SEP = \\
else
	ifndef CROSS
		ifeq ($(shell uname -s),Darwin)
			APPLE = 1
		endif
	endif

	RM = rm -f
	SEP = /
endif

AR = ar -rc $@ $^
CCLD = $(CC) -o $@ $^
WL =

ifdef DEBUG
	SET_CFLAGS = -g
else
	SET_CFLAGS = -O -DNDEBUG
endif

ifdef DEVBUILD
	SET_CFLAGS += -DAGA_DEVBUILD
endif

ifdef MAINTAINER
	SET_CFLAGS += -ansi -pedantic -pedantic-errors -Wall -Wextra -Werror
endif

SET_LDFLAGS =
SET_LDLIBS = -lm

XQUARTZ_ROOT = /opt/X11

ifdef WINDOWS
	LIB =
	OBJ = .obj
	EXE = .exe
	A = .lib

	GL_LDLIBS = -lopengl32 -lglu32 -lgdi32 -lshell32 -luser32 -lwinmm
	GL_LDLIBS += -lcomdlg32
else
	LIB = lib
	OBJ = .o
	EXE =
	A = .a

	GL_LDLIBS = -lGL -lGLU -lX11
	ifdef APPLE
		GL_CFLAGS = -I$(XQUARTZ_ROOT)/include
		GL_LDFLAGS = -L$(XQUARTZ_ROOT)/lib
	endif
endif

include lib/prof/apro.mk
include vendor/python.mk
include vendor/www.mk
include src/aga.mk

SET_CFLAGS += $(GL_CFLAGS)
SET_CFLAGS += -I$(APRO) -I$(PYI) -I$(WWW) -Iinclude
SET_CFLAGS += -DAGA_VERSION=\"$(VERSION)\"

.SUFFIXES: $(OBJ)
.c$(OBJ):
	$(CC) -c $(CFLAGS) $(SET_CFLAGS) -o $@ $<

.DEFAULT_GOAL := all
.PHONY: all
all: $(AGA_OUT)

.PHONY: clean
.PHONY: clean_apro clean_python clean_www clean_aga
clean: clean_apro clean_python clean_www clean_aga
