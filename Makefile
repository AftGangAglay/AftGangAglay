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

AR = lib /nologo /out:$@
O = /Fe:$@
ALL = $**
WL = /link

!ifdef DEBUG
CFLAGS = /Od /Zi /D_DEBUG /MTd
!else
CFLAGS = /O2 /DNDEBUG /MT
!endif

GL_LDLIBS = opengl32.lib glu32.lib gdi32.lib shell32.lib user32.lib

CC = $(CC) /nologo

!include lib/prof/apro.mk
!include vendor/python.mk
!include vendor/www.mk
!include src/aga.mk

SET_CFLAGS = /I$(APRO) /I$(PYI) /I$(WWW) /Iinclude /DAGA_VERSION=\"$(VERSION)\"

.c$(OBJ):
	$(CC) /c $(CFLAGS) $(SET_CFLAGS) $(GLABI_CFLAGS) /Fo:$@ $<

all: $(AGA_OUT)

clean: clean_apro clean_python clean_www clean_aga
