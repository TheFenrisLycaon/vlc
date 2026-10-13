/*****************************************************************************
 * playlist/player.c
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

#include <heimdall_common.h>
#include <heimdall_interface.h>

#include "player.h"

#include "control.h"
#include "item.h"
#include "notify.h"
#include "playlist.h"
#include "preparse.h"

static void
player_on_current_media_changed(heimdall_player_t *player, input_item_t *new_media,
                                void *userdata)
{
    HEIMDALL_UNUSED(player);
    heimdall_playlist_t *playlist = userdata;

    /* the playlist and the player share the lock */
    heimdall_playlist_AssertLocked(playlist);

    input_item_t *media = playlist->current != -1
                        ? playlist->items.data[playlist->current]->media
                        : NULL;
    if (new_media == media)
    {
        heimdall_playlist_UpdateNextMedia(playlist);
        return;
    }

    ssize_t index;
    if (new_media)
    {
        index = heimdall_playlist_IndexOfMedia(playlist, new_media);
        if (index != -1)
        {
            heimdall_playlist_item_t *item = playlist->items.data[index];
            if (playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
                randomizer_Select(&playlist->randomizer, item);
        }
    }
    else
        index = -1;

    struct heimdall_playlist_state state;
    heimdall_playlist_state_Save(playlist, &state);

    playlist->current = index;
    playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
    playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);

    heimdall_playlist_state_NotifyChanges(playlist, &state);

    heimdall_playlist_UpdateNextMedia(playlist);
}

static void
on_player_state_changed(heimdall_player_t *player,
                        enum heimdall_player_state new_state, void *userdata)
{
    heimdall_playlist_t *playlist = userdata;

    if (new_state == HEIMDALL_PLAYER_STATE_STOPPED
     && playlist->stopped_action == HEIMDALL_PLAYLIST_MEDIA_STOPPED_EXIT)
        libheimdall_Quit(heimdall_object_instance(player));
}

static void
on_player_media_meta_changed(heimdall_player_t *player, input_item_t *media,
                             void *userdata)
{
    HEIMDALL_UNUSED(player);
    heimdall_playlist_t *playlist = userdata;

    /* the playlist and the player share the lock */
    heimdall_playlist_AssertLocked(playlist);

    heimdall_playlist_NotifyMediaUpdated(playlist, media);
}

static void
on_player_media_length_changed(heimdall_player_t *player, heimdall_tick_t new_length,
                               void *userdata)
{
    HEIMDALL_UNUSED(player);
    HEIMDALL_UNUSED(new_length);
    heimdall_playlist_t *playlist = userdata;

    /* the playlist and the player share the lock */
    heimdall_playlist_AssertLocked(playlist);

    input_item_t *media = heimdall_player_GetCurrentMedia(player);
    assert(media);

    heimdall_playlist_NotifyMediaUpdated(playlist, media);
}

static void
on_player_media_subitems_changed(heimdall_player_t *player, input_item_t *media,
                                 const input_item_node_t *subitems,
                                 void *userdata)
{
    HEIMDALL_UNUSED(player);
    HEIMDALL_UNUSED(media);
    heimdall_playlist_t *playlist = userdata;
    heimdall_playlist_ExpandItemFromNode(playlist, subitems);
}

static const struct heimdall_player_cbs player_callbacks = {
    .on_current_media_changed = player_on_current_media_changed,
    .on_state_changed = on_player_state_changed,
    .on_media_meta_changed = on_player_media_meta_changed,
    .on_length_changed = on_player_media_length_changed,
    .on_media_subitems_changed = on_player_media_subitems_changed,
};

bool
heimdall_playlist_PlayerInit(heimdall_playlist_t *playlist, heimdall_object_t *parent)
{
    playlist->player = heimdall_player_New(parent, HEIMDALL_PLAYER_LOCK_NORMAL);
    if (unlikely(!playlist->player))
        return false;

    heimdall_player_Lock(playlist->player);
    /* the playlist and the player share the lock */
    heimdall_playlist_AssertLocked(playlist);
    playlist->player_listener = heimdall_player_AddListener(playlist->player,
                                                       &player_callbacks,
                                                       playlist);
    heimdall_player_Unlock(playlist->player);
    if (unlikely(!playlist->player_listener))
    {
        heimdall_player_Delete(playlist->player);
        return false;
    }
    return true;
}

void
heimdall_playlist_PlayerDestroy(heimdall_playlist_t *playlist)
{
    heimdall_player_Lock(playlist->player);
    heimdall_player_RemoveListener(playlist->player, playlist->player_listener);
    heimdall_player_Unlock(playlist->player);

    heimdall_player_Delete(playlist->player);
}

heimdall_player_t *
heimdall_playlist_GetPlayer(heimdall_playlist_t *playlist)
{
    return playlist->player;
}

int
heimdall_playlist_Start(heimdall_playlist_t *playlist)
{
    return heimdall_player_Start(playlist->player);
}

void
heimdall_playlist_Stop(heimdall_playlist_t *playlist)
{
    heimdall_player_Stop(playlist->player);
}

void
heimdall_playlist_Pause(heimdall_playlist_t *playlist)
{
    heimdall_player_Pause(playlist->player);
}

void
heimdall_playlist_Resume(heimdall_playlist_t *playlist)
{
    heimdall_player_Resume(playlist->player);
}

void
heimdall_playlist_SetMediaStoppedAction(heimdall_playlist_t *playlist,
                                   enum heimdall_playlist_media_stopped_action action)
{
    heimdall_playlist_AssertLocked(playlist);
    playlist->stopped_action = action;
    heimdall_playlist_UpdateNextMedia(playlist);
    heimdall_playlist_Notify(playlist, on_media_stopped_action_changed, action);
}
