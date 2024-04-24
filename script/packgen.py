#!/usr/bin/python
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

from sys import argv
from os import fstat
from struct import pack, unpack

IMAGE_MAGIC = 0xA6A13600
MODEL_MAGIC = 0xA6A3D700

IMAGE = 1
PYTHON = 2
MODEL = 3
UNKNOWN = 4

FILENAME = 0
FD = 1
SIZE = 2
OFFSET = 3
TYPE = 4

IMAGE_WIDTH = 5

MODEL_MIN_X = 5
MODEL_MIN_Y = 6
MODEL_MIN_Z = 7
MODEL_MAX_X = 8
MODEL_MAX_Y = 9
MODEL_MAX_Z = 10

if len(argv) < 3:
	print('usage: ' + argv[0] + ' <output> <input...>')
	exit(1)

file_list = []
off = 0
size = 0

# TODO: Descriptive list of file types and actions.

# TODO: Script minification/precompilation.

# TODO: Remove this "optimization" where we potentially hold open hundreds of
#	   FDs.
#	   Possibly replace with an archive update mode?
for file in argv[2:]:
	ent = list(range(5))

	f = open(file, 'rb')
	sz = fstat(f.fileno()).st_size

	f.seek(-4, 2)
	magic = unpack('I', f.read(4))[0]
	if file[-3:] == '.py':
		sz += 2
		ent[TYPE] = PYTHON
	elif magic == IMAGE_MAGIC:
		sz -= 8
		f.seek(-8, 2)
		ent.append(unpack('I', f.read(4))[0])  # Read width
		ent[TYPE] = IMAGE
	elif magic == MODEL_MAGIC:
		sz -= 28
		f.seek(-28, 2)
		ent.extend(unpack('ffffff', f.read(24)))  # Read extents
		ent[TYPE] = MODEL
	else:
		ent[TYPE] = UNKNOWN
	f.seek(0)

	ent[FILENAME] = file
	ent[FD] = f
	ent[SIZE] = sz
	ent[OFFSET] = off

	file_list.append(ent)
	off += sz

conf = '<root>\n'
for f in file_list:
	conf += '\t<item name="' + str(f[FILENAME]) + '">\n'

	conf += '\t\t<item name="Offset" type="Integer">\n'
	conf += '\t\t\t' + str(f[OFFSET]) + '\n'
	conf += '\t\t</item>\n'

	conf += '\t\t<item name="Size" type="Integer">\n'
	conf += '\t\t\t' + str(f[SIZE]) + '\n'
	conf += '\t\t</item>\n'

	if f[TYPE] == IMAGE:
		conf += '\t\t<item name="Width" type="Integer">\n'
		conf += '\t\t\t' + str(f[IMAGE_WIDTH]) + '\n'
		conf += '\t\t</item>\n'

	if f[TYPE] == MODEL:
		conf += '\t\t<item name="MinX" type="Float">\n'
		conf += '\t\t\t' + str(f[MODEL_MIN_X]) + '\n'
		conf += '\t\t</item>\n'
		conf += '\t\t<item name="MinY" type="Float">\n'
		conf += '\t\t\t' + str(f[MODEL_MIN_Y]) + '\n'
		conf += '\t\t</item>\n'
		conf += '\t\t<item name="MinZ" type="Float">\n'
		conf += '\t\t\t' + str(f[MODEL_MIN_Z]) + '\n'
		conf += '\t\t</item>\n'
		conf += '\t\t<item name="MaxX" type="Float">\n'
		conf += '\t\t\t' + str(f[MODEL_MAX_X]) + '\n'
		conf += '\t\t</item>\n'
		conf += '\t\t<item name="MaxY" type="Float">\n'
		conf += '\t\t\t' + str(f[MODEL_MAX_Y]) + '\n'
		conf += '\t\t</item>\n'
		conf += '\t\t<item name="MaxZ" type="Float">\n'
		conf += '\t\t\t' + str(f[MODEL_MAX_Z]) + '\n'
		conf += '\t\t</item>\n'

		# Version 2 -- we started discarding model vertex colouration.
		conf += '\t\t<item name="Version" type="Integer">\n'
		conf += '\t\t\t2\n'
		conf += '\t\t</item>\n'

	conf += '\t</item>\n'
conf += '</root>\n'

with open(argv[1], 'wb+') as f:
	f.write(pack('II', len(conf), 0xA6A))
	f.write(str.encode(conf))

	for i in file_list:
		data = i[FD].read()
		sz = min(i[SIZE], len(data))
		f.write(data[:sz])

		if i[TYPE] == PYTHON:
			f.write(b'\n\xFF')  # Writing the `\n' fixes a strange EOF bug.

		i[FD].close()
