/*****************************************************************************
 * playlist/control.c
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

#include "control.h"

#include "item.h"
#include "notify.h"
#include "playlist.h"
#include "player.h"

static void
heimdall_playlist_PlaybackOrderChanged(heimdall_playlist_t *playlist)
{
    if (playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
    {
        /* randomizer is expected to be empty at this point */
        assert(randomizer_Count(&playlist->randomizer) == 0);
        if (playlist->items.size)
            randomizer_Add(&playlist->randomizer, playlist->items.data,
                           playlist->items.size);

        bool loop = playlist->repeat == HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL;
        randomizer_SetLoop(&playlist->randomizer, loop);
    }
    else
        /* we don't use the randomizer anymore */
        randomizer_Clear(&playlist->randomizer);

    struct heimdall_playlist_state state;
    heimdall_playlist_state_Save(playlist, &state);

    playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
    playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);

    heimdall_playlist_Notify(playlist, on_playback_order_changed, playlist->order);
    heimdall_playlist_state_NotifyChanges(playlist, &state);

    char const *state_text = NULL;
    switch (playlist->order)
    {
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL:
            state_text = N_("Off");
            break;
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM:
            state_text = N_("On");
            break;
    }
    heimdall_player_osd_Message(playlist->player,
                           _("Random: %s"), heimdall_gettext(state_text));
    /* heimdall_player_osd_Message() does nothing in tests */
    HEIMDALL_UNUSED(state_text);
}

static void
heimdall_playlist_PlaybackRepeatChanged(heimdall_playlist_t *playlist)
{
    if (playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
    {
        bool loop = playlist->repeat == HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL;
        randomizer_SetLoop(&playlist->randomizer, loop);
    }

    struct heimdall_playlist_state state;
    heimdall_playlist_state_Save(playlist, &state);

    playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
    playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);

    heimdall_playlist_Notify(playlist, on_playback_repeat_changed, playlist->repeat);
    heimdall_playlist_state_NotifyChanges(playlist, &state);

    char const *state_text = NULL;
    switch (playlist->repeat)
    {
        case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE:
            state_text = N_("Off");
            break;
        case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL:
            state_text = N_("All");
            break;
        case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT:
            state_text = N_("One");
            break;
    }
    heimdall_player_osd_Message(playlist->player,
                           _("Loop: %s"), heimdall_gettext(state_text));
    /* heimdall_player_osd_Message() does nothing in tests */
    HEIMDALL_UNUSED(state_text);
}

enum heimdall_playlist_playback_repeat
heimdall_playlist_GetPlaybackRepeat(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    return playlist->repeat;
}

enum heimdall_playlist_playback_order
heimdall_playlist_GetPlaybackOrder(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    return playlist->order;
}

void
heimdall_playlist_SetPlaybackRepeat(heimdall_playlist_t *playlist,
                               enum heimdall_playlist_playback_repeat repeat)
{
    heimdall_playlist_AssertLocked(playlist);

    if (playlist->repeat == repeat)
        return;

    playlist->repeat = repeat;
    heimdall_playlist_PlaybackRepeatChanged(playlist);
}

void
heimdall_playlist_SetPlaybackOrder(heimdall_playlist_t *playlist,
                              enum heimdall_playlist_playback_order order)
{
    heimdall_playlist_AssertLocked(playlist);

    if (playlist->order == order)
        return;

    playlist->order = order;
    heimdall_playlist_PlaybackOrderChanged(playlist);
}

int
heimdall_playlist_SetCurrentMedia(heimdall_playlist_t *playlist, ssize_t index)
{
    heimdall_playlist_AssertLocked(playlist);

    input_item_t *media = index != -1
                        ? playlist->items.data[index]->media
                        : NULL;
    return heimdall_player_SetCurrentMedia(playlist->player, media);
}

static inline bool
heimdall_playlist_NormalOrderHasPrev(heimdall_playlist_t *playlist)
{
    if (playlist->current == -1)
        return false;

    if (playlist->repeat == HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL)
        return playlist->items.size > 0;

    return playlist->current > 0;
}

static inline size_t
heimdall_playlist_NormalOrderGetPrevIndex(heimdall_playlist_t *playlist)
{
    switch (playlist->repeat)
    {
        case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE:
        case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT:
            return playlist->current - 1;
        case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL:
            if (playlist->current == 0)
                return playlist->items.size - 1;
            return playlist->current - 1;
        default:
            heimdall_assert_unreachable();
    }
}

static inline bool
heimdall_playlist_NormalOrderHasNext(heimdall_playlist_t *playlist)
{
    if (playlist->repeat == HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL)
        return playlist->items.size > 0;

    /* also works if current == -1 or playlist->items.size == 0 */
    return playlist->current < (ssize_t) playlist->items.size - 1;
}

static inline size_t
heimdall_playlist_NormalOrderGetNextIndex(heimdall_playlist_t *playlist)
{
    switch (playlist->repeat)
    {
        case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE:
        case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT:
            assert(playlist->current < (ssize_t) playlist->items.size - 1);
            return playlist->current + 1;
        case HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL:
            assert(playlist->items.size != 0);
            return (playlist->current + 1) % playlist->items.size;
        default:
            heimdall_assert_unreachable();
    }
}

static inline bool
heimdall_playlist_RandomOrderHasPrev(heimdall_playlist_t *playlist)
{
    return randomizer_HasPrev(&playlist->randomizer);
}

static inline size_t
heimdall_playlist_RandomOrderGetPrevIndex(heimdall_playlist_t *playlist)
{
    heimdall_playlist_item_t *prev = randomizer_PeekPrev(&playlist->randomizer);
    assert(prev);
    ssize_t index = heimdall_playlist_IndexOf(playlist, prev);
    assert(index != -1);
    return (size_t) index;
}

static inline bool
heimdall_playlist_RandomOrderHasNext(heimdall_playlist_t *playlist)
{
    if (playlist->repeat == HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL)
        return playlist->items.size > 0;
    return randomizer_HasNext(&playlist->randomizer);
}

static inline size_t
heimdall_playlist_RandomOrderGetNextIndex(heimdall_playlist_t *playlist)
{
    heimdall_playlist_item_t *next = randomizer_PeekNext(&playlist->randomizer);
    assert(next);
    ssize_t index = heimdall_playlist_IndexOf(playlist, next);
    assert(index != -1);
    return (size_t) index;
}

static size_t
heimdall_playlist_GetPrevIndex(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    switch (playlist->order)
    {
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL:
            return heimdall_playlist_NormalOrderGetPrevIndex(playlist);
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM:
            return heimdall_playlist_RandomOrderGetPrevIndex(playlist);
        default:
            heimdall_assert_unreachable();
    }
}

static size_t
heimdall_playlist_GetNextIndex(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    switch (playlist->order)
    {
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL:
            return heimdall_playlist_NormalOrderGetNextIndex(playlist);
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM:
            return heimdall_playlist_RandomOrderGetNextIndex(playlist);
        default:
            heimdall_assert_unreachable();
    }
}

bool
heimdall_playlist_ComputeHasPrev(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    switch (playlist->order)
    {
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL:
            return heimdall_playlist_NormalOrderHasPrev(playlist);
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM:
            return heimdall_playlist_RandomOrderHasPrev(playlist);
        default:
            heimdall_assert_unreachable();
    }
}

bool
heimdall_playlist_ComputeHasNext(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    switch (playlist->order)
    {
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL:
            return heimdall_playlist_NormalOrderHasNext(playlist);
        case HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM:
            return heimdall_playlist_RandomOrderHasNext(playlist);
        default:
            heimdall_assert_unreachable();
    }
}

ssize_t
heimdall_playlist_GetCurrentIndex(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    return playlist->current;
}

static void
heimdall_playlist_SetCurrentIndex(heimdall_playlist_t *playlist, ssize_t index)
{
    struct heimdall_playlist_state state;
    heimdall_playlist_state_Save(playlist, &state);

    playlist->current = index;
    playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
    playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);

    heimdall_playlist_state_NotifyChanges(playlist, &state);
}

bool
heimdall_playlist_HasPrev(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    return playlist->has_prev;
}

bool
heimdall_playlist_HasNext(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    return playlist->has_next;
}

int
heimdall_playlist_Prev(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);

    if (!heimdall_playlist_ComputeHasPrev(playlist))
        return HEIMDALL_EGENERIC;

    ssize_t index = heimdall_playlist_GetPrevIndex(playlist);
    assert(index != -1);

    int ret = heimdall_playlist_SetCurrentMedia(playlist, index);
    if (ret != HEIMDALL_SUCCESS)
        return ret;

    if (playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
    {
        /* mark the item as selected in the randomizer */
        heimdall_playlist_item_t *selected = randomizer_Prev(&playlist->randomizer);
        assert(selected == playlist->items.data[index]);
        HEIMDALL_UNUSED(selected);
    }

    heimdall_playlist_SetCurrentIndex(playlist, index);
    heimdall_player_osd_Message(playlist->player, _("Previous"));
    return HEIMDALL_SUCCESS;
}

int
heimdall_playlist_Next(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);

    if (!heimdall_playlist_ComputeHasNext(playlist))
        return HEIMDALL_EGENERIC;

    ssize_t index = (ssize_t)heimdall_playlist_GetNextIndex(playlist);
    int ret = heimdall_playlist_SetCurrentMedia(playlist, index);
    if (ret != HEIMDALL_SUCCESS)
        return ret;

    if (playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
    {
        /* mark the item as selected in the randomizer */
        heimdall_playlist_item_t *selected = randomizer_Next(&playlist->randomizer);
        assert(selected == playlist->items.data[index]);
        HEIMDALL_UNUSED(selected);
    }

    heimdall_playlist_SetCurrentIndex(playlist, index);
    heimdall_player_osd_Message(playlist->player, _("Next"));
    return HEIMDALL_SUCCESS;
}

int
heimdall_playlist_GoTo(heimdall_playlist_t *playlist, ssize_t index)
{
    heimdall_playlist_AssertLocked(playlist);
    assert(index == -1 || (size_t) index < playlist->items.size);

    int ret = heimdall_playlist_SetCurrentMedia(playlist, index);
    if (ret != HEIMDALL_SUCCESS)
        return ret;

    if (index != -1 && playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
    {
        heimdall_playlist_item_t *item = playlist->items.data[index];
        randomizer_Select(&playlist->randomizer, item);
    }

    heimdall_playlist_SetCurrentIndex(playlist, index);
    return HEIMDALL_SUCCESS;
}

static ssize_t
heimdall_playlist_GetNextMediaIndex(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    if (playlist->repeat == HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT)
        return playlist->current;
    if (!heimdall_playlist_ComputeHasNext(playlist))
        return -1;
    return (ssize_t)heimdall_playlist_GetNextIndex(playlist);
}

void
heimdall_playlist_UpdateNextMedia(heimdall_playlist_t *playlist)
{
    /* the playlist and the player share the lock */
    heimdall_playlist_AssertLocked(playlist);
    input_item_t *media = NULL;

    switch (playlist->stopped_action)
    {
        case HEIMDALL_PLAYLIST_MEDIA_STOPPED_CONTINUE:
        case HEIMDALL_PLAYLIST_MEDIA_STOPPED_EXIT:
        {
            ssize_t index = heimdall_playlist_GetNextMediaIndex(playlist);
            if (index != -1)
                media = playlist->items.data[index]->media;
            /* fall through */
        }
        case HEIMDALL_PLAYLIST_MEDIA_STOPPED_STOP:
            break;
        default:
            heimdall_assert_unreachable();
    }

    heimdall_player_SetNextMedia(playlist->player, media);
}
