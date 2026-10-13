/*****************************************************************************
 * sub_renderer.h
 *****************************************************************************
 * Copyright (C) 2020 Videolabs
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

#ifndef HEIMDALL_SUB_RENDERER_H
#define HEIMDALL_SUB_RENDERER_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_opengl.h>

#include "gl_api.h"
#include "gl_common.h"
#include "interop.h"

struct heimdall_render_subpicture;

/**
 * A subpictures renderer handles the rendering of RGB subpictures.
 */
struct heimdall_gl_sub_renderer;

/**
 * Create a new subpictures renderer
 *
 * \param gl the GL context
 * \param api the OpenGL API
 */
struct heimdall_gl_sub_renderer *
heimdall_gl_sub_renderer_New(heimdall_gl_t *gl, const struct heimdall_gl_api *api,
                        struct heimdall_gl_interop *interop);

/**
 * Delete a subpictures renderer
 *
 * \param sr the renderer
 */
void
heimdall_gl_sub_renderer_Delete(struct heimdall_gl_sub_renderer *sr);

/**
 * Prepare the fragment shader
 *
 * Concretely, it allocates OpenGL textures if necessary and uploads the
 * picture.
 *
 * \param sr the renderer
 * \param subpicture the subpicture to render
 */
int
heimdall_gl_sub_renderer_Prepare(struct heimdall_gl_sub_renderer *sr,
                            const struct heimdall_render_subpicture *subpicture);

/**
 * Change the output size
 */
void
heimdall_gl_sub_renderer_SetOutputSize(struct heimdall_gl_sub_renderer *sr,
                                  unsigned width, unsigned height);

/**
 * Draw the prepared subpicture
 *
 * \param sr the renderer
 */
int
heimdall_gl_sub_renderer_Draw(struct heimdall_gl_sub_renderer *sr);

#endif
