/**
 * @file window.c
 * @brief Android native window provider module for Heimdall media player
 */
/*****************************************************************************
 * Copyright © 2013 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Adrien Maglo <magsoft@videolan.org>
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
# include <config.h>
#endif

#include <stdarg.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_window.h>
#include <heimdall_codec.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_media_player.h>

#include <dlfcn.h>
#include <jni.h>

#include "utils.h"
#include "../wasync_resize_compressor.h"

typedef struct
{
    heimdall_wasync_resize_compressor_t compressor;
} vout_window_sys_t;

struct vout_window_sys_cb
{
    void *opaque;
    libheimdall_video_output_cleanup_cb cleanup_cb;
};

static void OnNewWindowSize(heimdall_window_t *wnd,
                            unsigned i_width, unsigned i_height)
{
    vout_window_sys_t *sys = (vout_window_sys_t *) wnd->sys;
    heimdall_wasync_resize_compressor_reportSize(&sys->compressor, i_width, i_height);
}

static void OnNewMouseCoords(heimdall_window_t *wnd,
                             const struct awh_mouse_coords *coords)
{
    heimdall_window_ReportMouseMoved(wnd, coords->i_x, coords->i_y);
    switch (coords->i_action)
    {
        case AMOTION_EVENT_ACTION_DOWN:
            heimdall_window_ReportMousePressed(wnd, coords->i_button);
            break;
        case AMOTION_EVENT_ACTION_UP:
            heimdall_window_ReportMouseReleased(wnd, coords->i_button);
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            break;
    }
}

static void Close(heimdall_window_t *wnd)
{
    vout_window_sys_t *sys = (vout_window_sys_t *) wnd->sys;
    heimdall_wasync_resize_compressor_destroy(&sys->compressor);
    AWindowHandler_destroy(wnd->display.anativewindow);
}

static int Open(heimdall_window_t *wnd)
{
    static const struct heimdall_window_operations ops = {
        .destroy = Close,
    };

    /* We cannot create a window without the associated AWindow instance. */
    jobject jobj = var_InheritAddress(wnd, "drawable-androidwindow");
    if (jobj == NULL)
        return HEIMDALL_EGENERIC;

    vout_window_sys_t *sys = heimdall_obj_malloc(HEIMDALL_OBJECT(wnd), sizeof (*sys));
    if (sys == NULL)
        return HEIMDALL_ENOMEM;

    if (heimdall_wasync_resize_compressor_init(&sys->compressor, wnd))
        return HEIMDALL_EGENERIC;

    wnd->sys = sys;

    AWindowHandler *p_awh = AWindowHandler_new(HEIMDALL_OBJECT(wnd), wnd,
        &(awh_events_t) { OnNewWindowSize, OnNewMouseCoords });
    if (p_awh == NULL)
        return HEIMDALL_EGENERIC;

    wnd->type = HEIMDALL_WINDOW_TYPE_ANDROID_NATIVE;
    wnd->display.anativewindow = p_awh;
    wnd->handle.android_id = AWindow_Video;
    wnd->ops = &ops;

    return HEIMDALL_SUCCESS;
}

static void DestroyCallback(heimdall_window_t *wnd)
{
    struct vout_window_sys_cb *sys = wnd->sys;
    if (sys->cleanup_cb != NULL)
        sys->cleanup_cb(sys->opaque);
}

static int OpenCallback(heimdall_window_t *wnd)
{
    static const struct heimdall_window_operations ops = {
        .destroy = DestroyCallback,
    };

    libheimdall_video_engine_t engine = var_InheritInteger(wnd, "vout-cb-type");
    if (engine != libheimdall_video_engine_anw)
        return HEIMDALL_EGENERIC;

    struct vout_window_sys_cb *sys = heimdall_obj_malloc(HEIMDALL_OBJECT(wnd), sizeof (*sys));
    if (sys == NULL)
        return HEIMDALL_ENOMEM;

    sys->opaque = var_InheritAddress(wnd, "vout-cb-opaque");
    libheimdall_video_output_setup_cb setup_cb = var_InheritAddress(wnd, "vout-cb-setup");
    libheimdall_video_update_output_cb update_cb = var_InheritAddress(wnd, "vout-cb-update-output");
    sys->cleanup_cb = var_InheritAddress(wnd, "vout-cb-cleanup");

    if (update_cb == NULL)
        return HEIMDALL_EGENERIC;

    if (setup_cb != NULL)
    {
        const libheimdall_video_setup_device_cfg_t cfg = {
            .hardware_decoding = true,
        };
        libheimdall_video_setup_device_info_t out;
        bool success = setup_cb(&sys->opaque, &cfg, &out);
        if (!success)
            return HEIMDALL_EGENERIC;
        (void) out; /* Ignored on Android */
    }

    /* Default values, parameters are overridden by the producer */
    const libheimdall_video_render_cfg_t render_cfg = {
        .width = 1,
        .height = 1,
    };
    libheimdall_video_output_cfg_t out = { .u.anw = { NULL, NULL } };
    bool success = update_cb(sys->opaque, &render_cfg, &out);
    if (!success)
        goto error;
    assert(out.u.anw.video != NULL);

    AWindowHandler *awh =
        AWindowHandler_newFromANWs(HEIMDALL_OBJECT(wnd), out.u.anw.video, out.u.anw.subtitle);
    if (awh == NULL)
        goto error;

    wnd->sys = sys;
    wnd->type = HEIMDALL_WINDOW_TYPE_ANDROID_NATIVE;
    wnd->display.anativewindow = awh;
    wnd->handle.android_id = AWindow_Video;
    wnd->ops = &ops;

    return HEIMDALL_SUCCESS;
error:
    if (sys->cleanup_cb != NULL)
        sys->cleanup_cb(sys->opaque);
    return HEIMDALL_EGENERIC;
}

static int
OpenDecDevice(heimdall_decoder_device *device, heimdall_window_t *window)
{
    AWindowHandler *awh;
    if (window && window->type == HEIMDALL_WINDOW_TYPE_ANDROID_NATIVE)
    {
        awh = window->display.anativewindow;
    }
    else
    {
        awh = AWindowHandler_new(HEIMDALL_OBJECT(device), NULL, NULL);
        if (awh == NULL)
            return HEIMDALL_ENOMEM;
    }

    static const struct heimdall_decoder_device_operations ops =
    {
        .close = NULL,
    };
    device->ops = &ops;
    device->type = HEIMDALL_DECODER_DEVICE_AWINDOW;
    device->opaque = awh;

    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_shortname(N_("Android Window"))
    set_description(N_("Android native window"))
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_capability("vout window", 10)
    set_callback(Open)
    add_submodule ()
        set_capability("vout window", 11)
        set_callback(OpenCallback)
    add_submodule ()
        set_callback_dec_device(OpenDecDevice, 1)
        add_shortcut("android")
heimdall_module_end()
