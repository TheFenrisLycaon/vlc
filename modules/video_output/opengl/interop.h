/*****************************************************************************
 * interop.h
 *****************************************************************************
 * Copyright (C) 2019 Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef HEIMDALL_GL_INTEROP_PRIV_H
#define HEIMDALL_GL_INTEROP_PRIV_H

#include <heimdall_common.h>
#include <heimdall_opengl.h>
#include <heimdall_picture.h>
#include <heimdall_picture_pool.h>
#include <heimdall_opengl_interop.h>
#include "gl_common.h"


struct heimdall_gl_interop *
heimdall_gl_interop_New(struct heimdall_gl_t *gl, heimdall_video_context *context,
                   const video_format_t *fmt);

struct heimdall_gl_interop *
heimdall_gl_interop_NewForSubpictures(struct heimdall_gl_t *gl);

void
heimdall_gl_interop_Delete(struct heimdall_gl_interop *interop);

int
heimdall_gl_interop_GenerateTextures(const struct heimdall_gl_interop *interop,
                                const GLsizei *tex_width,
                                const GLsizei *tex_height, GLuint *textures);

void
heimdall_gl_interop_DeleteTextures(const struct heimdall_gl_interop *interop,
                              GLuint *textures);

#endif
