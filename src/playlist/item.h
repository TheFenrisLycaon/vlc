/*****************************************************************************
 * playlist/item.h
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

#ifndef HEIMDALL_PLAYLIST_ITEM_H
#define HEIMDALL_PLAYLIST_ITEM_H

#include <heimdall_atomic.h>
#include <heimdall_preparser.h>

typedef struct heimdall_playlist_item heimdall_playlist_item_t;
typedef struct input_item_t input_item_t;

struct heimdall_playlist_item
{
    input_item_t *media;
    uint64_t id;
    heimdall_preparser_req *preparser_req;
    heimdall_atomic_rc_t rc;
};

/* _New() is private, it is called when inserting new media in the playlist */
heimdall_playlist_item_t *
heimdall_playlist_item_New(input_item_t *media, uint64_t id);

#endif
