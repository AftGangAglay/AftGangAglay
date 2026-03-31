# SPDX-License-Identifier: X11
# Copyright (C) 2025, 2026 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

# TODO: Create a proper catalogue of build requirements.
# TODO: BSD/Make, IRIX-y make etc. support?

include VERSION

ifdef OS
	ifndef CROSS
		WINDOWS = 1
	endif

	RM = del
	SEP = \\
else
	RM = rm -f
	SEP = /
endif

override LDLIBS += -lm

ifdef DEBUG
	override CFLAGS += -g
else
	override CFLAGS += -O -DNDEBUG
endif

ifdef DEVBUILD
	override CFLAGS += -DAGA_DEVBUILD
endif

ifdef NOVERIFY
	override CFLAGS += -DAGA_NOVERIFY
endif

ifdef MAINTAINER
	override CFLAGS += -ansi -pedantic -pedantic-errors -Wall -W -Werror
endif

ifdef WINDOWS
	LIB =
	OBJ = .obj
	EXE = .exe
	A = .lib

	override LDLIBS += -lopengl32 -lglu32 -lshell32 -lwinmm -lcomdlg32
	override LDLIBS += -luser32 -lkernel32 -lgdi32

	# TODO: Does this work under clang invoking LINK?
	# NOTE: `-Wl' is not historically accurate.
	override LDFLAGS += -Wl,-subsystem,windows
else
	LIB = lib
	OBJ = .o
	EXE =
	A = .a

	override LDLIBS += -lGL -lGLU -lX11 -lXm -lXt
endif

AR = ar
STATIC = $(AR) -rc $@ $(filter %$(OBJ),$^)

LINK = $(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

include lib/asys/asys.mk
include lib/prof/apro.mk
MIL_BASE = lib$(SEP)mil$(SEP)mil$(SEP)
include lib/mil/mil/mil.mk

include vendor/python.mk
include vendor/www.mk
include vendor/glm.mk
include vendor/tiff.mk

include src/aga.mk

override CFLAGS += -I$(APRO_INCLUDE) -I$(ASYS_INCLUDE) -I$(PY_INCLUDE)
override CFLAGS += -I$(WWW_INCLUDE) -I$(GLM_INCLUDE) -I$(TIFF_INCLUDE)
override CFLAGS += -I$(MIL_INCLUDE)

override CFLAGS += -Iinclude -Ivendor$(SEP)libtiff$(SEP) -Ivendor$(SEP)
override CFLAGS += -DAGA_VERSION=\"$(VERSION)\"

.SUFFIXES: $(OBJ)
.c$(OBJ):
	$(CC) -c $(CFLAGS) -o $@ $<

.DEFAULT_GOAL := all
.PHONY: all
all: $(AGA_OUT)

.PHONY: clean
.PHONY: clean_asys clean_apro clean_mil
.PHONY: clean_python clean_www clean_glm clean_tiff
.PHONY: clean_aga

clean: clean_asys clean_apro clean_mil
clean: clean_python clean_www clean_glm clean_tiff
clean: clean_aga
