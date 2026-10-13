/*****************************************************************************
 * vout_helper.h: OpenGL vout_display helpers
 *****************************************************************************
 * Copyright (C) 2004-2016 VLC authors and VideoLAN
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir _AT_ lycaria _DOT_ org>
 *          Rémi Denis-Courmont
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *          Ilkka Ollakka <ileoo@videolan.org>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
 *          Rafaël Carré <funman@lycariaorg>
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

#ifndef HEIMDALL_OPENGL_VOUT_HELPER_H
#define HEIMDALL_OPENGL_VOUT_HELPER_H

#include "gl_common.h"
#include "gl_scale.h"

#define GLINTEROP_TEXT N_("Open GL/GLES hardware interop")
#define GLINTEROP_LONGTEXT N_( \
    "Force a \"glinterop\" module.")

#define GLSAMPLER_TEXT N_("Open GL/GLES sampler")
#define GLSAMPLER_LONGTEXT N_( \
    "Force an \"opengl sampler\" module.")

#define add_glopts() \
    add_module("glinterop", "glinterop", "any", GLINTEROP_TEXT, GLINTEROP_LONGTEXT) \
    add_module("gl-sampler", "opengl sampler", "any", GLSAMPLER_TEXT, GLSAMPLER_LONGTEXT) \
    add_glscale_opts()

typedef struct vout_display_opengl_t vout_display_opengl_t;

vout_display_opengl_t *vout_display_opengl_New(video_format_t *fmt,
                                               const heimdall_fourcc_t **subpicture_chromas,
                                               heimdall_gl_t *gl,
                                               const heimdall_viewpoint_t *viewpoint,
                                               heimdall_video_context *context);
void vout_display_opengl_Delete(vout_display_opengl_t *vgl);

int vout_display_opengl_SetViewpoint(vout_display_opengl_t *vgl, const heimdall_viewpoint_t*);

void vout_display_opengl_SetOutputSize(vout_display_opengl_t *vgl,
                                       unsigned width, unsigned height);

void vout_display_opengl_Viewport(vout_display_opengl_t *vgl, int x, int y,
                                  unsigned width, unsigned height);

int vout_display_opengl_Prepare(vout_display_opengl_t *vgl,
                                picture_t *picture,
                                const struct heimdall_render_subpicture *subpicture);
int vout_display_opengl_Display(vout_display_opengl_t *vgl);

int vout_display_opengl_UpdateFormat(vout_display_opengl_t *vgl,
                                     const video_format_t *fmt,
                                     heimdall_video_context *vctx);

int vout_display_opengl_ChangeProjection(vout_display_opengl_t *vgl,
                                         video_projection_mode_t projection);

int vout_display_opengl_ChangeStereoMode(vout_display_opengl_t *vgl,
                                         heimdall_stereoscopic_mode_t projection);

#endif
