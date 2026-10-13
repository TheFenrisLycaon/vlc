/*****************************************************************************
 * heimdall_pip_controller.h : picture in picture controller module api
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Maxime Chapelet <umxprime at videolabs dot io>
 *
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

#ifndef HEIMDALL_PIP_CONTROLLER_H
#define HEIMDALL_PIP_CONTROLLER_H 1

#include <heimdall_common.h>
#include <heimdall_tick.h>
#include <heimdall_player.h>
#include <CoreFoundation/CoreFoundation.h>

typedef struct pip_controller_t pip_controller_t;

struct pip_controller_operations {
    void (*set_display_layer)(pip_controller_t *, void *);
    int (*close)(pip_controller_t *);
};

struct pip_controller_media_callbacks {
    void (*play)(void* opaque);
    void (*pause)(void* opaque);
    void (*seek_by)(heimdall_tick_t time, dispatch_block_t completion, void *opaque);
    heimdall_tick_t (*media_length)(void* opaque);
    heimdall_tick_t (*media_time)(void* opaque);
    bool (*is_media_seekable)(void* opaque);
    bool (*is_media_playing)(void* opaque);
};

struct pip_controller_t
{
    struct heimdall_object_t obj;

    void               *p_sys;

    const struct pip_controller_operations *ops;
    const struct pip_controller_media_callbacks *media_cbs;
};

#endif // HEIMDALL_PIP_CONTROLLER_H