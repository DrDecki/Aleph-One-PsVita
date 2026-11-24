#ifndef _OGL_HEADERS_
#define _OGL_HEADERS_

/*

	Copyright (C) 2009 by Gregory Smith
	and the "Aleph One" developers.
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

	Uniform header for all Aleph One OpenGL users
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_OPENGL

#ifdef __vita__
#include <vitaGL.h>
#include <string.h>
#ifndef VITA_GL_COMPAT
#define VITA_GL_COMPAT
static inline void glColor3us(GLushort r, GLushort g, GLushort b) { glColor4f(r / 65535.0f, g / 65535.0f, b / 65535.0f, 1.0f); }
static inline void glColor4usv(const GLushort *v) { glColor4f(v[0] / 65535.0f, v[1] / 65535.0f, v[2] / 65535.0f, v[3] / 65535.0f); }
static inline void glColor4us(GLushort r, GLushort g, GLushort b, GLushort a) { glColor4f(r / 65535.0f, g / 65535.0f, b / 65535.0f, a / 65535.0f); }
static inline void glColor3usv(const GLushort *v) { glColor4f(v[0] / 65535.0f, v[1] / 65535.0f, v[2] / 65535.0f, 1.0f); }
static inline void glMultMatrixd(const double *m) { GLfloat f[16]; for (int i = 0; i < 16; i++) f[i] = (GLfloat)m[i]; glMultMatrixf(f); }
static inline GLint gluScaleImage(GLenum format, GLint wi, GLint hi, GLenum ti, const void *in, GLint wo, GLint ho, GLenum to, void *out)
{
	if (ti != GL_UNSIGNED_BYTE || to != GL_UNSIGNED_BYTE || wi <= 0 || hi <= 0) return 1;
	int bpp = (format == GL_RGBA) ? 4 : (format == GL_RGB) ? 3 : (format == GL_LUMINANCE_ALPHA) ? 2 : 1;
	const unsigned char *src = (const unsigned char *)in;
	unsigned char *dst = (unsigned char *)out;
	for (int y = 0; y < ho; y++) {
		int sy = y * hi / ho;
		for (int x = 0; x < wo; x++) {
			int sx = x * wi / wo;
			memcpy(dst + (y * wo + x) * bpp, src + (sy * wi + sx) * bpp, bpp);
		}
	}
	return 0;
}
#endif
#else

#ifdef __WIN32__

#define GLEW_STATIC 1
#include <GL/glew.h>

#else

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <SDL2/SDL_opengl.h>

#if defined (__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#endif

#endif

#endif

#endif
