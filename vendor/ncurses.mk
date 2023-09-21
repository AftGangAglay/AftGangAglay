# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

NCURSES_ROOT = vendor/ncurses/src

LIBNCURSES = $(NCURSES_ROOT)/libncurses.a
NCURSES_IFLAGS = -isystem $(NCURSES_ROOT)

NCURSES_CFLAGS = -std=c89 -ansi -w -DBRAINDEAD -D_XOPEN_SOURCE
# NOTE: This shouldn't be necessary according to this:
# 		https://stackoverflow.com/a/25571280/13771204
# 		However `-std=gnu89' still complains.
NCURSES_CFLAGS += -Dinline=__inline__
NCURSES_CFLAGS += -Wno-incompatible-function-pointer-types

ifdef DEBUG
	NCURSES_CFLAGS += -g -O0 -D_DEBUG
else
	NCURSES_CFLAGS += -DNDEBUG -Ofast
endif

NCURSES_TERMINFO = vendor/ncurses/terminfo

NCURSES_TERMCAPS_ROOT = vendor/ncurses/data/
NCURSES_C_TERMCAPS = console
NCURSES_H_TERMCAPS = hp70092
NCURSES_P_TERMCAPS = pcvt25h pcvt25hc
NCURSES_V_TERMCAPS = vt100 vt200 vt300
NCURSES_X_TERMCAPS = xterm xterm-bold xterm-ic xterms

all: $(LIBNCURSES) $(NCURSES_TERMINFO)

$(NCURSES_TERMINFO):
	mkdir -p $@

$(NCURSES_ROOT)/Makefile:
	cat $(NCURSES_ROOT)/Config.linux $(NCURSES_ROOT)/Makefile.dist > $@

NCURSES_MAKEFLAGS = SRCDIR=$(NCURSES_TERMINFO) CC="$(CC) $(NCURSES_CFLAGS)"
$(LIBNCURSES): $(NCURSES_ROOT)/Makefile SUBMAKE
	$(MAKE) -C $(NCURSES_ROOT) $(NCURSES_MAKEFLAGS)

clean: clean_ncurses
.PHONY: clean_ncurses
clean_ncurses:
	-$(MAKE) -C $(NCURSES_ROOT) clean
	rm -rf $(NCURSES_ROOT)/Makefile
	rm -rf $(NCURSES_ROOT)/tic
	rm -rf $(NCURSES_ROOT)/untic
	rm -rf $(NCURSES_ROOT)/terminfo.h
	rm -rf $(NCURSES_TERMINFO)
