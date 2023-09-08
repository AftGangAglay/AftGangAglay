# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PYTHON_ROOT = vendor/python

LIBPYTHON = vendor/python/src/libpython.a
PYTHON_IFLAGS = -isystem $(PYTHON_ROOT)/src

all: $(LIBPYTHON)

PYTHON_CFLAGS = -std=c89 -ansi -w
PYTHON_CFLAGS += -Wno-incompatible-function-pointer-types

$(LIBPYTHON):
	$(MAKE) -C $(PYTHON_ROOT)/src CFLAGS="$(PYTHON_CFLAGS)"

clean: clean_python
.PHONY: clean_python
clean_python:
	$(MAKE) -C $(PYTHON_ROOT)/src clean
	rm -rf $(LIBPYTHON)
