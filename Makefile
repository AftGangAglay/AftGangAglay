# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

!include VERSION

default: all

RM = del
SEP = \\

LIB =
OBJ = .obj
EXE = .exe
A = .lib

CC = $(CC) /nologo

AR = lib /nologo /out:$@ $**
CCLD = $(CC) /Fe:$@ $**
WL = /link

SET_CFLAGS = /Zc:wchar_t

!ifdef DEBUG
SET_CFLAGS = $(SET_CFLAGS) /Od /Zi /D_DEBUG /MTd
!else
SET_CFLAGS = $(SET_CFLAGS) /O2 /DNDEBUG /MT
!endif

GL_LDLIBS = opengl32.lib glu32.lib gdi32.lib shell32.lib user32.lib winmm.lib

!include lib/prof/apro.mk
!include vendor/python.mk
!include vendor/www.mk
!include src/aga.mk

SET_LDFLAGS =
SET_LDLIBS =

!ifdef MAINTAINER
SET_CFLAGS = $(SET_CFLAGS) /Wall /WX

# Padding
SET_CFLAGS = $(SET_CFLAGS) /wd4820
# Function was not inlined
SET_CFLAGS = $(SET_CFLAGS) /wd4710
# Spectre mitigations
SET_CFLAGS = $(SET_CFLAGS) /wd5045
# Function selected for automatic inlining
SET_CFLAGS = $(SET_CFLAGS) /wd4711
# Assignment inside conditional expression (even with double paren)
SET_CFLAGS = $(SET_CFLAGS) /wd4706
# Non-explicitly handled enum value (Doesn't count `default:')
SET_CFLAGS = $(SET_CFLAGS) /wd4061
# Conditional expression is constant (Even if block contains a `break')
SET_CFLAGS = $(SET_CFLAGS) /wd4127
!endif

SET_CFLAGS = $(SET_CFLAGS) /I$(APRO) /I$(PYI) /I$(WWW) /Iinclude
SET_CFLAGS = $(SET_CFLAGS) /DAGA_VERSION=\"$(VERSION)\"

.c$(OBJ):
	$(CC) /c $(CFLAGS) $(SET_CFLAGS) /Fo:$@ $<

all: $(AGA_OUT)

clean: clean_apro clean_python clean_www clean_aga
