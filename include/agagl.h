/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_GL_H
#define AGA_GL_H

#include <agaresult.h>

#ifdef _WIN32
# ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4255) /* Function with `()' prototype. */
#  pragma warning(disable: 4668) /* Symbol not defined as macro. */
# endif
/*
 * This is super annoying as it leaks a load of garbage into scope.
 * `windows.h' is needed for declaration attributes.
 * `stddef.h' is needed for `wchar_t'.
 */
# ifndef AGA_WGL_SUPPRESS_AUX
#  define AGA_WANT_WINDOWS_H

#  include <agaw32.h>
#  ifndef _WCHAR_T_DEFINED
#   include <agastd.h>
#  endif

# endif

# include <GL/gl.h>
# include <GL/glu.h>

# ifdef _MSC_VER
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
