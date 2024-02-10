# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

XQUARTZ_ROOT = /opt/X11

ifdef WINDOWS
	WGL = 1
else
	GLX = 1
endif

ifdef WGL
	GLABI_CFLAGS += -DAGA_WGL
	GLABI_LDLIBS += -lopengl32 -lglu32 -lgdi32 -lshell32
endif

ifdef GLX
	GLABI_CFLAGS += -DAGA_GLX
	GLABI_LDLIBS += -lGL -lGLU -lX11
	ifdef APPLE
		GLABI_CFLAGS += -I$(XQUARTZ_ROOT)/include
		GLABI_LDLIBS += -L$(XQUARTZ_ROOT)/lib
	endif
endif

