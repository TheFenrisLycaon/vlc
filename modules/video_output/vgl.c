/*****************************************************************************
 * vgl.c: virtual LibHeimdall OpenGL extension
 *****************************************************************************
 * Copyright (c) 2018 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Pierre Lamot <pierre@videolabs.io>
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
#include <heimdall_vout_display.h>
#include <heimdall_opengl.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>
#include <heimdall/libheimdall_renderer_discoverer.h>
#include <heimdall/libheimdall_media_player.h>

#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif

typedef struct vout_display_sys_t
{
    libheimdall_video_output_cleanup_cb cleanupCb;
    libheimdall_video_output_setup_cb setupCb;
    libheimdall_video_update_output_cb resizeCb;
    libheimdall_video_swap_cb swapCb;
    libheimdall_video_makeCurrent_cb makeCurrentCb;
    libheimdall_video_getProcAddress_cb getProcAddressCb;

    void* opaque;
    unsigned width;
    unsigned height;
} vout_display_sys_t;


static void *OurGetProcAddress(heimdall_gl_t *gl, const char *name)
{
    vout_display_sys_t *sys = gl->sys;
    return sys->getProcAddressCb(sys->opaque, name);
}

static int MakeCurrent(heimdall_gl_t *gl)
{
    vout_display_sys_t *sys = gl->sys;
    bool success = sys->makeCurrentCb(sys->opaque, true);
    return success ? HEIMDALL_SUCCESS : HEIMDALL_EGENERIC;
}

static void ReleaseCurrent(heimdall_gl_t *gl)
{
    vout_display_sys_t *sys = gl->sys;
    sys->makeCurrentCb(sys->opaque, false);
}

static void VglSwapBuffers(heimdall_gl_t *gl)
{
    vout_display_sys_t *sys = gl->sys;
    MakeCurrent(gl);
    sys->swapCb(sys->opaque);
    ReleaseCurrent(gl);
}

static int ResizeInternal(heimdall_gl_t * gl, unsigned w, unsigned h)
{
    vout_display_sys_t *sys = gl->sys;
    if( sys->width == w && sys->height == h )
        return HEIMDALL_SUCCESS;

    MakeCurrent(gl);
    libheimdall_video_render_cfg_t output_cfg = {
        w, h, 8, true,
        libheimdall_video_colorspace_BT709, libheimdall_video_primaries_BT709,
        libheimdall_video_transfer_func_SRGB, NULL,
    };
    libheimdall_video_output_cfg_t render_cfg = {
        .u.opengl_format = GL_RGBA,
        .full_range = true,
        .colorspace = libheimdall_video_colorspace_BT709,
        .primaries = libheimdall_video_primaries_BT709,
        .transfer = libheimdall_video_transfer_func_SRGB,
        .orientation = libheimdall_video_orient_top_left,
    };
    bool ret = sys->resizeCb(sys->opaque, &output_cfg, &render_cfg);
    ReleaseCurrent(gl);

    if (!ret)
        return HEIMDALL_EGENERIC;

    assert(render_cfg.u.opengl_format == GL_RGBA);
    assert(render_cfg.full_range == true);
    assert(render_cfg.colorspace == libheimdall_video_colorspace_BT709);
    assert(render_cfg.primaries  == libheimdall_video_primaries_BT709);
    assert(render_cfg.transfer   == libheimdall_video_transfer_func_SRGB);

    /* video_orientation_t enum is matching libheimdall_video_orient_t. */
    gl->orientation = (video_orientation_t)render_cfg.orientation;

    sys->width = w;
    sys->height = h;

    return HEIMDALL_SUCCESS;
}

static void Resize(heimdall_gl_t * gl, unsigned w, unsigned h)
{
    ResizeInternal(gl, w, h);
}

static void Close(heimdall_gl_t *gl)
{
    vout_display_sys_t *sys = gl->sys;
    if( sys->cleanupCb )
        sys->cleanupCb(sys->opaque);
}


#define SET_CALLBACK_ADDR(var, varname) \
    do {                                                           \
        var = var_InheritAddress(gl, varname);                     \
        if( !var ) {                                               \
            msg_Err( gl, "%s address is missing", varname );       \
            return HEIMDALL_EGENERIC;                                   \
        }                                                          \
    } while( 0 )

static int Open(heimdall_gl_t *gl, unsigned width, unsigned height,
                const struct heimdall_gl_cfg *gl_cfg)
{
    vout_display_sys_t * sys;

    libheimdall_video_engine_t engineType = var_InheritInteger( gl, "vout-cb-type" );
    switch (engineType)
    {
        case libheimdall_video_engine_opengl:
            if (gl->api_type != HEIMDALL_OPENGL)
                return HEIMDALL_ENOTSUP;
            break;

        case libheimdall_video_engine_gles2:
            if (gl->api_type != HEIMDALL_OPENGL_ES2)
                return HEIMDALL_ENOTSUP;
            break;

        default:
            return HEIMDALL_ENOTSUP;
    }

    if (gl_cfg->need_alpha)
    {
        msg_Err(gl, "Cannot support alpha yet");
        return HEIMDALL_ENOTSUP;
    }

    /* Allocate structure */
    gl->sys = sys = heimdall_obj_calloc(HEIMDALL_OBJECT(gl), 1, sizeof(*sys));
    if( !sys )
        return HEIMDALL_ENOMEM;

    sys->opaque = var_InheritAddress(gl, "vout-cb-opaque");
    sys->setupCb = var_InheritAddress(gl, "vout-cb-setup");
    sys->cleanupCb = var_InheritAddress(gl, "vout-cb-cleanup");
    SET_CALLBACK_ADDR(sys->resizeCb, "vout-cb-update-output");
    SET_CALLBACK_ADDR(sys->swapCb, "vout-cb-swap");
    SET_CALLBACK_ADDR(sys->makeCurrentCb, "vout-cb-make-current");
    SET_CALLBACK_ADDR(sys->getProcAddressCb, "vout-cb-get-proc-address");

    if( sys->setupCb )
    {
        libheimdall_video_setup_device_cfg_t setup_cfg = { .hardware_decoding = false };
        libheimdall_video_setup_device_info_t configured_cfg;
        if( !sys->setupCb(&sys->opaque, &setup_cfg, &configured_cfg) )
        {
            msg_Err( gl, "user setup failed" );
            return HEIMDALL_EGENERIC;
        }
    }
    if (ResizeInternal(gl, width, height) != HEIMDALL_SUCCESS)
    {
        if( sys->cleanupCb )
            sys->cleanupCb(sys->opaque);
        return HEIMDALL_EGENERIC;
    }

    static const struct heimdall_gl_operations gl_ops =
    {
        .make_current = MakeCurrent,
        .release_current = ReleaseCurrent,
        .resize = Resize,
        .swap = VglSwapBuffers,
        .get_proc_address = OurGetProcAddress,
        .close = Close,
    };
    gl->ops = &gl_ops;

    return HEIMDALL_SUCCESS;
}

#undef SET_CALLBACK_ADDR

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

heimdall_module_begin()
    set_shortname("GL texture")
    set_description("GL texture output")
    set_subcategory(SUBCAT_VIDEO_VOUT)

    set_callback_opengl(Open, 0)

    add_shortcut("vglmem")

    add_submodule()
    set_callback_opengl_es2(Open, 0)
    add_shortcut("vglmem")
heimdall_module_end()
