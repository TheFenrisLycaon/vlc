/*****************************************************************************
 * playlist/playlist.c
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

#include "playlist.h"

#include <heimdall_common.h>
#include <heimdall_preparser.h>

#include "content.h"
#include "item.h"
#include "player.h"

heimdall_playlist_t *
heimdall_playlist_New(heimdall_object_t *parent, enum heimdall_playlist_preparsing rec,
                 unsigned preparse_max_threads, heimdall_tick_t preparse_timeout)
{
    heimdall_playlist_t *playlist = malloc(sizeof(*playlist));
    if (unlikely(!playlist))
        return NULL;

    if (rec != HEIMDALL_PLAYLIST_PREPARSING_DISABLED)
    {
        const struct heimdall_preparser_cfg cfg = {
            .types = HEIMDALL_PREPARSER_TYPE_PARSE | HEIMDALL_PREPARSER_TYPE_FETCHMETA_LOCAL,
            .max_parser_threads = preparse_max_threads,
            .timeout = preparse_timeout,
#if !defined(HAVE_HEIMDALL_EXTERNAL_PREPARSER)
        .external_process = false,
#else
        .external_process = true,
#endif
        };
        playlist->parser = heimdall_preparser_New(parent, &cfg);
        if (playlist->parser == NULL)
        {
            free(playlist);
            return NULL;
        }
    }
    else
        playlist->parser = NULL;
    playlist->recursive = rec;

    bool ok = heimdall_playlist_PlayerInit(playlist, parent);
    if (unlikely(!ok))
    {
        if (playlist->parser != NULL)
            heimdall_preparser_Delete(playlist->parser);
        free(playlist);
        return NULL;
    }
    playlist->stopped_action = HEIMDALL_PLAYLIST_MEDIA_STOPPED_CONTINUE;

    heimdall_vector_init(&playlist->items);
    randomizer_Init(&playlist->randomizer);
    playlist->current = -1;
    playlist->has_prev = false;
    playlist->has_next = false;
    heimdall_list_init(&playlist->listeners);
    playlist->repeat = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE;
    playlist->order = HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL;
    playlist->idgen = 0;

    return playlist;
}

void
heimdall_playlist_Delete(heimdall_playlist_t *playlist)
{
    assert(heimdall_list_is_empty(&playlist->listeners));

    if (playlist->parser != NULL) {
        heimdall_preparser_Delete(playlist->parser);
    }

    heimdall_playlist_PlayerDestroy(playlist);
    randomizer_Destroy(&playlist->randomizer);
    heimdall_playlist_ClearItems(playlist);
    free(playlist);
}

void
heimdall_playlist_Lock(heimdall_playlist_t *playlist)
{
    heimdall_player_Lock(playlist->player);
}

void
heimdall_playlist_Unlock(heimdall_playlist_t *playlist)
{
    heimdall_player_Unlock(playlist->player);
}
