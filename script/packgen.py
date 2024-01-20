#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

from sys import argv
from os import fstat
from struct import pack

if len(argv) < 3:
    print('usage: ' + argv[0] + ' <output> <input...>')
    exit(1)

file_list = []
off = 0
size = 0

for i in argv[2:]:
    f = open(i, 'rb')
    file_size = fstat(f.fileno()).st_size + int(i[-3:] == '.py')
    file_list.append([ i, f, file_size, off ])
    off += file_size

conf = '<root>\n'
for f in file_list:
    conf += '\t<item name="' + str(f[0]) + '">\n'

    conf += '\t\t<item name="Offset" type="Integer">\n'
    conf += '\t\t\t' + str(f[3]) + '\n'
    conf += '\t\t</item>\n'

    conf += '\t\t<item name="Size" type="Integer">\n'
    conf += '\t\t\t' + str(f[2]) + '\n'
    conf += '\t\t</item>\n'

    conf += '\t</item>\n'
conf += '</root>\n'

with open(argv[1], 'wb+') as f:
    f.write(pack('II', len(conf), 0xA6A))
    f.write(str.encode(conf))

    for i in file_list:
        f.write(i[1].read())
        if i[0][-3:] == '.py': f.write(b'\xFF')
        i[1].close()
