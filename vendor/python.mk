# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PYTHON_ROOT = vendor/python

LIBPYTHON = $(PYTHON_ROOT)/src/libpython.a
PYTHON_IFLAGS = -isystem $(PYTHON_ROOT)/src

all: $(LIBPYTHON)

PYTHON_CFLAGS = -std=c89 -ansi -w -DSYSV
PYTHON_CFLAGS += -Wno-incompatible-function-pointer-types
ifdef WINDOWS
	PYTHON_CFLAGS += -D_WINDOWS -DNO_LSTAT
endif

ifdef DEBUG
	PYTHON_CFLAGS += -g -O0 -D_DEBUG -DDEBUG
else
	PYTHON_CFLAGS += -DNDEBUG -Ofast
endif

$(LIBPYTHON): SUBMAKE
	$(MAKE) -C $(PYTHON_ROOT)/src CFLAGS="$(PYTHON_CFLAGS)"

clean: clean_python
.PHONY: clean_python
clean_python:
	$(MAKE) -C $(PYTHON_ROOT)/src clean
	rm -f $(LIBPYTHON)
