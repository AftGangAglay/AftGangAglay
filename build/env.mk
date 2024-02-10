# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

RANLIB = ranlib
WINDRES = windres
ARFLAGS = -rc

(%): %
%.a:
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $@

ifdef WINDOWS_SHELL
	RM = del
	CAT = type
define PATHREM
	$(RM) $(subst /,\\,$(1))
endef
else
	CAT = cat
define PATHREM
	$(RM) $(1)
endef
endif

ifdef WINDOWS
	EXE = .exe
%.exe: %.o
	$(CC) -o $@ $(LDFLAGS) $(filter %.o,$^) $(LOADLIBES) $(LDLIBS)
%: %.o
%: %.c
endif

