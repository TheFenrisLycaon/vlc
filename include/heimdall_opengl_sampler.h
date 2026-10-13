/*****************************************************************************
 * heimdall_opengl_sampler.h: Heimdall OpenGL sampler API
 *****************************************************************************
 * Copyright (C) 2020-2026 VLC authors and VideoLAN
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

#ifndef HEIMDALL_GL_SAMPLER_H
#define HEIMDALL_GL_SAMPLER_H 1

#include <stdint.h>
#include <heimdall_es.h>
#include <heimdall_picture.h>
#include <heimdall_opengl_picture.h>

/**
 * \file
 * This file defines the public OpenGL sampler interface.
 *
 * The purpose of a sampler is to provide pixel values of a Heimdall input picture,
 * stored in any format.
 *
 * Concretely, a GLSL function:
 *
 *     vec4 heimdall_texture(vec2 coords)
 *
 * returns the RGBA values for the given coordinates.
 *
 * Contrary to the standard GLSL function:
 *
 *     vec4 texture2D(sampler2D sampler, vec2 coords)
 *
 * it does not take a sampler2D as parameter. The role of the sampler is to
 * abstract the input picture from the filter, so the input picture is
 * implicitly available.
 */

struct heimdall_gl_sampler;

struct heimdall_gl_sampler_ops {
    /**
     * Callback to fetch locations of uniform or attribute variables
     *
     * This function pointer cannot be NULL. This callback is called one time
     * after the program is linked.
     *
     * \param sampler the sampler
     * \param program the linked GL program handle
     */
    void (*fetch_locations)(struct heimdall_gl_sampler *sampler, uint32_t program);

    /**
     * Callback to load sampler data
     *
     * This function pointer cannot be NULL. This callback can be used to
     * specify values of uniform variables.
     *
     * \param sampler the sampler
     */
    void (*load)(struct heimdall_gl_sampler *sampler);

    /**
     * Update the input picture
     *
     * \param sampler the sampler
     * \param picture the OpenGL picture
     * \return HEIMDALL_SUCCESS or a Heimdall error
     */
    int (*update)(struct heimdall_gl_sampler *sampler,
                  const struct heimdall_gl_picture *picture);

    /**
     * Select the plane to expose
     *
     * If the sampler exposes planes separately (for plane filters), select the
     * plane to expose via the GLSL function heimdall_texture().
     *
     * \param sampler the sampler
     * \param plane the plane number
     */
    void (*select_plane)(struct heimdall_gl_sampler *sampler, unsigned plane);

    /**
     * Close the sampler, releasing resources
     *
     * \param sampler the sampler
     */
    void (*close)(struct heimdall_gl_sampler *sampler);
};

/**
 * OpenGL sampler
 *
 * Generates GLSL code for sampling input textures and handles chroma
 * conversion, transfer functions, and color space mapping.
 */
struct heimdall_gl_sampler {
    heimdall_object_t obj;
    module_t *module;

    /** OpenGL context */
    struct heimdall_gl_t *gl;

    /** Input video format */
    video_format_t fmt_in;

    /** Number of texture planes */
    unsigned tex_count;

    /** Per-plane texture widths */
    int32_t tex_widths[PICTURE_PLANE_MAX];

    /** Per-plane texture heights */
    int32_t tex_heights[PICTURE_PLANE_MAX];

    /** GL texture target (e.g. GL_TEXTURE_2D) */
    uint32_t tex_target;

    /** Per-plane GL format (GLenum values) */
    uint32_t formats[PICTURE_PLANE_MAX];

    /** Whether textures use half-float storage */
    bool half_float;

    /**
     * Matrix to convert from picture coordinates to texture coordinates
     *
     * The matrix is 2x3 and is stored in column-major order:
     *
     *     / a b c \
     *     \ d e f /
     *
     * It is stored as an array of 6 floats:
     *
     *     [a, d, b, e, c, f]
     *
     * It is NULL before the first picture is available and may theoretically
     * change on every picture.
     */
    const float *pic_to_tex_matrix;

    struct {
        /**
         * Version header appropriate for this shader.
         */
        char *version;

        /**
         * Precision preamble appropriate for this shader.
         */
        char *precision;

        /**
         * Piece of fragment shader code declaring OpenGL extensions.
         *
         * It is initialized by the sampler, and may be NULL if no extensions
         * are required.
         *
         * If non-NULL, users of this sampler must inject this code into their
         * fragment shader, immediately after the "version" line.
         */
        char *extensions;

        /**
         * Piece of fragment shader code providing the GLSL function
         * heimdall_texture(vec2 coords).
         *
         * It is initialized by the sampler, and is never NULL.
         *
         * Users of this sampler should inject this code into their fragment
         * shader, before any call to heimdall_texture().
         */
        char *body;
    } shader;

    const struct heimdall_gl_sampler_ops *ops;

    /** Private data for the sampler implementation */
    void *sys;
};

/**
 * \copydoc heimdall_gl_sampler_ops::fetch_locations
 */
static inline void
heimdall_gl_sampler_FetchLocations(struct heimdall_gl_sampler *sampler, uint32_t program)
{
    sampler->ops->fetch_locations(sampler, program);
}

/**
 * \copydoc heimdall_gl_sampler_ops::load
 */
static inline void
heimdall_gl_sampler_Load(struct heimdall_gl_sampler *sampler)
{
    sampler->ops->load(sampler);
}

/**
 * \copydoc heimdall_gl_sampler_ops::update
 */
static inline int
heimdall_gl_sampler_Update(struct heimdall_gl_sampler *sampler,
                      const struct heimdall_gl_picture *picture)
{
    return sampler->ops->update(sampler, picture);
}

/**
 * \copydoc heimdall_gl_sampler_ops::select_plane
 */
static inline void
heimdall_gl_sampler_SelectPlane(struct heimdall_gl_sampler *sampler, unsigned plane)
{
    sampler->ops->select_plane(sampler, plane);
}

/**
 * Activation function for OpenGL sampler module implementations.
 *
 * The input format is described by the public fields of the sampler object
 * which are populated before the module is loaded.
 *
 * \param sampler the sampler
 * \param expose_planes if set, heimdall_texture() exposes a single plane at a time
 *
 * \return HEIMDALL_SUCCESS when the module can be opened
 */
typedef int
heimdall_gl_sampler_open_fn(struct heimdall_gl_sampler *sampler, bool expose_planes);

#define set_callback_opengl_sampler(open) \
    { \
        heimdall_gl_sampler_open_fn *fn = open; \
        (void) fn; \
        set_callback(fn); \
    }

#endif /* HEIMDALL_GL_SAMPLER_H */
