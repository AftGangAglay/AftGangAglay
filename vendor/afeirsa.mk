# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

AFEIRSA_ROOT = vendor/afeirsa

include $(AFEIRSA_ROOT)/build/glabi.mk

AFEIRSA_SOURCES = $(wildcard $(AFEIRSA_ROOT)/src/*.c)
AFEIRSA_OBJECTS = $(AFEIRSA_SOURCES:.c=.o)
LIBAFEIRSA = $(AFEIRSA_ROOT)/src/libafeirsa.a

AFEIRSA_CFLAGS = $(GLABI_CFLAGS) -I$(AFEIRSA_ROOT)/include -DAF_BUILD
AFEIRSA_CFLAGS += -DGL10_COMPAT=1 -DNO_EXT=1
AFEIRSA_IFLAGS = -isystem $(AFEIRSA_ROOT)/include $(PUBLIC_IFLAGS)

all: $(LIBAFEIRSA)

$(LIBAFEIRSA): $(AFEIRSA_OBJECTS)

$(AFEIRSA_OBJECTS): CFLAGS += $(AFEIRSA_CFLAGS)

clean: clean_afeirsa
.PHONY: clean_afeirsa
clean_afeirsa:
	$(call PATHREM,$(AFEIRSA_OBJECTS))
	$(call PATHREM,$(LIBAFEIRSA))
