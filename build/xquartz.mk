# SPDX-License-Identifier: X11
# Copyright (C) 2026 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

XQUARTZ_ROOT = /opt/X11

override CFLAGS += -I$(XQUARTZ_ROOT)$(SEP)include
override LDFLAGS += -L$(XQUARTZ_ROOT)$(SEP)lib
