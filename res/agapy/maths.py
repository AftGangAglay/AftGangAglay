# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

PI = 3.14159265358979323846
RADS = PI / 180.0

class transform():
    def create(self):
        self.pos = [ 0.0, 0.0, 0.0 ]
        self.rot = [ 0.0, 0.0, 0.0 ]
        self.scale = [ 1.0, 1.0, 1.0 ]
        return self
