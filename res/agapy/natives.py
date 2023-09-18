# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

import agan
import maths

TRIANGLES = 0
TRIANGLE_STRIP = 1
TRIANGLE_FAN = 2
LINES = 3
LINE_STRIP = 4
LINE_LOOP = 5
QUADS = 6
QUAD_STRIP = 7
POINTS = 8
POLYGON = 9

class largefile():
    def create(self, str):
        self.nativeptr = agan.mklargefile(str)
        return self
    #
    def close(self):
        agan.killlargefile(self.nativeptr)

class vertexbuffer():
    def create(self, file):
        self.nativeptr = agan.mkvertbuf(file.nativeptr)
        return self
    #
    def close(self):
        agan.killvertbuf(self.nativeptr)
    #
    def draw(self, (primitive, trans)):
        agan.drawbuf(self.nativeptr, primitive, trans)
