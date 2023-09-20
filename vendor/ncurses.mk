# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

NCURSES_ROOT = vendor/ncurses/src

LIBNCURSES = $(NCURSES_ROOT)/libncurses.a
NCURSES_IFLAGS = -isystem $(NCURSES_ROOT)/src

NCURSES_CFLAGS = -std=c89 -ansi -w -DBRAINDEAD
NCURSES_CFLAGS += -Wno-incompatible-function-pointer-types

ifdef DEBUG
	NCURSES_CFLAGS += -g -O0 -D_DEBUG
else
	NCURSES_CFLAGS += -DNDEBUG -Ofast
endif

all: $(LIBNCURSES)

$(NCURSES_ROOT)/Makefile:
	cat $(NCURSES_ROOT)/Config.linux $(NCURSES_ROOT)/Makefile.dist > $@

$(LIBNCURSES): $(NCURSES_ROOT)/Makefile SUBMAKE
	$(MAKE) -C $(NCURSES_ROOT) CC="$(CC) $(CFLAGS)"

clean: clean_ncurses
.PHONY: clean_ncurses
clean_ncurses:
	-$(MAKE) -C $(NCURSES_ROOT) clean
	rm -rf $(NCURSES_ROOT)/Makefile
	rm -rf $(NCURSES_ROOT)/tic
	rm -rf $(NCURSES_ROOT)/untic
	rm -rf $(NCURSES_ROOT)/terminfo.h
