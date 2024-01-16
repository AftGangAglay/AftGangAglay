#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

from sys import argv
from struct import pack
from PIL import Image

if len(argv) != 3:
    print('usage: ' + argv[0] + ' <input> <output>')
    exit(1)

with Image.open(argv[1]) as img:
    with img.convert('RGBA') as cvt:
        data = cvt.tobytes()
        data += pack('I', cvt.width)
        with open(argv[2], 'wb+') as f:
            f.write(data)
