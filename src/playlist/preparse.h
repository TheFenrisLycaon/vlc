/*****************************************************************************
 * playlist/preparse.h
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

#ifndef HEIMDALL_PLAYLIST_PREPARSE_H
#define HEIMDALL_PLAYLIST_PREPARSE_H

#include <heimdall_common.h>
#include <heimdall_preparser.h>

#include "item.h"

typedef struct heimdall_playlist heimdall_playlist_t;
typedef struct input_item_node_t input_item_node_t;

/* Start an auto-preparse request for the media of a playlist item.
   On success, the request is stored in `item->preparser_req`, which owns it.
   Do nothing if the media does not need to be preparsed or if the request
   could not be started. */
void
heimdall_playlist_AutoPreparse(heimdall_playlist_t *playlist, heimdall_playlist_item_t *item,
                          bool parse_subitems);

int
heimdall_playlist_ExpandItem(heimdall_playlist_t *playlist, size_t index,
                        const input_item_node_t *node);

int
heimdall_playlist_ExpandItemFromNode(heimdall_playlist_t *playlist,
                                const input_item_node_t *subitems);

#endif
