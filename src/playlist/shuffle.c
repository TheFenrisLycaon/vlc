/*****************************************************************************
 * playlist/shuffle.c
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
#include <heimdall_rand.h>
#include "control.h"
#include "item.h"
#include "notify.h"
#include "playlist.h"

void
heimdall_playlist_Shuffle(heimdall_playlist_t *playlist)
{
    heimdall_playlist_AssertLocked(playlist);
    if (playlist->items.size < 2)
        /* we use size_t (unsigned), so the following loop would be incorrect */
        return;

    heimdall_playlist_item_t *current = playlist->current != -1
                                 ? playlist->items.data[playlist->current]
                                 : NULL;

    /* initialize separately instead of using heimdall_lrand48() to avoid locking the
     * mutex once for each item */
    unsigned short xsubi[3];
    heimdall_rand_bytes(xsubi, sizeof(xsubi));

    /* Fisher-Yates shuffle */
    for (size_t i = playlist->items.size - 1; i != 0; --i)
    {
        size_t selected = (size_t) (nrand48(xsubi) % (i + 1));

        /* swap items i and selected */
        heimdall_playlist_item_t *tmp = playlist->items.data[i];
        playlist->items.data[i] = playlist->items.data[selected];
        playlist->items.data[selected] = tmp;
    }

    struct heimdall_playlist_state state;
    if (current)
    {
        /* the current position have changed after the shuffle */
        heimdall_playlist_state_Save(playlist, &state);
        playlist->current = heimdall_playlist_IndexOf(playlist, current);
        playlist->has_prev = heimdall_playlist_ComputeHasPrev(playlist);
        playlist->has_next = heimdall_playlist_ComputeHasNext(playlist);
    }

    heimdall_playlist_Notify(playlist, on_items_reset, playlist->items.data,
                        playlist->items.size);
    if (current)
        heimdall_playlist_state_NotifyChanges(playlist, &state);
}
