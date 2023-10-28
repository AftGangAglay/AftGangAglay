# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

import aga
import cam

import posix

class game():
    def load(self, scene):
        scene_name = 'res/scene/' + scene
        self.scene = []
        aga.log('Loading scene `' + scene_name + '\'')
        for obj in posix.listdir(scene_name):
            if(obj[0] <> '.'):
                obj = 'res/scene/' + scene + '/' + obj
                self.scene.append(aga.mkobj(obj))
    #
    def create(self):
        aga.log('I wake!')
        #
        self.conf = aga.getconf()
        #
        # TODO: Water plant sidequest
        bounds = [ \
            [ cam.INNER, [   4.5 ,  4.5  ], [ - 4.0 , - 4.5  ] ], \
            [ cam.INNER, [   6.0 , -1.2  ], [ - 0.0 , - 3.75 ] ], \
            [ cam.INNER, [  23.0 ,  4.5  ], [   6.0 , -11.5  ] ], \
            [ cam.OUTER, [   0.5 ,  4.5  ], [ - 4.0 ,   1.0  ] ], \
            [ cam.OUTER, [ - 2.8 ,  1.0  ], [ - 4.0 , - 1.0  ], \
                'I\'ve had enough today' ], \
            [ cam.OUTER, [ - 1.15, -2.5 ], [ - 3.75, - 4.5  ], \
                'It\'s  been loading Floppy Bord for 3 weeks' ], \
            [ cam.OUTER, [   2.75, -3.0  ], [ - 2.25, - 4.5  ] ], \
            [ cam.OUTER, [   3.2 , -3.25 ], [   2.75, - 4.5  ], '...' ], \
            [ cam.OUTER, [   4.5 ,  2.0  ], [   4.3 , - 0.25 ], \
                'Trying my best' ], \
            [ cam.OUTER, [  23.0 , -5.25 ], [  14.0 , - 6.25 ] ], \
            [ cam.OUTER, [   4.5 ,  4.0  ], [   3.0 ,   2.75 ], \
                'You need some water' ], \
        ]
        #
        self.cam = cam.camera().create(self.conf, bounds)
        #
        self.audio_enabled = self.conf['audio_enabled']
        #
        if(self.audio_enabled):
            clipsrc = 'res/snd/PawnWithAShotgun.mp3.raw'
            self.clipfile = aga.largefile().create(clipsrc)
            self.clip = aga.clip().create(self.clipfile)
        #
        self.load('0')
        #
        lightpos = [ \
            [ - 3.0, 1.5,  0.0 ], \
            [  20.0, 0.5, -2.5 ] \
        ]
        self.lightparams = [ \
            [ 128.0, 0.0, 0.0, 0.1 ], \
            [ 128.0, 0.0, 0.0, 0.05 ] \
        ]
        self.lighttrans = []
        for l in range(len(lightpos)):
            self.lighttrans.append(aga.transform().create())
            self.lighttrans[l].pos = lightpos[l]
        #
        return self
    #
    def update(self):
        if(self.audio_enabled): self.clip.play()
        #
        self.cam.update()
        #
        aga.startlight()
        for i in range(len(self.lighttrans)):
            l0 = aga.mklight()
            t = self.lighttrans[i]
            aga.lightpos(l0, t)
            p = self.lightparams[i]
            aga.lightparam(l0, p)
        #
        for obj in self.scene:
            aga.putobj(obj)
    #
    def close(self):
        if(self.audio_enabled):
            self.clipfile.close()
            self.clip.close()
        for obj in self.scene:
            aga.killobj(obj)
