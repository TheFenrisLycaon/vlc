/*****************************************************************************
 * playlist/content.c
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

#include "content.h"

#include "control.h"
#include "item.h"
#include "notify.h"
#include "playlist.h"
#include "preparse.h"

void
heimdall_playlist_ClearItems(heimdall_playlist_t *playlist)
{
    heimdall_playlist_item_t *item;
    heimdall_vector_foreach(item, &playlist->items)
        heimdall_playlist_item_Release(item);
    heimdall_vector_clear(&playlist->items);
}

static void
heimdall_playlist_ItemsReset(heimdall_playlist_t *playlist)
{
    if (playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
        randomizer_Clear(&playlist->randomizer);

    struct heimdall_playlist_state state;
    heimdall_playlist_state_Save(playlist, &state);

    playlist->current = -1;
    playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
    playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);

    heimdall_playlist_Notify(playlist, on_items_reset, playlist->items.data,
                   playlist->items.size);
    heimdall_playlist_state_NotifyChanges(playlist, &state);
}

static void
heimdall_playlist_ItemsInserted(heimdall_playlist_t *playlist, size_t index, size_t count,
                           bool subitems)
{
    if (playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
        randomizer_Add(&playlist->randomizer,
                       &playlist->items.data[index], count);

    struct heimdall_playlist_state state;
    heimdall_playlist_state_Save(playlist, &state);

    if (playlist->current >= (ssize_t) index)
        playlist->current += count;
    playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
    playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);

    heimdall_playlist_item_t **items = &playlist->items.data[index];
    heimdall_playlist_Notify(playlist, on_items_added, index, items, count);
    heimdall_playlist_state_NotifyChanges(playlist, &state);

    for (size_t i = index; i < index + count; ++i)
        heimdall_playlist_AutoPreparse(playlist, playlist->items.data[i], subitems);
}

static void
heimdall_playlist_ItemsMoved(heimdall_playlist_t *playlist, size_t index, size_t count,
                        size_t target)
{
    struct heimdall_playlist_state state;
    heimdall_playlist_state_Save(playlist, &state);

    if (playlist->current != -1) {
        size_t current = (size_t) playlist->current;
        if (index < target)
        {
            if (current >= index && current < index + count)
                /* current item belongs to the moved block */
                playlist->current += target - index;
            else if (current >= index + count && current < target + count)
                /* current item was shifted backwards to the moved block */
                playlist->current -= count;
            /* else the current item does not move */
        }
        else
        {
            if (current >= index && current < index + count)
                /* current item belongs to the moved block */
                playlist->current -= index - target;
            else if (current >= target && current < index)
                /* current item was shifted forward to the moved block */
                playlist->current += count;
            /* else the current item does not move */
        }
    }

    playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
    playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);

    heimdall_playlist_Notify(playlist, on_items_moved, index, count, target);
    heimdall_playlist_state_NotifyChanges(playlist, &state);
}

static void
heimdall_playlist_ItemsRemoving(heimdall_playlist_t *playlist, size_t index, size_t count)
{
    if (playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
        randomizer_Remove(&playlist->randomizer,
                          &playlist->items.data[index], count);
}

/* return whether the current media has changed */
static bool
heimdall_playlist_ItemsRemoved(heimdall_playlist_t *playlist, size_t index, size_t count)
{
    struct heimdall_playlist_state state;
    heimdall_playlist_state_Save(playlist, &state);

    bool current_media_changed = false;
    if (playlist->current != -1) {
        size_t current = (size_t) playlist->current;
        if (current >= index && current < index + count) {
            /* current item has been removed */
            if (index + count < playlist->items.size) {
                /* select the first item after the removed block */
                playlist->current = index;
            } else {
                /* no more items */
                playlist->current = -1;
            }
            current_media_changed = true;
        } else if (current >= index + count) {
            playlist->current -= count;
        }
    }
    playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
    playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);

    heimdall_playlist_Notify(playlist, on_items_removed, index, count);
    heimdall_playlist_state_NotifyChanges(playlist, &state);

    return current_media_changed;
}

static void
heimdall_playlist_ItemReplaced(heimdall_playlist_t *playlist, size_t index)
{
    struct heimdall_playlist_state state;
    heimdall_playlist_state_Save(playlist, &state);

    playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
    playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);

    heimdall_playlist_Notify(playlist, on_items_updated, index,
                        &playlist->items.data[index], 1);
    heimdall_playlist_state_NotifyChanges(playlist, &state);

    heimdall_playlist_AutoPreparse(playlist, playlist->items.data[index], false);
}

size_t
heimdall_playlist_Count(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    return playlist->items.size;
}

heimdall_playlist_item_t *
heimdall_playlist_Get(heimdall_playlist_t *playlist, size_t index)
{
    heimdall_playlist_AssertLocked(playlist);
    return playlist->items.data[index];
}

ssize_t
heimdall_playlist_IndexOf(heimdall_playlist_t *playlist, const heimdall_playlist_item_t *item)
{
    heimdall_playlist_AssertLocked(playlist);

    ssize_t index;
    heimdall_vector_index_of(&playlist->items, item, &index);
    return index;
}

ssize_t
heimdall_playlist_IndexOfMedia(heimdall_playlist_t *playlist, const input_item_t *media)
{
    heimdall_playlist_AssertLocked(playlist);

    playlist_item_vector_t *items = &playlist->items;
    for (size_t i = 0; i < items->size; ++i)
        if (items->data[i]->media == media)
            return i;
    return -1;
}

ssize_t
heimdall_playlist_IndexOfId(heimdall_playlist_t *playlist, uint64_t id)
{
    heimdall_playlist_AssertLocked(playlist);

    playlist_item_vector_t *items = &playlist->items;
    for (size_t i = 0; i < items->size; ++i)
        if (items->data[i]->id == id)
            return i;
    return -1;
}

void
heimdall_playlist_Clear(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);

    int ret = heimdall_player_SetCurrentMedia(playlist->player, NULL);
    HEIMDALL_UNUSED(ret); /* what could we do? */

    if (playlist->parser != NULL)
        heimdall_preparser_Cancel(playlist->parser, NULL);

    heimdall_playlist_ClearItems(playlist);
    heimdall_playlist_ItemsReset(playlist);
}

static int
heimdall_playlist_MediaToItems(heimdall_playlist_t *playlist, input_item_t *const media[],
                          size_t count, heimdall_playlist_item_t *items[])
{
    heimdall_playlist_AssertLocked(playlist);
    size_t i;
    for (i = 0; i < count; ++i)
    {
        uint64_t id = playlist->idgen++;
        items[i] = heimdall_playlist_item_New(media[i], id);
        if (unlikely(!items[i]))
            break;
    }
    if (i < count)
    {
        /* allocation failure, release partial items */
        while (i)
            heimdall_playlist_item_Release(items[--i]);
        return HEIMDALL_ENOMEM;
    }
    return HEIMDALL_SUCCESS;
}

int
heimdall_playlist_Insert(heimdall_playlist_t *playlist, size_t index,
                    input_item_t *const media[], size_t count)
{
    heimdall_playlist_AssertLocked(playlist);
    assert(index <= playlist->items.size);

    /* make space in the vector */
    if (!heimdall_vector_insert_hole(&playlist->items, index, count))
        return HEIMDALL_ENOMEM;

    /* create playlist items in place */
    int ret = heimdall_playlist_MediaToItems(playlist, media, count,
                                        &playlist->items.data[index]);
    if (ret != HEIMDALL_SUCCESS)
    {
        /* we were optimistic, it failed, restore the vector state */
        heimdall_vector_remove_slice(&playlist->items, index, count);
        return ret;
    }

    heimdall_playlist_ItemsInserted(playlist, index, count, true);
    heimdall_playlist_UpdateNextMedia(playlist);

    return HEIMDALL_SUCCESS;
}

void
heimdall_playlist_Move(heimdall_playlist_t *playlist, size_t index, size_t count,
                  size_t target)
{
    heimdall_playlist_AssertLocked(playlist);
    assert(index + count <= playlist->items.size);
    assert(target + count <= playlist->items.size);

    heimdall_vector_move_slice(&playlist->items, index, count, target);

    heimdall_playlist_ItemsMoved(playlist, index, count, target);
    heimdall_playlist_UpdateNextMedia(playlist);
}

void
heimdall_playlist_Remove(heimdall_playlist_t *playlist, size_t index, size_t count)
{
    heimdall_playlist_AssertLocked(playlist);
    assert(index < playlist->items.size);

    heimdall_playlist_ItemsRemoving(playlist, index, count);

    for (size_t i = 0; i < count; ++i) {
        heimdall_playlist_item_t *item = playlist->items.data[index + i];
        if (playlist->parser != NULL
                && item->preparser_req != NULL)
            heimdall_preparser_Cancel(playlist->parser, item->preparser_req);

        heimdall_playlist_item_Release(item);
    }

    heimdall_vector_remove_slice(&playlist->items, index, count);

    bool current_media_changed = heimdall_playlist_ItemsRemoved(playlist, index,
                                                           count);
    if (current_media_changed)
        heimdall_playlist_SetCurrentMedia(playlist, playlist->current);
    else
        heimdall_playlist_UpdateNextMedia(playlist);
}

static int
heimdall_playlist_Replace(heimdall_playlist_t *playlist, size_t index,
                     input_item_t *media)
{
    heimdall_playlist_AssertLocked(playlist);
    assert(index < playlist->items.size);

    uint64_t id = playlist->idgen++;
    heimdall_playlist_item_t *item = heimdall_playlist_item_New(media, id);
    if (!item)
        return HEIMDALL_ENOMEM;

    if (playlist->order == HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM)
    {
        randomizer_Remove(&playlist->randomizer,
                          &playlist->items.data[index], 1);
        randomizer_Add(&playlist->randomizer, &item, 1);
    }

    heimdall_playlist_item_t *old = playlist->items.data[index];
    if (playlist->parser != NULL
            && old->preparser_req != NULL)
        heimdall_preparser_Cancel(playlist->parser, old->preparser_req);
    heimdall_playlist_item_Release(old);
    playlist->items.data[index] = item;

    heimdall_playlist_ItemReplaced(playlist, index);
    return HEIMDALL_SUCCESS;
}

int
heimdall_playlist_Expand(heimdall_playlist_t *playlist, size_t index,
                    input_item_t *const media[], size_t count)
{
    heimdall_playlist_AssertLocked(playlist);
    assert(index < playlist->items.size);

    if (count == 0)
        heimdall_playlist_RemoveOne(playlist, index);
    else
    {
        int ret = heimdall_playlist_Replace(playlist, index, media[0]);
        if (ret != HEIMDALL_SUCCESS)
            return ret;

        if (count > 1)
        {
            /* make space in the vector */
            if (!heimdall_vector_insert_hole(&playlist->items, index + 1, count - 1))
                return HEIMDALL_ENOMEM;

            /* create playlist items in place */
            ret = heimdall_playlist_MediaToItems(playlist, &media[1], count - 1,
                                            &playlist->items.data[index + 1]);
            if (ret != HEIMDALL_SUCCESS)
            {
                /* we were optimistic, it failed, restore the vector state */
                heimdall_vector_remove_slice(&playlist->items, index + 1, count - 1);
                return ret;
            }
            heimdall_playlist_ItemsInserted(playlist, index + 1, count - 1, false);
        }

        if ((ssize_t) index == playlist->current)
            heimdall_playlist_SetCurrentMedia(playlist, playlist->current);
        else
            heimdall_playlist_UpdateNextMedia(playlist);
    }

    return HEIMDALL_SUCCESS;
}
