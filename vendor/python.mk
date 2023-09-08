# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PYTHON_ROOT = vendor/python

LIBPYTHON = vendor/python/src/libpython.a
PYTHON_IFLAGS = -isystem $(PYTHON_ROOT)/src

all: $(LIBPYTHON)

$(LIBPYTHON):
	$(MAKE) -C $(PYTHON_ROOT)/src

clean: clean_python
.PHONY: clean_python
clean_python:
	$(MAKE) -C $(PYTHON_ROOT)/src clean
