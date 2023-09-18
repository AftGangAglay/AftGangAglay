# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

# We're following the same vertex spec defined in `agacore.h'
# > struct aga_vertex {
# >     float col[4];
# >     float uv[2];
# >     float pos[3];
# >     float norm[3];
# > };

from numpy import float32, array
import pyassimp
from sys import argv

VERTSZ = 3 + 4 + 2 + 3

if len(argv) < 3:
    print(f'usage: {argv[0]} <input> <output>')
    exit(1)

data: list[float32] = list()
vertices: list[float32] = list()

with pyassimp.load(argv[1]) as scene:
    for mesh in scene.meshes:
        for i in range(len(mesh.vertices)):
            z: float32 = float32(0.0)
            zz = array([z, z], dtype=float32)
            zzz = array([z, z, z], dtype=float32)
            zzzz = array([z, z, z, z], dtype=float32)

            o: float32 = float32(1.0)
            oo = array([o, o], dtype=float32)
            ooo = array([o, o, o], dtype=float32)
            oooo = array([o, o, o, o], dtype=float32)

            vertices.extend(oooo)  # No color

            if len(mesh.texturecoords):
                a = mesh.texturecoords[0][i]
                vertices.append(a[0])
                vertices.append(a[1])
            else:
                vertices.extend(zz)

            if len(mesh.normals):
                a = mesh.normals[i]
                vertices.append(a[0])
                vertices.append(a[1])
                vertices.append(a[2])
            else:
                vertices.extend(zzz)

            if len(mesh.vertices):
                a = mesh.vertices[i]
                vertices.append(a[0])
                vertices.append(a[1])
                vertices.append(a[2])
            else:
                vertices.extend(zzz)

    for face in mesh.faces:
        for vertex in face:
            print(vertex)
            data.extend(vertices[vertex * VERTSZ:(vertex + 1) * VERTSZ])

data_s = array(data, dtype=float32)

with open(argv[2], 'wb+') as f:
    f.write(bytes(data_s))
