# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

# TODO: Whitespace/Tabulation fixing.

include VERSION

ifdef OS
	ifndef CROSS
		WINDOWS = 1
	endif

	RM = del
	SEP = \\

	LIB =
	OBJ = .obj
	EXE = .exe
	A = .lib
else
	ifndef CROSS
		ifeq ($(shell uname -s),Darwin)
			APPLE = 1
		endif
	endif

	RM = rm
	SEP = /

	LIB = lib
	OBJ = .o
	EXE =
	A = .a
endif

WINDRES = windres
AR = ar -rc
O = -o $@
ALL = $^
WL =
NICEOUT =

ifdef DEBUG
	CFLAGS = -g -D_DEBUG
else
	CFLAGS = -O -DNDEBUG
endif

LDLIBS = -lm

XQUARTZ_ROOT = /opt/X11

ifdef WINDOWS
	GL_LDLIBS = -lopengl32 -lglu32 -lgdi32 -lshell32
else
	GL_LDLIBS = -lGL -lGLU -lX11
	ifdef APPLE
		GL_CFLAGS = -I$(XQUARTZ_ROOT)/include
		GL_LDFLAGS = -L$(XQUARTZ_ROOT)/lib
	endif
endif

include vendor/python.mk
include vendor/www.mk
include src/aga.mk

SET_CFLAGS = -Iinclude -I$(PY) -I$(WWW) -DAGA_VERSION=\"$(VERSION)\"

ifdef WINDOWS
.SUFFIXES: .obj
.rc$(OBJ):
	$(WINDRES) -i $< -o $@.res
	$(WINDRES) -i $@.res $-o $@
	$(RM) $@.res
else
.rc$(OBJ):
	$(CC) -c -o $@ res$(SEP)none.c
endif

.c$(OBJ):
	$(CC) -c $(CFLAGS) $(SET_CFLAGS) $(GLABI_CFLAGS) $(O) $<

.DEFAULT_GOAL := all
.PHONY: all
all: $(AGA_OUT)

.PHONY: clean clean_python clean_www clean_aga
clean: clean_python clean_www clean_aga
