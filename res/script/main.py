# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

import aga

import math

class game():
    def create(self):
        aga.log('I wake!')
        #
        self.trans = aga.transform().create()
        self.trans.pos[1] = -2.0
        self.conf = aga.getconf()
        #
        self.sensitivity = self.conf['sensitivity']
        self.move_speed = self.conf['move_speed']
        self.audio_enabled = self.conf['audio_enabled']
        #
        if(self.audio_enabled):
            clipsrc = 'res/snd/PawnWithAShotgun.mp3.raw'
            self.clipfile = aga.largefile().create(clipsrc)
            self.clip = aga.clip().create(self.clipfile)
        #
        self.origin = aga.transform().create()
        self.upstairs_light = aga.transform().create()
        self.upstairs_light.pos = [ -14.0, -5.0, 4.0 ]
        #
        self.bounds = [ \
            [[  4.5,  4.5 ], [ -4.0,  -4.5  ]], \
            [[  6.0, -1.2 ], [ -0.0,  -3.75 ]], \
            [[ 23.0,  4.5 ], [  6.0, -11.5  ]] \
        ]
        self.noclip = 0
        #
        self.scene = []
        self.scenefiles = [ \
            'res/scene/0/skybox.sgml', \
            'res/scene/0/thing.sgml', \
            'res/scene/0/bedside.sgml', \
            'res/scene/0/lamp.sgml', \
            'res/scene/0/env.sgml' ]
        for p in self.scenefiles:
            self.scene.append(aga.mkobj(p))
        #
        self.selected = 0
        self.cmodekey = 0
        self.cmodeaspect = 0
        self.cmodespd = 0.1
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
        dx = fwd * -sin + right * cos
        dz = fwd * ncos + right * sin
        self.trans.pos[0] = self.trans.pos[0] + dx
        self.trans.pos[2] = self.trans.pos[2] + dz
        x = self.trans.pos[0]
        z = self.trans.pos[2]
        if(aga.getkey(aga.KEY_equal)): aga.log(self.trans.pos)
        #
        inside = 0
        #
        for bound in self.bounds:
            if(x > -bound[0][0] and x < -bound[1][0]):
                if(z > -bound[0][1] and z < -bound[1][1]):
                    inside = 1
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
        else: self.cmodekey = 0
    #
    def update(self):
        if(self.audio_enabled): self.clip.play()
        #
        self.control()
        #
        aga.startlight()
        l0 = aga.mklight()
        lighttrans = aga.transform().create()
        lighttrans.pos[0] = -3.0
        lighttrans.pos[1] = 1.5
        lighttrans.pos[2] = 0.0
        aga.lightpos(l0, lighttrans)
        aga.lightparam(l0, [ 0.0, 0.0, 0.0, 0.05 ])
        #
        aga.yeslight()
        for obj in self.scene:
            aga.putobj(obj)
    #
    def close(self):
        if(self.audio_enabled):
            self.clipfile.close()
            self.clip.close()
        for obj in self.scene:
            aga.killobj(obj)
