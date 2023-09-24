# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

import aga

import math

class game():
    def create(self):
        aga.log('I wake!')
        #
        self.trans = aga.transform().create()
        self.conf = aga.getconf()
        #
        self.sensitivity = self.conf['sensitivity']
        self.move_speed = self.conf['move_speed']
        #
        try:
            clipsrc = 'res/snd/PawnWithAShotgun.mp3.raw'
            self.clipfile = aga.largefile().create(clipsrc)
            self.clip = aga.clip().create(self.clipfile)
        except:
            pass
        #
        self.origin = aga.transform().create()
        self.skybox = aga.mkobj('res/scene/0/skybox.sgml')
        self.thing = aga.mkobj('res/scene/0/thing.sgml')
        self.env = aga.mkobj('res/scene/0/env.sgml')
        #
        return self
    #
    def control(self):
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
        self.trans.pos[0] = self.trans.pos[0] + fwd * -sin + right * cos
        self.trans.pos[2] = self.trans.pos[2] + fwd * ncos + right * sin
        #
        aga.setcam(self.trans)
    #
    def update(self):
        self.control()
        #
        aga.startlight()
        l0 = aga.mklight()
        aga.lightpos(l0, self.origin)
        aga.lightparam(l0, [ 128.0, 0.0, 0.1, 0.01 ])
        #
        aga.nolight()
        aga.putobj(self.skybox)
        aga.yeslight()
        #
        aga.putobj(self.env)
        aga.putobj(self.thing)
        #
        self.clip.play()
    #
    def close(self):
        self.clipfile.close()
        self.clip.close()
        aga.killobj(self.skybox)
        aga.killobj(self.env)
        aga.killobj(self.thing)
