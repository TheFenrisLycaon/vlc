/*****************************************************************************
 * playlist/playlist.h
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

#ifndef HEIMDALL_PLAYLIST_NEW_INTERNAL_H
#define HEIMDALL_PLAYLIST_NEW_INTERNAL_H

#include <heimdall_common.h>
#include <heimdall_playlist.h>
#include <heimdall_preparser.h>
#include <heimdall_vector.h>
#include "../player/player.h"
#include "randomizer.h"

typedef struct input_item_t input_item_t;

#ifdef TEST_PLAYLIST
/* mock the player in tests */
# define heimdall_player_New(a,b) (HEIMDALL_UNUSED(a), malloc(sizeof (heimdall_player_t)))
# define heimdall_player_Delete(p) free(p)
# define heimdall_player_Lock(p) HEIMDALL_UNUSED(p)
# define heimdall_player_Unlock(p) HEIMDALL_UNUSED(p)
# define heimdall_player_AddListener(a,b,c) (HEIMDALL_UNUSED(b), malloc(sizeof(heimdall_player_listener_id)))
# define heimdall_player_RemoveListener(a,b) free(b)
# define heimdall_player_SetCurrentMedia(a,b) (HEIMDALL_UNUSED(b), HEIMDALL_SUCCESS)
# define heimdall_player_SetNextMedia(a,b) HEIMDALL_UNUSED(b)
# define heimdall_player_osd_Message(p, fmt...) HEIMDALL_UNUSED(p)
#endif /* TEST_PLAYLIST */

typedef struct HEIMDALL_VECTOR(heimdall_playlist_item_t *) playlist_item_vector_t;

struct heimdall_playlist
{
    heimdall_player_t *player;
    heimdall_preparser_t *parser;
    libheimdall_int_t *libheimdall;
    enum heimdall_playlist_media_stopped_action stopped_action;
    enum heimdall_playlist_preparsing recursive;
    /* all remaining fields are protected by the lock of the player */
    struct heimdall_player_listener_id *player_listener;
    playlist_item_vector_t items;
    struct randomizer randomizer;
    ssize_t current;
    bool has_prev;
    bool has_next;
    struct heimdall_list listeners; /**< list of heimdall_playlist_listener_id.node */
    enum heimdall_playlist_playback_repeat repeat;
    enum heimdall_playlist_playback_order order;
    uint64_t idgen;
};

/* Also disable heimdall_assert_locked in tests since the symbol is not exported */
#if !defined(NDEBUG) && !defined(TEST_PLAYLIST)
static inline void
heimdall_playlist_AssertLocked(heimdall_playlist_t *playlist)
{
    heimdall_player_assert_locked(playlist->player);
}
#else
#define heimdall_playlist_AssertLocked(x) ((void) (0))
#endif

#endif
