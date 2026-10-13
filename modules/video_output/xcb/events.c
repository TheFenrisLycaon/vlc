/**
 * @file events.c
 * @brief X C Bindings Heimdall video output events handling
 */
/*****************************************************************************
 * Copyright © 2009 Rémi Denis-Courmont
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

#include <inttypes.h>
#include <assert.h>

#include <xcb/xcb.h>

#include <heimdall_common.h>
#include <heimdall_vout_display.h>

#include "events.h"

#ifndef HAVE_PARTIAL_LINKING
const char heimdall_module_name[] = "xcb";
#endif

#pragma GCC visibility push(default)

int heimdall_xcb_error_Check(struct heimdall_logger *log, xcb_connection_t *conn,
                        const char *str, xcb_void_cookie_t ck)
{
    xcb_generic_error_t *err;

    err = xcb_request_check (conn, ck);
    if (err)
    {
        int code = err->error_code;

        free (err);
        heimdall_error(log, "%s: X11 error %d", str, code);
        assert (code != 0);
        return code;
    }
    return 0;
}

/**
 * Connect to the X server.
 */
static xcb_connection_t *Connect(struct heimdall_logger *log, const char *display)
{
    xcb_connection_t *conn = xcb_connect (display, NULL);
    if (xcb_connection_has_error (conn) /*== NULL*/)
    {
        heimdall_error(log, "cannot connect to X server (%s)",
                  (display != NULL) ? display : "default");
        xcb_disconnect (conn);
        return NULL;
    }

    const xcb_setup_t *setup = xcb_get_setup (conn);
    heimdall_debug(log, "connected to X%"PRIu16".%"PRIu16" server",
              setup->protocol_major_version, setup->protocol_minor_version);
    heimdall_debug(log, " vendor : %.*s", (int)setup->vendor_len,
              xcb_setup_vendor(setup));
    heimdall_debug(log, " version: %"PRIu32, setup->release_number);
    return conn;
}

/**
 * Find screen matching a given root window.
 */
static const xcb_screen_t *FindScreen(struct heimdall_logger *log,
                                      xcb_connection_t *conn,
                                      xcb_window_t root)
{
    /* Find the selected screen */
    const xcb_setup_t *setup = xcb_get_setup (conn);
    for (xcb_screen_iterator_t i = xcb_setup_roots_iterator (setup);
         i.rem > 0; xcb_screen_next (&i))
    {
        if (i.data->root == root)
        {
            heimdall_debug(log, "using screen 0x%"PRIx32, root);
            return i.data;
        }
    }
    heimdall_error(log, "window screen not found");
    return NULL;
}

int heimdall_xcb_parent_Create(struct heimdall_logger *log, const heimdall_window_t *wnd,
                          xcb_connection_t **restrict pconn,
                          const xcb_screen_t **restrict pscreen)
{
    if (wnd->type != HEIMDALL_WINDOW_TYPE_XID)
    {
        heimdall_error(log, "window not available");
        return HEIMDALL_ENOTSUP;
    }

    xcb_connection_t *conn = Connect(log, wnd->display.x11);
    if (conn == NULL)
        goto error;
    *pconn = conn;

    xcb_get_geometry_reply_t *geo =
        xcb_get_geometry_reply (conn, xcb_get_geometry (conn, wnd->handle.xid),
                                NULL);
    if (geo == NULL)
    {
        heimdall_error(log, "window not valid");
        goto error;
    }

    const xcb_screen_t *screen = FindScreen(log, conn, geo->root);
    free (geo);
    if (screen == NULL)
        goto error;
    *pscreen = screen;
    return HEIMDALL_SUCCESS;

error:
    if (conn != NULL)
        xcb_disconnect (conn);
    return HEIMDALL_EGENERIC;
}

/**
 * Process an X11 event.
 */
static int ProcessEvent(struct heimdall_logger *log, xcb_generic_event_t *ev)
{
    switch (ev->response_type & 0x7f)
    {
        case XCB_MAPPING_NOTIFY:
            break;

        default:
            heimdall_debug(log, "unhandled event %"PRIu8, ev->response_type);
    }

    free (ev);
    return HEIMDALL_SUCCESS;
}

int heimdall_xcb_Manage(struct heimdall_logger *log, xcb_connection_t *conn)
{
    xcb_generic_event_t *ev;

    while ((ev = xcb_poll_for_event (conn)) != NULL)
        ProcessEvent(log, ev);

    if (xcb_connection_has_error (conn))
    {
        heimdall_error(log, "X server failure");
        return HEIMDALL_EGENERIC;
    }

    return HEIMDALL_SUCCESS;
}
