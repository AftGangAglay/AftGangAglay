/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_GL_H
#define AGA_GL_H

#include <agaresult.h>

#ifdef _WIN32
/*
 * This is super annoying as it leaks a load of garbage into scope.
 * `windows.h' is needed for declaration attributes.
 * `stddef.h' is needed for `wchar_t'.
 * TODO: `stddef.h' isn't needed under MSVC if the appropriate opt is set.
 */
# ifndef AGA_WGL_SUPPRESS_AUX

#  include <windows.h>
#  include <stddef.h>

# endif

# include <GL/gl.h>
# include <GL/glu.h>

#else
# define GL_GLEXT_PROTOTYPES
# include <GL/gl.h>
# include <GL/glext.h>
# include <GL/glu.h>
# include <GL/glx.h>
# undef GL_GLEXT_PROTOTYPES
#endif

#ifdef AGA_NO_VERIFY
# define AGA_GL_CHK(proc) (void) proc
#else
# define AGA_GL_CHK(proc) \
    do { \
        enum aga_result err = aga_glerr(__FILE__, proc); \
        if(err) return err; \
    } while(0)
#endif

enum aga_result aga_glerr(const char* loc, const char* proc);

#endif
