/**
 * @file wextern.c
 * @brief Dummy video window provider where the size is handled externally
 */
/*****************************************************************************
 * Copyright © 2019 VideoLabs, VideoLAN and VideoLAN Authors
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
# include <config.h>
#endif

#include <stdarg.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_window.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>
#include <heimdall/libheimdall_renderer_discoverer.h>
#include <heimdall/libheimdall_media_player.h>

static int Open(heimdall_window_t *);

heimdall_module_begin()
    set_shortname(N_("Callback window"))
    set_description(N_("External callback window"))
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_capability("vout window", 0)
    set_callback(Open)
heimdall_module_end()

typedef struct {
    void                                   *opaque;
    libheimdall_video_output_set_window_cb       setWindowCb;
} wextern_t;

static void WindowResize(void *opaque, unsigned width, unsigned height)
{
    heimdall_window_t *window = opaque;
    heimdall_window_ReportSize(window, width, height);
}

static void WindowMouseMoved(void *opaque, int x, int y)
{
    heimdall_window_t *window = opaque;
    heimdall_window_ReportMouseMoved(window, x, y);
}

static void WindowMousePress(void *opaque, libheimdall_video_output_mouse_button_t button)
{
    heimdall_window_t *window = opaque;
    heimdall_window_ReportMousePressed(window, button);
}

static void WindowMouseRelease(void *opaque, libheimdall_video_output_mouse_button_t button)
{
    heimdall_window_t *window = opaque;
    heimdall_window_ReportMouseReleased(window, button);
}

static int Enable(struct heimdall_window *wnd, const heimdall_window_cfg_t *wcfg)
{
    wextern_t *sys = wnd->sys;

    if ( sys->setWindowCb != NULL )
        /* bypass the size handling as the window doesn't handle the size */
        sys->setWindowCb( sys->opaque,
            WindowResize,
            WindowMouseMoved, WindowMousePress, WindowMouseRelease,
            wnd );

    (void) wcfg;
    return HEIMDALL_SUCCESS;
}

static void Disable(struct heimdall_window *wnd)
{
    wextern_t *sys = wnd->sys;

    if ( sys->setWindowCb != NULL )
        sys->setWindowCb( sys->opaque, NULL, NULL, NULL, NULL, NULL );
}

static const struct heimdall_window_operations ops = {
    .enable  = Enable,
    .disable = Disable,
    // .resize: don't let the core resize us on zoom/crop/ar changes
    //          the display module should do the ReportSize for us
};

static int Open(heimdall_window_t *wnd)
{
    wextern_t *sys = heimdall_obj_malloc(HEIMDALL_OBJECT(wnd), sizeof(*sys));
    if (unlikely(sys==NULL))
        return HEIMDALL_ENOMEM;

    sys->opaque          = var_InheritAddress( wnd, "vout-cb-opaque" );
    sys->setWindowCb     = var_InheritAddress( wnd, "vout-cb-window-cb" );

    wnd->sys = sys;
    wnd->type = HEIMDALL_WINDOW_TYPE_DUMMY;
    wnd->ops = &ops;
    wnd->info.has_double_click = false;
    return HEIMDALL_SUCCESS;
}
