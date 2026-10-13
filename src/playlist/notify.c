/*****************************************************************************
 * playlist/notify.c
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "notify.h"

#include "item.h"
#include "playlist.h"

static void
heimdall_playlist_NotifyCurrentState(heimdall_playlist_t *playlist,
                                heimdall_playlist_listener_id *listener)
{
    heimdall_playlist_NotifyListener(playlist, listener, on_items_reset,
                                playlist->items.data, playlist->items.size);
    heimdall_playlist_NotifyListener(playlist, listener, on_playback_repeat_changed,
                                playlist->repeat);
    heimdall_playlist_NotifyListener(playlist, listener, on_playback_order_changed,
                                playlist->order);
    heimdall_playlist_NotifyListener(playlist, listener, on_current_index_changed,
                                playlist->current);
    heimdall_playlist_NotifyListener(playlist, listener, on_has_prev_changed,
                                playlist->has_prev);
    heimdall_playlist_NotifyListener(playlist, listener, on_has_next_changed,
                                playlist->has_next);
    heimdall_playlist_NotifyListener(playlist, listener, on_media_stopped_action_changed,
                                playlist->stopped_action);
}

heimdall_playlist_listener_id *
heimdall_playlist_AddListener(heimdall_playlist_t *playlist,
                         const struct heimdall_playlist_callbacks *cbs,
                         void *userdata, bool notify_current_state)
{
    heimdall_playlist_AssertLocked(playlist);

    heimdall_playlist_listener_id *listener = malloc(sizeof(*listener));
    if (unlikely(!listener))
        return NULL;

    listener->cbs = cbs;
    listener->userdata = userdata;
    heimdall_list_append(&listener->node, &playlist->listeners);

    if (notify_current_state)
        heimdall_playlist_NotifyCurrentState(playlist, listener);

    return listener;
}

void
heimdall_playlist_RemoveListener(heimdall_playlist_t *playlist,
                            heimdall_playlist_listener_id *listener)
{
    /* The playlist head is not needed to remove a node, but the list must be
     * locked. */
    heimdall_playlist_AssertLocked(playlist); HEIMDALL_UNUSED(playlist);

    heimdall_list_remove(&listener->node);
    free(listener);
}

void
heimdall_playlist_state_Save(heimdall_playlist_t *playlist,
                        struct heimdall_playlist_state *state)
{
    state->current = playlist->current;
    state->has_prev = playlist->has_prev;
    state->has_next = playlist->has_next;
}

void
heimdall_playlist_state_NotifyChanges(heimdall_playlist_t *playlist,
                                 struct heimdall_playlist_state *saved_state)
{
    if (saved_state->current != playlist->current)
        heimdall_playlist_Notify(playlist, on_current_index_changed, playlist->current);
    if (saved_state->has_prev != playlist->has_prev)
        heimdall_playlist_Notify(playlist, on_has_prev_changed, playlist->has_prev);
    if (saved_state->has_next != playlist->has_next)
        heimdall_playlist_Notify(playlist, on_has_next_changed, playlist->has_next);
}

static inline bool
heimdall_playlist_HasItemUpdatedListeners(heimdall_playlist_t *playlist)
{
    heimdall_playlist_listener_id *listener;
    heimdall_playlist_listener_foreach(listener, playlist)
        if (listener->cbs->on_items_updated)
            return true;
    return false;
}

void
heimdall_playlist_NotifyMediaUpdated(heimdall_playlist_t *playlist, input_item_t *media)
{
    heimdall_playlist_AssertLocked(playlist);
    if (!heimdall_playlist_HasItemUpdatedListeners(playlist))
        /* no need to find the index if there are no listeners */
        return;

    ssize_t index;
    if (playlist->current != -1 &&
            playlist->items.data[playlist->current]->media == media)
        /* the player typically sends events for the current item, so we can
         * often avoid to search */
        index = playlist->current;
    else
    {
        /* linear search */
        index = heimdall_playlist_IndexOfMedia(playlist, media);
        if (index == -1)
            return;
    }
    heimdall_playlist_Notify(playlist, on_items_updated, index,
                        &playlist->items.data[index], 1);
}
