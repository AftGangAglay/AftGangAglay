# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

LIBTIFF_ROOT = vendor/libtiff

LIBTIFF = $(LIBTIFF_ROOT)/libtiff/libtiff.a
LIBTIFF_IFLAGS = -isystem $(LIBTIFF_ROOT)/libtiff

LIBTIFF_CFLAGS = -std=c89 -ansi -D_SVID_SOURCE -w
LIBTIFF_CFLAGS += -Wno-incompatible-function-pointer-types

ifdef DEBUG
	LIBTIFF_CFLAGS += -g -O0 -D_DEBUG
else
	LIBTIFF_CFLAGS += -DNDEBUG -Ofast
endif

all: $(LIBTIFF)

$(LIBTIFF_ROOT)/Makefile:
	cd $(LIBTIFF_ROOT) && (./configure << yes)

LIBTIFF_MAKE_FLAGS = COPTS="$(LIBTIFF_CFLAGS)" CC="$(CC)"
$(LIBTIFF): $(LIBTIFF_ROOT)/Makefile
	$(MAKE) -C $(LIBTIFF_ROOT) $(LIBTIFF_MAKE_FLAGS)

clean: clean_libtiff
.PHONY: clean_libtiff
clean_libtiff:
	-$(MAKE) -C $(LIBTIFF_ROOT) clean
	rm -rf $(LIBTIFF_ROOT)/config.log
	rm -rf $(LIBTIFF_ROOT)/Makefile
	rm -rf $(LIBTIFF_ROOT)/port.h
	rm -rf $(LIBTIFF_ROOT)/libtiff/Makefile
	rm -rf $(LIBTIFF_ROOT)/man/Makefile
	rm -rf $(LIBTIFF_ROOT)/tools/Makefile
	rm -rf $(LIBTIFF_ROOT)/port/install.sh
