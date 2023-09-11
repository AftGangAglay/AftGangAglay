# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

import test2

class fizz():
    def create(self):
        print(self)
        print('python class fizz!')
        self.x = 4
    #
    def update(self):
        print('update!')


class buzz():
    def create(self):
        print('python class buzz!')
    #
    def update(self):
        print('update!')
