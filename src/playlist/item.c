/*****************************************************************************
 * playlist/item.c
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

#include "item.h"

#include <heimdall_playlist.h>
#include <heimdall_input_item.h>

heimdall_playlist_item_t *
heimdall_playlist_item_New(input_item_t *media, uint64_t id)
{
    heimdall_playlist_item_t *item = malloc(sizeof(*item));
    if (unlikely(!item))
        return NULL;

    heimdall_atomic_rc_init(&item->rc);
    item->id = id;
    item->preparser_req = NULL;
    item->media = media;
    input_item_Hold(media);
    return item;
}

void
heimdall_playlist_item_Hold(heimdall_playlist_item_t *item)
{
    heimdall_atomic_rc_inc(&item->rc);
}

void
heimdall_playlist_item_Release(heimdall_playlist_item_t *item)
{
    if (heimdall_atomic_rc_dec(&item->rc))
    {
        if (item->preparser_req != NULL)
            heimdall_preparser_req_Release(item->preparser_req);
        input_item_Release(item->media);
        free(item);
    }
}

input_item_t *
heimdall_playlist_item_GetMedia(heimdall_playlist_item_t *item)
{
    return item->media;
}

uint64_t
heimdall_playlist_item_GetId(heimdall_playlist_item_t *item)
{
    return item->id;
}
