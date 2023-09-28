# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

LIBTIFF_ROOT = vendor/libtiff

LIBTIFF = $(LIBTIFF_ROOT)/libtiff/libtiff.a
LIBTIFF_IFLAGS = -isystem $(LIBTIFF_ROOT)/libtiff

LIBTIFF_CFLAGS = -std=c89 -ansi -D_SVID_SOURCE -w
LIBTIFF_CFLAGS += -Wno-incompatible-function-pointer-types

ifndef WINDOWS
	LIBTIFF_CFLAGS += -Dunix
endif

ifdef DEBUG
	LIBTIFF_CFLAGS += -g -O0 -D_DEBUG
else
	LIBTIFF_CFLAGS += -DNDEBUG -Ofast
endif

ifdef APPLE
	LIBTIFF_CONFIG_FLAGS += i386-unknown-bsd
endif

all: $(LIBTIFF)

$(LIBTIFF_ROOT)/Makefile:
	cd $(LIBTIFF_ROOT) && (./configure $(LIBTIFF_CONFIG_FLAGS) << yes)

LIBTIFF_MAKE_FLAGS = COPTS="$(LIBTIFF_CFLAGS)" CC="$(CC)"
LIBTIFF_MAKE_FLAGS += CROSS_TOOL="$(CROSS_TOOL)"
ifdef WINDOWS
	LIBTIFF_MAKE_FLAGS += WINDOWS=1
endif
$(LIBTIFF): $(LIBTIFF_ROOT)/Makefile SUBMAKE
	$(MAKE) -C $(LIBTIFF_ROOT)/libtiff $(LIBTIFF_MAKE_FLAGS)

clean: clean_libtiff
.PHONY: clean_libtiff
clean_libtiff:
	-$(MAKE) -C $(LIBTIFF_ROOT) clean
	rm -f $(LIBTIFF_ROOT)/config.log
	rm -f $(LIBTIFF_ROOT)/Makefile
	rm -f $(LIBTIFF_ROOT)/port.h
	rm -f $(LIBTIFF_ROOT)/libtiff/Makefile
	rm -f $(LIBTIFF_ROOT)/man/Makefile
	rm -f $(LIBTIFF_ROOT)/tools/Makefile
	rm -f $(LIBTIFF_ROOT)/port/install.sh
