/*****************************************************************************
 * filter_priv.h
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

#ifndef HEIMDALL_GL_FILTER_PRIV_H
#define HEIMDALL_GL_FILTER_PRIV_H

#include <heimdall_common.h>
#include <heimdall_list.h>
#include <heimdall_picture.h>
#include <heimdall_opengl_filter.h>

#include "gl_api.h"
#include "picture.h"

struct heimdall_gl_filter_priv {
    struct heimdall_gl_filter filter;

    struct heimdall_gl_api api;

    /* For a blend filter, must be the same as the size_out of the previous
     * filter */
    struct heimdall_gl_tex_size size_out;

    struct heimdall_gl_format glfmt_in;

    /* Describe the output planes, independently of whether textures are
     * created for this filter (the last filter does not own any textures). */
    unsigned plane_count;
    GLsizei plane_widths[PICTURE_PLANE_MAX];
    GLsizei plane_heights[PICTURE_PLANE_MAX];

    /* owned (this filter must delete it) */
    GLuint framebuffers_out[PICTURE_PLANE_MAX];

    /* owned (each attached to framebuffers_out[i]) */
    GLuint textures_out[PICTURE_PLANE_MAX];
    GLsizei tex_widths[PICTURE_PLANE_MAX];
    GLsizei tex_heights[PICTURE_PLANE_MAX];
    unsigned tex_count;

    /* For multisampling, if msaa_level != 0 */
    GLuint framebuffer_msaa; /* owned */
    GLuint renderbuffer_msaa; /* owned (attached to framebuffer_msaa) */
    /* } */

    struct heimdall_list node; /**< node of heimdall_gl_filters.list */

    /* Blend filters are attached to their non-blend "parent" instead of the
     * filter chain to simplify the rendering code */
    struct heimdall_list blend_subfilters; /**< list of heimdall_gl_filter_priv.node */

    bool has_picture;
};

static inline struct heimdall_gl_filter_priv *
heimdall_gl_filter_PRIV(struct heimdall_gl_filter *filter)
{
    return container_of(filter, struct heimdall_gl_filter_priv, filter);
}

struct heimdall_gl_filter *
heimdall_gl_filter_New(struct heimdall_gl_t *gl);

int
heimdall_gl_filter_LoadModule(heimdall_object_t *parent, const char *name,
                         struct heimdall_gl_filter *filter,
                         const config_chain_t *config,
                         struct heimdall_gl_sampler *sampler,
                         struct heimdall_gl_tex_size *size_out);
#define heimdall_gl_filter_LoadModule(o, a, b, c, d, e) \
    heimdall_gl_filter_LoadModule(HEIMDALL_OBJECT(o), a, b, c, d, e)

void
heimdall_gl_filter_Delete(struct heimdall_gl_filter *filter);

int
heimdall_gl_filter_InitFramebuffers(struct heimdall_gl_filter *filter, bool is_last);

/** Recompute plane count, widths and heights after size_out have changed */
void
heimdall_gl_filter_InitPlaneSizes(struct heimdall_gl_filter *filter);

void
heimdall_gl_filter_ApplyOutputSize(struct heimdall_gl_filter *filter);

#endif
