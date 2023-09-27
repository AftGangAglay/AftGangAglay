# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PYTHON3 = python3

MODELS = $(wildcard res/model/*.obj)
MODELBINS = $(MODELS:.obj=.obj.raw)

SOUNDS = $(wildcard res/snd/*.mp3)
SOUNDBINS = $(SOUNDS:.mp3=.mp3.raw)

%.obj.raw: %.obj script/vertgen.py
	$(PYTHON3) script/vertgen.py $< $@

%.mp3.raw: %.mp3 script/gensnd.sh
ifndef NOSND
	script/gensnd.sh $< $@
else
	touch $@
endif

all: $(MODELBINS) $(SOUNDBINS)

clean: clean_res
.PHONY: clean_res
clean_res:
	rm -f $(MODELBINS)
	rm -f $(SOUNDBINS)
