/*****************************************************************************
 * heimdall_opengl_filter.h
 *****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
 * Copyright (C) 2020-2023 Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Alexandre Janniaux <ajanni@videolabs.io>
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

#ifndef HEIMDALL_OPENGL_FILTER_H
#define HEIMDALL_OPENGL_FILTER_H

#include <heimdall_tick.h>
#include <heimdall_ancillary.h>
#include <heimdall_es.h>

struct heimdall_gl_filter;
struct heimdall_gl_sampler;
struct heimdall_gl_picture;

#ifdef __cplusplus
extern "C"
{
#endif

struct heimdall_gl_tex_size {
    unsigned width;
    unsigned height;
};

struct heimdall_gl_input_meta {
    heimdall_tick_t pts;
    unsigned plane;
    const heimdall_video_dovi_metadata_t *dovi_rpu;
    video_orientation_t orientation;
};

typedef int
heimdall_gl_filter_open_fn(struct heimdall_gl_filter *filter,
                      const config_chain_t *config,
                      struct heimdall_gl_sampler *sampler,
                      struct heimdall_gl_tex_size *size_out);

#define set_callback_opengl_filter(open) \
    { \
        heimdall_gl_filter_open_fn *fn = open; \
        (void) fn; \
        set_callback(fn); \
    }

struct heimdall_gl_filter_ops {
    /**
     * Draw the result of the filter to the current framebuffer
     */
    int (*draw)(struct heimdall_gl_filter *filter, const struct heimdall_gl_picture *pic,
                const struct heimdall_gl_input_meta *meta);

    /**
     * Free filter resources
     */
    void (*close)(struct heimdall_gl_filter *filter);

    /**
     * Request a (responsive) filter to adapt its output size (optional)
     *
     * A responsive filter is a filter for which the size of the produced
     * pictures depends on the output (e.g. display) size rather than the
     * input. This is for example the case for a renderer.
     *
     * A new output size is requested (size_out). The filter is authorized to
     * change the size_out to enforce its own constraints.
     *
     * In addition, it may request to the previous filter (if any) an optimal
     * size it wants to receive. If set to non-zero value, this previous filter
     * will receive this size as its requested size (and so on).
     *
     * \retval true if the resize is accepted (possibly with a modified
     *              size_out)
     * \retval false if the resize is rejected (included on error)
     */
    int (*request_output_size)(struct heimdall_gl_filter *filter,
                               struct heimdall_gl_tex_size *size_out,
                               struct heimdall_gl_tex_size *optimal_in);

    /**
     * Callback to notify input size changes
     *
     * When a filter changes its output size as a result of
     * request_output_size(), the next filter is notified by this callback.
     */
    void (*on_input_size_change)(struct heimdall_gl_filter *filter,
                                 const struct heimdall_gl_tex_size *size);
};

/**
 * OpenGL filter, in charge of a rendering pass.
 */
struct heimdall_gl_filter {
    heimdall_object_t obj;
    module_t *module;

    struct heimdall_gl_t *gl;
    struct heimdall_gl_sampler *sampler;

    struct {
        /**
         * An OpenGL filter may either operate on the input RGBA picture, or on
         * individual input planes (without chroma conversion) separately.
         *
         * In practice, this is useful for deinterlace filters.
         *
         * This flag must be set by the filter module (default is false).
         */
        bool filter_planes;

        /**
         * A blend filter draws over the input picture (without reading it).
         *
         * Meaningless if filter_planes is true.
         *
         * This flag must be set by the filter module (default is false).
         */
        bool blend;

        /**
         * Request MSAA level.
         *
         * This value must be set by the filter module (default is 0, which
         * means disabled).
         *
         * Meaningless if filter_planes is true.
         *
         * The actual MSAA level may be overwritten to 0 if multisampling is
         * not supported, or to a higher value if another filter rendering on
         * the same framebuffer requested a higher MSAA level.
         */
        unsigned msaa_level;
    } config;

    const struct heimdall_gl_filter_ops *ops;
    void *sys;
};

#ifdef __cplusplus
}
#endif

#endif
