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
        self.file = aga.largefile().create('res/thing.raw')
        self.buf = aga.vertexbuffer().create(self.file)
        self.modeltrans = aga.transform().create()
        #
        self.tex = aga.texture().create('res/arse.tiff')
        #
        self.spherefile = aga.largefile().create('res/sphere.raw')
        self.spherebuf = aga.vertexbuffer().create(self.spherefile)
        self.spheretrans = aga.transform().create()
        self.spheretrans.scale[0] = 40.0
        self.spheretrans.scale[1] = 40.0
        self.spheretrans.scale[2] = 40.0
        #
        self.testtex = aga.texture().create('res/test.tiff')
        #
        self.clipfile = aga.largefile().create('res/nggyu-u8pcm-8k.raw')
        self.clip = aga.clip().create(self.clipfile)
        #
        return self
    #
    def control(self):
        motion = aga.getmotion()
        #
        right = 0.0
        fwd = 0.0
        #
        if(aga.getkey(aga.KEY_w)):
            fwd = self.move_speed
        if(aga.getkey(aga.KEY_s)):
            fwd = -self.move_speed
        if(aga.getkey(aga.KEY_a)):
            right = self.move_speed
        if(aga.getkey(aga.KEY_d)):
            right = -self.move_speed
        #
        # TODO: Get in input settings
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
        self.testtex.use()
        self.spheretrans.rot[0] = self.spheretrans.rot[0] + 0.5
        self.spheretrans.rot[1] = self.spheretrans.rot[1] + 0.5
        self.spheretrans.rot[2] = self.spheretrans.rot[2] + 0.5
        self.spherebuf.draw(aga.TRIANGLES, self.spheretrans)
        #
        self.tex.use()
        self.modeltrans.pos[0] = self.modeltrans.pos[0] + 0.01
        self.modeltrans.rot[1] = self.modeltrans.rot[1] + 1.0
        self.modeltrans.scale[2] = self.modeltrans.scale[2] + 0.005
        self.buf.draw(aga.TRIANGLES, self.modeltrans)
        #
        self.clip.play()
    #
    def close(self):
        self.file.close()
        self.buf.close()
        self.tex.close()
        self.spherefile.close()
        self.spherebuf.close()
        self.testtex.close()
        self.clipfile.close()
        self.clip.close()
