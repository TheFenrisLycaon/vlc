/*****************************************************************************
 * gl_api.h
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

#ifndef HEIMDALL_GL_API_H
#define HEIMDALL_GL_API_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdbool.h>
#include <heimdall_common.h>
#include <heimdall_opengl.h>

#include "gl_common.h"

struct heimdall_gl_api {
    opengl_vtable_t vt;

    /* Highest GLSL version supported by the API */
    int glsl_version;

    /* True if the current API is OpenGL ES, set by the caller */
    bool is_gles;

    /* Multisampling for anti-aliasing */
    bool supports_multisample;

    /* sampler3D feature and syntax support */
    bool supports_sampler3D;
};

int
heimdall_gl_api_Init(struct heimdall_gl_api *api, heimdall_gl_t *gl);

#endif
