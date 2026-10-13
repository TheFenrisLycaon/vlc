/*****************************************************************************
 * filters.h
 *****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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

#ifndef HEIMDALL_GL_FILTERS_H
#define HEIMDALL_GL_FILTERS_H

#include <heimdall_common.h>
#include <heimdall_list.h>
#include <heimdall_opengl.h>
#include <heimdall_tick.h>

#include "gl_api.h"
#include "interop.h"
#include "sampler.h"

struct heimdall_gl_filters;
struct heimdall_gl_filter;

/**
 * Create a new OpenGL filter chain
 *
 * \param gl the OpenGL context
 * \param api the OpenGL api
 * \param interop the interop to use for the sampler of the first filter
 */
struct heimdall_gl_filters *
heimdall_gl_filters_New(struct heimdall_gl_t *gl, const struct heimdall_gl_api *api,
                   struct heimdall_gl_interop *interop,
                   video_orientation_t orientation);

/**
 * Delete the OpenGL filter chain
 *
 * \param filters the filter chain
 */
void
heimdall_gl_filters_Delete(struct heimdall_gl_filters *filters);

/**
 * Create and append a filter loaded from a module to the filter chain
 *
 * The created filter is owned by the filter chain.
 *
 * \param filters the filter chain
 * \param name the module name
 * \param config the module configuration
 * \return a weak reference to the filter (NULL on error)
 */
struct heimdall_gl_filter *
heimdall_gl_filters_Append(struct heimdall_gl_filters *filters, const char *name,
                      const config_chain_t *config);

struct heimdall_gl_filter *
heimdall_gl_filters_Replace(struct heimdall_gl_filters *filters,
                       struct heimdall_gl_filter *filter,
                       const char *name,
                       const config_chain_t *config);

/**
 * Init the framebuffers for the appended filters.
 *
 * This function must be called once after all filters have been appended. It
 * is an error to call heimdall_gl_filters_Append() after this function.
 *
 * \param filters the filter chain
 * \return HEIMDALL_SUCCESS on success, another value on error
 */
int
heimdall_gl_filters_InitFramebuffers(struct heimdall_gl_filters *filters);

/**
 * Update the input picture to pass to the first filter
 *
 * \param filters the filter chain
 * \param picture the new input picture
 * \return HEIMDALL_SUCCESS on success, another value on error
 */
int
heimdall_gl_filters_UpdatePicture(struct heimdall_gl_filters *filters,
                             picture_t *picture);

/**
 * Draw by executing all the filters
 *
 * \param filters the filter chain
 * \return HEIMDALL_SUCCESS on success, another value on error
 */
int
heimdall_gl_filters_Draw(struct heimdall_gl_filters *filters);

/**
 * Set the viewport.
 */
void
heimdall_gl_filters_SetViewport(struct heimdall_gl_filters *filters, int x, int y,
                           unsigned width, unsigned height);

/**
 * Change the output size
 */
int
heimdall_gl_filters_SetOutputSize(struct heimdall_gl_filters *filters, unsigned width,
                             unsigned height);

#endif
