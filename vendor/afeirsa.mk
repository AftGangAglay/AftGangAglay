# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

AFEIRSA_ROOT = vendor/afeirsa

include $(AFEIRSA_ROOT)/build/glabi.mk

LIBAFEIRSA = $(AFEIRSA_ROOT)/src/libafeirsa.a
AFEIRSA_CFLAGS = $(GLABI)
AFEIRSA_IFLAGS = -isystem $(AFEIRSA_ROOT)/include $(PUBLIC_IFLAGS)

all: $(LIBAFEIRSA)

AFEIRSA_MAKE_FLAGS = GL10_COMPAT=1 NO_EXT=1
ifdef DEBUG
	AFEIRSA_MAKE_FLAGS += DEBUG=1
endif

$(LIBAFEIRSA): SUBMAKE
	$(MAKE) -C $(AFEIRSA_ROOT) $(AFEIRSA_MAKE_FLAGS)

clean: clean_afeirsa
.PHONY: clean_afeirsa
clean_afeirsa:
	$(MAKE) -C $(AFEIRSA_ROOT) clean
