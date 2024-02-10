# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

AGA_RCFILES = $(wildcard res/*.rc)
AGA_RESFILES = $(AGA_RCFILES:.rc=.res)
AGA_RESOBJECTS = $(AGA_RCFILES:.rc=.o)

%.res: %.rc $(EMBED)
ifdef EMBED
	$(WINDRES) -DAGA_PACK_PATH=$(EMBED) -i $< -o $@
else
	$(WINDRES) -i $< -o $@
endif

%.o: %.rc
	$(WINDRES) -i $< -o $@

clean: clean_res
.PHONY: clean_res
clean_res:
	$(call PATHREM,$(AGA_RESFILES))
	$(call PATHREM,$(AGA_RESOBJECTS))

ifdef WINDOWS
$(AGA_OUT):	$(AGA_RESFILES)
endif
