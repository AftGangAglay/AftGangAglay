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
            self.file = aga.largefile().create('res/model/thing.obj.raw')
            self.buf = aga.vertexbuffer().create(self.file)
        except:
            pass
        self.modeltrans = aga.transform().create()
        #
        try:
            self.tex = aga.texture().create('res/img/arse.tiff')
        except:
            pass
        #
        try:
            self.spherefile = aga.largefile().create('res/model/sphere.obj.raw')
            self.spherebuf = aga.vertexbuffer().create(self.spherefile)
        except:
            pass
        self.spheretrans = aga.transform().create()
        self.spheretrans.scale[0] = 80.0
        self.spheretrans.scale[1] = 80.0
        self.spheretrans.scale[2] = 80.0
        #
        try:
            self.testtex = aga.texture().create('res/img/test.tiff')
        except:
            pass
        #
        try:
            self.face = aga.texture().create('res/img/BlackKnight.tiff')
        except:
            pass
        #
        try:
            clipsrc = 'res/snd/PawnWithAShotgun.mp3.raw'
            self.clipfile = aga.largefile().create(clipsrc)
            self.clip = aga.clip().create(self.clipfile)
        except:
            pass
        #
        try:
            self.env0 = aga.largefile().create('res/model/env0.obj.raw')
            self.env0buf = aga.vertexbuffer().create(self.env0)
        except:
            pass
        self.env0trans = aga.transform().create()
        self.env0trans.pos[1] = -5.0
        self.env0trans.scale[0] = 5.0
        self.env0trans.scale[1] = 5.0
        self.env0trans.scale[2] = 5.0
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
        aga.startlight()
        l0 = aga.mklight()
        aga.lightpos(l0, self.spheretrans)
        aga.lightparam(l0, [ 128.0, 0.0, 0.1, 0.01 ])
        #
        aga.nolight()
        self.face.use()
        self.spheretrans.rot[0] = self.spheretrans.rot[0] + 0.5
        self.spheretrans.rot[1] = self.spheretrans.rot[1] + 0.5
        self.spheretrans.rot[2] = self.spheretrans.rot[2] + 0.5
        self.spherebuf.draw(aga.TRIANGLES, self.spheretrans)
        aga.yeslight()
        #
        self.testtex.use()
        self.env0buf.draw(aga.TRIANGLES, self.env0trans)
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
        self.env0.close()
        self.env0buf.close()
