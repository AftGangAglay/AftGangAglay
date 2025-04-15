# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

# TODO: Revisit `INSTALL' and create a proper catalogue of build requirements.
# TODO: BSD/Make, IRIX-y make etc. support?

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

AR = ar
STATIC = $(AR) -rc $@ $^

override LDLIBS += -lm

LINK = $(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

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

XQUARTZ_ROOT = /opt/X11

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

	override LDLIBS += -lGL -lGLU -lX11
	ifdef APPLE
		override CFLAGS += -I$(XQUARTZ_ROOT)/include
		override LDFLAGS += -L$(XQUARTZ_ROOT)/lib
	endif
endif

include lib/sys/asys.mk
include lib/prof/apro.mk

include vendor/python.mk
include vendor/www.mk
include vendor/glm.mk
include vendor/tiff.mk

include src/aga.mk

override CFLAGS += -I$(APRO_INCLUDE) -I$(ASYS_INCLUDE) -I$(PY_INCLUDE)
override CFLAGS += -I$(WWW_INCLUDE) -I$(GLM_INCLUDE) -I$(TIFF_INCLUDE)

override CFLAGS += -Iinclude -Ivendor$(SEP)libtiff$(SEP)
override CFLAGS += -DAGA_VERSION=\"$(VERSION)\"

.SUFFIXES: $(OBJ)
.c$(OBJ):
	$(CC) -c $(CFLAGS) -o $@ $<

.DEFAULT_GOAL := all
.PHONY: all
all: $(AGA_OUT)

.PHONY: clean
.PHONY: clean_asys clean_apro
.PHONY: clean_python clean_www clean_glm clean_tiff
.PHONY: clean_aga

clean: clean_asys clean_apro
clean: clean_python clean_www clean_glm clean_tiff
clean: clean_aga
