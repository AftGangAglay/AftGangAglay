# SPDX-License-Identifier: X11
# Copyright (C) 2026 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

HOMEBREW_ROOT = /opt/homebrew

override CFLAGS += -I$(HOMEBREW_ROOT)$(SEP)include
override LDFLAGS += -L$(HOMEBREW_ROOT)$(SEP)lib
