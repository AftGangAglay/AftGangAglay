#!/usr/bin/python
# SPDX-License-Identifier: X11
# Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

from sys import argv
from os import system

# TODO: Add some magic to this to verify the data is sensible.

print('warn: `aga-sndgen\' is deprecated in favour of `agabuild.sgml\'')

if len(argv) != 3:
	print('usage: ' + argv[0] + ' <input> <output>')
	exit(1)

system('ffmpeg -i ' + argv[1] + ' -f u8 -ar 8000 -ab 8k -ac 1 ' + argv[2])
