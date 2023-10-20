# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

import aga
import math

OUTER = 0
INNER = 1

class camera():
    def create(self, (conf, bounds)):
        self.trans = aga.transform().create()
        self.trans.pos[1] = -2.0
        #
        self.move_speed = conf['move_speed']
        self.sensitivity = conf['sensitivity']
        #
        self.cmodekey = 0
        #
        self.bounds = bounds
        self.noclip = 0
        #
        return self
    #
    def update(self):
        motion = aga.getmotion()
        #
        right = 0.0
        fwd = 0.0
        #
        if(aga.getkey(aga.KEY_w)): fwd = self.move_speed
        if(aga.getkey(aga.KEY_s)): fwd = -self.move_speed
        if(aga.getkey(aga.KEY_a)): right = self.move_speed
        if(aga.getkey(aga.KEY_d)): right = -self.move_speed
        #
        self.trans.rot[1] = self.trans.rot[1] + motion[0] * self.sensitivity
        self.trans.rot[0] = self.trans.rot[0] + motion[1] * self.sensitivity
        #
        yaw = self.trans.rot[1] * aga.RADS
        cos = math.cos(yaw)
        ncos = math.cos(-yaw)
        sin = math.sin(yaw)
        #
        dx = fwd * -sin + right * cos
        dz = fwd * ncos + right * sin
        self.trans.pos[0] = self.trans.pos[0] + dx
        self.trans.pos[2] = self.trans.pos[2] + dz
        pos = self.trans.pos
        if(aga.getkey(aga.KEY_p)): aga.log([ -pos[0], -pos[1], -pos[2] ])
        #
        inside = 0
        #
        for bound in self.bounds:
            if(pos[0] > -bound[1][0] and pos[0] < -bound[2][0]):
                if(pos[2] > -bound[1][1] and pos[2] < -bound[2][1]):
                    inside = bound[0]
                    if(not inside and len(bound) > 3):
                        aga.text(bound[3], [ 0.0, 0.0 ])
        #
        if(not self.noclip and not inside):
            self.trans.pos[0] = self.trans.pos[0] - dx
            self.trans.pos[2] = self.trans.pos[2] - dz
        #
        aga.setcam(self.trans)
        #
        if(aga.getkey(aga.KEY_n)):
            if(not self.cmodekey):
                if(not self.noclip):
                    self.noclip = 1
                    aga.log('noclip enabled')
                else:
                    self.noclip = 0
                    aga.log('noclip disabled')
                self.cmodekey = 1
        elif(aga.getkey(aga.KEY_y)):
            if(not self.cmodekey):
                aga.debugdraw()
                aga.log('debugdraw toggled')
                self.cmodekey = 1
        elif(aga.getkey(aga.KEY_m)):
            if(not self.cmodekey):
                for i in range(len(self.scene)):
                    aga.dumpobj(self.scene[i], self.scenefiles[i])
                aga.log('scene re-exported')
                self.cmodekey = 1
        else: self.cmodekey = 0
