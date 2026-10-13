/*****************************************************************************
 * renderer.h: OpenGL internal header
 *****************************************************************************
 * Copyright (C) 2016 VLC authors and VideoLAN
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

#ifndef HEIMDALL_GL_RENDERER_H
#define HEIMDALL_GL_RENDERER_H

#include <heimdall_common.h>
#include <heimdall_codec.h>
#include <heimdall_opengl.h>
#include <heimdall_opengl_filter.h>
#include <heimdall_plugin.h>

#include "gl_api.h"
#include "gl_common.h"
#include "interop.h"
#include "sampler.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define add_opengl_submodule_renderer() \
    add_submodule() \
    add_shortcut("renderer") \
    set_shortname("renderer") \
    set_capability("opengl filter", 0) \
    set_callback_opengl_filter(heimdall_gl_renderer_Open)

/**
 * OpenGL picture renderer
 */
struct heimdall_gl_renderer
{
    struct heimdall_gl_api api;
    const opengl_vtable_t *vt; /* for convenience, same as &api.vt */

    /* True to dump shaders */
    bool dump_shaders;

    GLuint program_id;

    struct {
        GLfloat ProjectionMatrix[16];
        GLfloat StereoMatrix[3*3];
        GLfloat ZoomMatrix[16];
        GLfloat ViewMatrix[16];
    } var;

    struct {
        GLint StereoMatrix;
        GLint ProjectionMatrix;
        GLint ViewMatrix;
        GLint ZoomMatrix;
        GLint OrientationMatrix;
    } uloc;

    struct {
        GLint PicCoordsIn;
        GLint VertexPosition;
    } aloc;

    struct heimdall_gl_sampler *sampler;

    unsigned nb_indices;
    GLuint vertex_buffer_object;
    GLuint index_buffer_object;
    GLuint texture_buffer_object;

    GLuint vertex_array_object;

    bool valid_coords;

    /* View point */
    heimdall_viewpoint_t vp;
    float f_teta;
    float f_phi;
    float f_roll;
    float f_fovx; /* f_fovx and f_fovy are linked but we keep both */
    float f_fovy; /* to avoid recalculating them when needed.      */
    float f_z;    /* Position of the camera on the sphere radius vector */
    float f_sar;

    /* Original size from which f_sar is computed
     *     f_sar = (float) target_width / target_height */
    unsigned target_width;
    unsigned target_height;

    /* Projection mode for the input data */
    video_projection_mode_t projection_mode;
    heimdall_stereoscopic_mode_t stereo_mode;
    video_multiview_mode_t multiview_mode;
};

heimdall_gl_filter_open_fn heimdall_gl_renderer_Open;

int
heimdall_gl_renderer_SetViewpoint(struct heimdall_gl_renderer *renderer,
                             const heimdall_viewpoint_t *p_vp);

#ifdef __cplusplus
}
#endif

#endif /* include-guard */
