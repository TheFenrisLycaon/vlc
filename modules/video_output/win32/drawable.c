/**
 * @file drawable.c
 * @brief Legacy monolithic LibHeimdall video window provider
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

#include <stdarg.h>
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_window.h>
#include "../wasync_resize_compressor.h"

#include <windows.h>

#define HWND_TEXT N_("Window handle (HWND)")
#define HWND_LONGTEXT N_( \
    "Video will be embedded in this pre-existing window. " \
    "If zero, a new window will be created.")

static int Open(heimdall_window_t *);
static void Close(heimdall_window_t *);

/*
 * Module descriptor
 */
heimdall_module_begin ()
    set_shortname (N_("Drawable"))
    set_description (N_("Embedded window video"))
    set_subcategory (SUBCAT_VIDEO_VOUT)
    set_capability ("vout window", 70)
    set_callback(Open)
    add_shortcut ("embed-hwnd")

    add_integer ("drawable-hwnd", 0, HWND_TEXT, HWND_LONGTEXT)
        change_volatile ()
heimdall_module_end ()

/* Keep a list of busy drawables, so we don't overlap videos if there are
 * more than one video track in the stream. */
static heimdall_mutex_t serializer = HEIMDALL_STATIC_MUTEX;
static HWND *used = NULL;

static const struct heimdall_window_operations ops = {
    .destroy = Close,
};

#define RECTWidth(r)   (LONG)((r).right - (r).left)
#define RECTHeight(r)  (LONG)((r).bottom - (r).top)

static const TCHAR *EMBED_HWND_CLASS = TEXT("Heimdall embedded HWND");

struct drawable_sys
{
    HWND embed_hwnd;
    RECT rect_parent;

    WNDPROC prev_proc;

    heimdall_wasync_resize_compressor_t compressor;
};

static LRESULT CALLBACK WinVoutEventProc(HWND hwnd, UINT message,
                                         WPARAM wParam, LPARAM lParam )
{
    HANDLE p_user_data = GetProp(hwnd, EMBED_HWND_CLASS);
    struct drawable_sys *sys = (struct drawable_sys *)p_user_data;
    LRESULT res = CallWindowProc(sys->prev_proc, hwnd, message, wParam, lParam);
    switch(message)
    {
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            if (RECTWidth(sys->rect_parent)  != RECTWidth(clientRect) ||
                RECTHeight(sys->rect_parent) != RECTHeight(clientRect))
            {
                sys->rect_parent = clientRect;

                heimdall_wasync_resize_compressor_reportSize(&sys->compressor,
                                                        RECTWidth(sys->rect_parent),
                                                        RECTHeight(sys->rect_parent));
            }
        }
        break;
    }
    return res;
}

static void RemoveDrawable(HWND val)
{
    size_t n = 0;

    /* Remove this drawable from the list of busy ones */
    heimdall_mutex_lock (&serializer);
    assert (used != NULL);
    while (used[n] != val)
    {
        assert (used[n]);
        n++;
    }
    do
        used[n] = used[n + 1];
    while (used[++n] != 0);

    if (n == 1)
    {
        free (used);
        used = NULL;
    }
    heimdall_mutex_unlock (&serializer);
}

/**
 * Find the drawable set by libheimdall application.
 */
static int Open(heimdall_window_t *wnd)
{
    uintptr_t drawable = var_InheritInteger (wnd, "drawable-hwnd");
    if (drawable == 0)
        return HEIMDALL_EGENERIC;
    HWND val = (HWND)drawable;

    HWND *tab;
    size_t n = 0;

    heimdall_mutex_lock (&serializer);
    if (used != NULL)
        for (/*n = 0*/; used[n]; n++)
            if (used[n] == val)
            {
                msg_Warn (wnd, "HWND 0x%p is busy", (void*)val);
                heimdall_mutex_unlock (&serializer);
                return HEIMDALL_EGENERIC;
            }

    tab = realloc (used, sizeof (*used) * (n + 2));
    if (unlikely(tab == NULL)) {
        heimdall_mutex_unlock (&serializer);
        return HEIMDALL_ENOMEM;
    }
    used = tab;
    used[n] = val;
    used[n + 1] = 0;

    heimdall_mutex_unlock (&serializer);

    struct drawable_sys *sys = heimdall_obj_calloc(HEIMDALL_OBJECT(wnd), 1, sizeof(*sys));
    if (unlikely(sys == NULL)) {
        RemoveDrawable(val);
        return HEIMDALL_ENOMEM;
    }

    sys->embed_hwnd = val;

    if (heimdall_wasync_resize_compressor_init(&sys->compressor, wnd))
    {
        msg_Err(wnd, "Failed to init async resize compressor");
        RemoveDrawable(val);
        return HEIMDALL_EGENERIC;
    }

    GetClientRect(val, &sys->rect_parent);
    heimdall_wasync_resize_compressor_reportSize(&sys->compressor,
                                            RECTWidth(sys->rect_parent),
                                            RECTHeight(sys->rect_parent));

    sys->prev_proc = (WNDPROC)(uintptr_t) GetWindowLongPtr(val, GWLP_WNDPROC);
    SetProp(val, EMBED_HWND_CLASS, sys);
    SetWindowLongPtr(val, GWLP_WNDPROC, (LONG_PTR) WinVoutEventProc);

    wnd->type = HEIMDALL_WINDOW_TYPE_HWND;
    wnd->handle.hwnd = (void *)val;
    wnd->ops = &ops;
    wnd->sys = sys;
    return HEIMDALL_SUCCESS;
}

/**
 * Release the drawable.
 */
static void Close (heimdall_window_t *wnd)
{
    struct drawable_sys *sys = wnd->sys;

    // do not use our callback anymore
    SetWindowLongPtr(sys->embed_hwnd, GWLP_WNDPROC, (LONG_PTR) sys->prev_proc);
    RemoveProp(sys->embed_hwnd, EMBED_HWND_CLASS);

    heimdall_wasync_resize_compressor_destroy(&sys->compressor);

    RemoveDrawable(sys->embed_hwnd);
}
