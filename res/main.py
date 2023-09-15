# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

import agan
import agapy

import math

class camera():
    def create(self):
        self.trans = agapy.transform().create()
        return self
    #
    def update(self):
        motion = agan.getmotion()
        #
        right = 0.0
        fwd = 0.0
        #
        if(agan.getkey(agapy.KEY_w)):
            fwd = 1.0
        if(agan.getkey(agapy.KEY_s)):
            fwd = -1.0
        if(agan.getkey(agapy.KEY_a)):
            right = 1.0
        if(agan.getkey(agapy.KEY_d)):
            right = -1.0
        #
        # TODO: Get in input settings
        self.trans.rot[1] = self.trans.rot[1] + motion[0]
        self.trans.rot[0] = self.trans.rot[0] + motion[1]
        #
        yaw = self.trans.rot[1] * agapy.RADS
        cos = math.cos(yaw)
        ncos = math.cos(-yaw)
        sin = math.sin(yaw)
        #
        self.trans.pos[0] = self.trans.pos[0] + fwd * -sin + right * cos
        self.trans.pos[2] = self.trans.pos[2] + fwd * ncos + right * sin
        #
        agan.setcam(self.trans)
