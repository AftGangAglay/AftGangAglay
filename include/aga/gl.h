/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_GL_H
#define AGA_GL_H

#include <asys/base.h>
#include <asys/result.h>

#ifdef ASYS_WIN32
# ifdef ASYS_VISUALC
#  pragma warning(push)
#  pragma warning(disable: 4255) /* Function with `()' prototype. */
#  pragma warning(disable: 4668) /* Symbol not defined as macro. */
# endif

/*
 * This is super annoying as it leaks a load of garbage into scope.
 * `windows.h' is needed for declaration attributes.
 */
# include <windows.h>
# include <GL/gl.h>
# include <GL/glu.h>

# ifdef ASYS_VISUALC
#  pragma warning(pop)
# endif
#else
# define GL_GLEXT_PROTOTYPES
# include <GL/gl.h>
# include <GL/glext.h>
# include <GL/glu.h>
# include <GL/glx.h>
# undef GL_GLEXT_PROTOTYPES
#endif

#endif
