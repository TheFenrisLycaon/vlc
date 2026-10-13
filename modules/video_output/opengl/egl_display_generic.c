/*****************************************************************************
 * egl_display_generic.c
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_opengl.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "egl_display.h"

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
static EGLenum GetPlatform(const char *extensions)
{
#ifdef EGL_KHR_platform_x11
    if (heimdall_gl_StrHasToken(extensions, "EGL_EXT_platform_x11"))
        return EGL_PLATFORM_X11_KHR;
#endif

#ifdef EGL_KHR_platform_wayland
    if (heimdall_gl_StrHasToken(extensions, "EGL_EXT_platform_wayland"))
        return EGL_PLATFORM_WAYLAND_KHR;
#endif

    return 0;
}
#endif

static heimdall_egl_display_open_fn Open;
static int
Open(struct heimdall_egl_display *display)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    /* The default display is refcounted on Android and Emscripten */
    display->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#elif defined(EGL_KHR_display_reference)
    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

    if (!heimdall_gl_StrHasToken(extensions, "EGL_KHR_display_reference"))
        return HEIMDALL_EGENERIC;

    EGLenum platform = GetPlatform(extensions);
    if (!platform)
        return HEIMDALL_EGENERIC;

    const EGLAttrib attribs[] = {
        EGL_TRACK_REFERENCES_KHR, EGL_TRUE,
        EGL_NONE,
    };

    display->display =
        eglGetPlatformDisplay(platform, EGL_DEFAULT_DISPLAY, attribs);
#endif

    if (display->display == EGL_NO_DISPLAY)
        return HEIMDALL_EGENERIC;

    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_description("EGL generic display")
    set_capability("egl display", 1)
    set_callback(Open)
    add_shortcut("egl_display_generic")
heimdall_module_end()
