#!/usr/bin/python
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

from sys import argv
from struct import pack
from mido import MidiFile

MAGIC = 0xA6A31D10

if len(argv) != 3:
	print('usage: ' + argv[0] + ' <input> <output>')
	exit(1)

with open(argv[2], 'wb+') as f:
	mid = MidiFile(argv[1])
	for msg in mid:
		if(msg.is_meta): continue
		f.write(pack('II', msg.time, 0))
		f.write(msg.bin())
		f.write(pack('I', 0))
