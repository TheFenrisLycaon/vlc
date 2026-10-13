/*****************************************************************************
 * egl_display.h
 *****************************************************************************
 * Copyright (C) 2021 Videolabs
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

#ifndef HEIMDALL_EGL_REFDISPLAY_H
#define HEIMDALL_EGL_REFDISPLAY_H

#include <heimdall_common.h>
#include <EGL/egl.h>

/**
 * Structure for module "egl display", to open an EGL display guaranteed to
 * be internally refcounted.
 */
struct heimdall_egl_display
{
    struct heimdall_object_t obj;

    EGLDisplay display;

    const struct heimdall_egl_display_ops *ops;
    void *sys;
};

struct heimdall_egl_display_ops {
    void (*close)(struct heimdall_egl_display *display);
};

typedef int
heimdall_egl_display_open_fn(struct heimdall_egl_display *display);

struct heimdall_egl_display *
heimdall_egl_display_New(heimdall_object_t *parent, const char *name);

void heimdall_egl_display_Delete(struct heimdall_egl_display *display);

#endif
