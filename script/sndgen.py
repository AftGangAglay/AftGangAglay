#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

from sys import argv
from os import system

# TODO: Add some magic to this to verify the data is sensible.

if len(argv) != 3:
	print('usage: ' + argv[0] + ' <input> <output>')
	exit(1)

system('ffmpeg -i ' + argv[1] + ' -f u8 -ar 8000 -ab 8k -ac 1 ' + argv[2])
