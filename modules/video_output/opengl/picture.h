/*****************************************************************************
 * picture.h
 *****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
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

#ifndef HEIMDALL_GL_PICTURE_PRIV_H
#define HEIMDALL_GL_PICTURE_PRIV_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_es.h>
#include <heimdall_picture.h>
#include <heimdall_opengl_picture.h>
#include "gl_common.h"

/**
 * Format of an OpenGL picture
 */
struct heimdall_gl_format {
    video_format_t fmt;

    GLenum tex_target;

    unsigned tex_count;
    GLsizei tex_widths[PICTURE_PLANE_MAX];
    GLsizei tex_heights[PICTURE_PLANE_MAX];

    uint32_t formats[PICTURE_PLANE_MAX];
    bool half_float;
};

#endif
