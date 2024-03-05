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

!ifdef DEBUG
SET_CFLAGS = /Od /Zi /D_DEBUG /MTd
!else
SET_CFLAGS = /O2 /DNDEBUG /MT
!endif

GL_LDLIBS = opengl32.lib glu32.lib gdi32.lib shell32.lib user32.lib

!include lib/prof/apro.mk
!include vendor/python.mk
!include vendor/www.mk
!include src/aga.mk

SET_LDFLAGS =
SET_LDLIBS =

!ifdef MAINTAINER
SET_CFLAGS = $(SET_CFLAGS) /Wall /WX
SET_CFLAGS = $(SET_CFLAGS) /wd4820 /wd5045
# TODO: We can't "maybe unused" with MSVC so maybe we give in to the `(void)'.
SET_CFLAGS = $(SET_CFLAGS) /wd4100
!endif

SET_CFLAGS = $(SET_CFLAGS) /I$(APRO) /I$(PYI) /I$(WWW) /Iinclude
SET_CFLAGS = $(SET_CFLAGS) /DAGA_VERSION=\"$(VERSION)\"

.c$(OBJ):
	$(CC) /c $(CFLAGS) $(SET_CFLAGS) /Fo:$@ $<

all: $(AGA_OUT)

clean: clean_apro clean_python clean_www clean_aga
