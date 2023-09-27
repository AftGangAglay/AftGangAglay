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
        self.audio_enabled = self.conf['audio_enabled']
        #
        if(self.audio_enabled):
            clipsrc = 'res/snd/PawnWithAShotgun.mp3.raw'
            self.clipfile = aga.largefile().create(clipsrc)
            self.clip = aga.clip().create(self.clipfile)
        #
        self.origin = aga.transform().create()
        #
        self.scene = []
        self.scenefiles = [ \
            'res/scene/0/skybox.sgml', \
            'res/scene/0/thing.sgml', \
            'res/scene/0/plane.sgml', \
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
        self.trans.pos[0] = self.trans.pos[0] + fwd * -sin + right * cos
        self.trans.pos[2] = self.trans.pos[2] + fwd * ncos + right * sin
        #
        aga.setcam(self.trans)
        #
        # Creative mode stuff
        if(aga.getkey(aga.KEY_q)):
            if(not self.cmodekey):
                self.selected = self.selected - 1
                if(self.selected < 0): self.selected = len(self.scene) - 1
                aga.log('Selected `' + self.scenefiles[self.selected] + '\'')
                self.cmodekey = 1
        elif(aga.getkey(aga.KEY_e)):
            if(not self.cmodekey):
                self.selected = self.selected + 1
                if(self.selected >= len(self.scene)): self.selected = 0
                aga.log('Selected `' + self.scenefiles[self.selected] + '\'')
                self.cmodekey = 1
        elif(aga.getkey(aga.KEY_z)):
            if(not self.cmodekey):
                self.cmodeaspect = self.cmodeaspect + 1
                if(self.cmodeaspect > 2): self.cmodeaspect = 0
                aga.log(['pos', 'rot', 'scale'][self.cmodeaspect])
                self.cmodekey = 1
        elif(aga.getkey(aga.KEY_c)):
            if(not self.cmodekey):
                self.cmodeaspect = self.cmodeaspect - 1
                if(self.cmodeaspect < 0): self.cmodeaspect = 2
                aga.log(['pos', 'rot', 'scale'][self.cmodeaspect])
                self.cmodekey = 1
        elif(aga.getkey(aga.KEY_minus)):
            if(not self.cmodekey):
                self.cmodespd = self.cmodespd - 0.1
                if(self.cmodespd <= 0.1): self.cmodespd = 0.1
                aga.log(self.cmodespd)
                self.cmodekey = 1
        elif(aga.getkey(aga.KEY_equal)):
            if(not self.cmodekey):
                self.cmodespd = self.cmodespd + 0.1
                aga.log(self.cmodespd)
                self.cmodekey = 1
        elif(aga.getkey(aga.KEY_p)):
            if(not self.cmodekey):
                aga.log('Saving scene...')
                for i in range(len(self.scene)):
                    aga.dumpobj(self.scene[i], self.scenefiles[i])
                aga.log('Saved!')
                self.cmodekey = 1
        else: self.cmodekey = 0
        #
        t = aga.objtrans(self.scene[self.selected])
        if(self.cmodeaspect = 0):
            if(aga.getkey(aga.KEY_i)): t.pos[1] = t.pos[1] + self.cmodespd
            if(aga.getkey(aga.KEY_k)): t.pos[1] = t.pos[1] - self.cmodespd
            if(aga.getkey(aga.KEY_j)): t.pos[0] = t.pos[0] - self.cmodespd
            if(aga.getkey(aga.KEY_l)): t.pos[0] = t.pos[0] + self.cmodespd
            if(aga.getkey(aga.KEY_u)): t.pos[2] = t.pos[2] + self.cmodespd
            if(aga.getkey(aga.KEY_o)): t.pos[2] = t.pos[2] - self.cmodespd
        elif(self.cmodeaspect = 1):
            if(aga.getkey(aga.KEY_i)): t.rot[1] = t.rot[1] + self.cmodespd
            if(aga.getkey(aga.KEY_k)): t.rot[1] = t.rot[1] - self.cmodespd
            if(aga.getkey(aga.KEY_j)): t.rot[0] = t.rot[0] - self.cmodespd
            if(aga.getkey(aga.KEY_l)): t.rot[0] = t.rot[0] + self.cmodespd
            if(aga.getkey(aga.KEY_u)): t.rot[2] = t.rot[2] + self.cmodespd
            if(aga.getkey(aga.KEY_o)): t.rot[2] = t.rot[2] - self.cmodespd
        elif(self.cmodeaspect = 2):
            if(aga.getkey(aga.KEY_i)): t.scale[1] = t.scale[1] + self.cmodespd
            if(aga.getkey(aga.KEY_k)): t.scale[1] = t.scale[1] - self.cmodespd
            if(aga.getkey(aga.KEY_j)): t.scale[0] = t.scale[0] - self.cmodespd
            if(aga.getkey(aga.KEY_l)): t.scale[0] = t.scale[0] + self.cmodespd
            if(aga.getkey(aga.KEY_u)): t.scale[2] = t.scale[2] + self.cmodespd
            if(aga.getkey(aga.KEY_o)): t.scale[2] = t.scale[2] - self.cmodespd
        #
    #
    def update(self):
        if(self.audio_enabled): self.clip.play()
        #
        self.control()
        #
        aga.startlight()
        l0 = aga.mklight()
        aga.lightpos(l0, self.origin)
        aga.lightparam(l0, [ 128.0, 0.0, 0.0, 0.0005 ])
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
