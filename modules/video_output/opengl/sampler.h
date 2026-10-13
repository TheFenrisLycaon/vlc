/*****************************************************************************
 * sampler.h
 *****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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

#ifndef HEIMDALL_GL_SAMPLER_PRIV_H
#define HEIMDALL_GL_SAMPLER_PRIV_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_opengl.h>
#include <heimdall_opengl_sampler.h>

#include "gl_common.h"
#include "picture.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Create a new sampler
 *
 * \param gl the OpenGL context
 * \param glfmt the input format
 * \param expose_planes if set, heimdall_texture() exposes a single plane at a time
 */
struct heimdall_gl_sampler *
heimdall_gl_sampler_New(struct heimdall_gl_t *gl,
                   const struct heimdall_gl_format *glfmt, bool expose_planes);

/**
 * Delete a sampler
 *
 * \param sampler the sampler
 */
void
heimdall_gl_sampler_Delete(struct heimdall_gl_sampler *sampler);

#ifdef __cplusplus
}
#endif

#endif
