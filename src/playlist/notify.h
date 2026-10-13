/*****************************************************************************
 * playlist/notify.h
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
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

#ifndef HEIMDALL_PLAYLIST_NOTIFY_H
#define HEIMDALL_PLAYLIST_NOTIFY_H

#include <heimdall_common.h>
#include <heimdall_list.h>

typedef struct heimdall_playlist heimdall_playlist_t;

struct heimdall_playlist_listener_id
{
    const struct heimdall_playlist_callbacks *cbs;
    void *userdata;
    struct heimdall_list node; /**< node of heimdall_playlist.listeners */
};

struct heimdall_playlist_state {
    ssize_t current;
    bool has_prev;
    bool has_next;
};

#define heimdall_playlist_listener_foreach(listener, playlist) \
    heimdall_list_foreach(listener, &(playlist)->listeners, node)

#define heimdall_playlist_NotifyListener(playlist, listener, event, ...) \
do { \
    if (listener->cbs->event) \
        listener->cbs->event(playlist, ##__VA_ARGS__, listener->userdata); \
} while (0)

#define heimdall_playlist_Notify(playlist, event, ...) \
do { \
    heimdall_playlist_AssertLocked(playlist); \
    heimdall_playlist_listener_id *listener; \
    heimdall_playlist_listener_foreach(listener, playlist) \
        heimdall_playlist_NotifyListener(playlist, listener, event, ##__VA_ARGS__); \
} while(0)

void
heimdall_playlist_state_Save(heimdall_playlist_t *playlist,
                        struct heimdall_playlist_state *state);

void
heimdall_playlist_state_NotifyChanges(heimdall_playlist_t *playlist,
                                 struct heimdall_playlist_state *saved_state);

void
heimdall_playlist_NotifyMediaUpdated(heimdall_playlist_t *playlist, input_item_t *media);

#endif
