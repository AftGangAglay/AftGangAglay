# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

!include VERSION

default: all

### Shell setup.

RM = del
# TODO: Has VisualC always allowed `#include <foo/bar.h>' over
# 		`#include <foo\bar.h>'? Do we need a macro like:
# 		`#include INCLUDE_PATH(foo, bar)'?
SEP = \\

### Preferred artefact names.

LIB =
OBJ = .obj
EXE = .exe
A = .lib

### Linkage.

STATIC = LIB /NOLOGO /OUT:$@ $**

LDLIBS = $(LDLIBS) user32.lib kernel32.lib gdi32.lib shell32.lib winmm.lib
LDLIBS = $(LDLIBS) opengl32.lib glu32.lib comdlg32.lib

LINK = LINK /NOLOGO /OUT:$@ $** $(LDFLAGS) $(LDLIBS)

### Modules.

!include lib/sys/asys.mk
!include lib/prof/apro.mk

!include vendor/python.mk
!include vendor/www.mk
!include vendor/glm.mk
!include vendor/tiff.mk

# TODO: Remove once pgen has been switched to `asys_main'.
LDFLAGS = $(LDFLAGS) /SUBSYSTEM:WINDOWS

!include src/aga.mk

### Compilation.

!ifdef DEBUG
CFLAGS = $(CFLAGS) /Od /Zi
!else
CFLAGS = $(CFLAGS) /Ox /DNDEBUG
# NOTE: Fixes register spilling diagnostics.
CFLAGS = $(CFLAGS) /fp:fast
!endif

!ifdef DEVBUILD
CFLAGS = $(CFLAGS) /DAGA_DEVBUILD
!endif

!ifdef NOVERIFY
CFLAGS = $(CFLAGS) /DAGA_NOVERIFY
!endif

!ifdef MAINTAINER
CFLAGS = $(CFLAGS) /Wall /WX

# Padding
CFLAGS = $(CFLAGS) /wd4820
# Function was not inlined
CFLAGS = $(CFLAGS) /wd4710
# Spectre mitigations
CFLAGS = $(CFLAGS) /wd5045
# Function selected for automatic inlining
CFLAGS = $(CFLAGS) /wd4711
# Assignment inside conditional expression (even with double paren)
CFLAGS = $(CFLAGS) /wd4706
# Non-explicitly handled enum value (Doesn't count `default:')
CFLAGS = $(CFLAGS) /wd4061
# Conditional expression is constant (Even if block contains a `break')
CFLAGS = $(CFLAGS) /wd4127
!endif

CFLAGS = $(CFLAGS) /I$(APRO_INCLUDE) /I$(ASYS_INCLUDE) /I$(PY_INCLUDE)
CFLAGS = $(CFLAGS) /I$(WWW_INCLUDE) /I$(GLM_INCLUDE) /I$(TIFF_INCLUDE)

CFLAGS = $(CFLAGS) /Iinclude /Ivendor$(SEP)libtiff$(SEP)
CFLAGS = $(CFLAGS) /DAGA_VERSION=\"$(VERSION)\"

.c$(OBJ):
	CL /nologo /c $(CFLAGS) /Fo:$@ $<

all: $(AGA_OUT)

clean: clean_asys clean_apro
clean: clean_python clean_www clean_glm clean_tiff
clean: clean_aga
