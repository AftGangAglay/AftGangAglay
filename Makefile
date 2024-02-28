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

AR = lib /out:
O = /Fe: $@
ALL = $**
WL = /link
NICEOUT = /nologo

!ifdef DEBUG
CFLAGS = /Od /Zi /D_DEBUG /MTd
!else
CFLAGS = /O2 /DNDEBUG /MT
!endif

GL_LDLIBS = opengl32.lib glu32.lib gdi32.lib shell32.lib

CC = $(CC) /nologo /showIncludes

!include vendor/python.mk
!include vendor/www.mk
!include src/aga.mk

SET_CFLAGS = /I include /I $(PY) /I $(WWW) /DAGA_VERSION="$(VERSION)"

.res$(OBJ):
	$(RC) /fo $@.res $<

.c$(OBJ):
	$(CC) /c $(SET_CFLAGS) $(GLABI_CFLAGS) /Fo: $@ $<

all: $(AGA_OUT)

clean: clean_python clean_www clean_aga
