#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

from sys import argv
from os import fstat
from struct import pack, unpack

IMAGE_MAGIC = 0xA6A13600

IMAGE = 1
PYTHON = 2
UNKNOWN = 3

FILENAME = 0
FD = 1
SIZE = 2
OFFSET = 3
TYPE = 4

IMAGE_WIDTH = 5

if len(argv) < 3:
    print('usage: ' + argv[0] + ' <output> <input...>')
    exit(1)

file_list = []
off = 0
size = 0

# TODO: Remove this "optimization" where we potentially hold open hundreds of
#       FDs.
#       Possibly replace with an archive update mode?
for file in argv[2:]:
    ent = list(range(5))

    f = open(file, 'rb')
    sz = fstat(f.fileno()).st_size

    if file[-3:] == '.py':
        sz += 2
        ent[TYPE] = PYTHON
    else:
        f.seek(-4, 2)
        if unpack("I", f.read(4))[0] == IMAGE_MAGIC:
            sz -= 4
            f.seek(-8, 2)
            ent.append(unpack("I", f.read(4))[0]) # Read width
            ent[TYPE] = IMAGE
        else: ent[TYPE] = UNKNOWN
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
            f.write(b'\n\xFF') # Writing the `\n' fixes a strange EOF bug.

        i[FD].close()
