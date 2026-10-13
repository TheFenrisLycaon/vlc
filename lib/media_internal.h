/*****************************************************************************
 * media_internal.h : Definition of opaque structures for libheimdall exported API
 * Also contains some internal utility functions
 *****************************************************************************
 * Copyright (C) 2005-2009 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
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

#ifndef _LIBHEIMDALL_MEDIA_INTERNAL_H
#define _LIBHEIMDALL_MEDIA_INTERNAL_H 1

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_media.h>

#include <heimdall_common.h>
#include <heimdall_input.h>
#include <heimdall_player.h>
#include <heimdall_preparser.h>
#include <heimdall_atomic.h>

/* internal parse state of a media */
typedef enum libheimdall_media_parsed_status_t
{
    libheimdall_media_parsed_status_none,
    libheimdall_media_parsed_status_done,
} libheimdall_media_parsed_status_t;

/* A media is a wrapper of its input item and shares its reference count:
 * libheimdall_media_retain()/libheimdall_media_release() forward to the input item and
 * the media is destroyed with it through input_item_SetLibheimdallOwner(). */
struct libheimdall_media_t
{
    input_item_t      *p_input_item;

    HEIMDALL_FORWARD_DECLARE_OBJECT(libheimdall_media_list_t*) p_subitems; /* A media descriptor can have Sub items. This is the only dependency we really have on media_list */
    void *p_user_data;

    _Atomic libheimdall_media_parsed_status_t parsed_status;
    heimdall_preparser_req *req;
};

/* Media Descriptor */
libheimdall_media_t * libheimdall_media_new_from_input_item( input_item_t * );

void libheimdall_media_add_subtree(libheimdall_media_t *, const input_item_node_t *);

static inline enum es_format_category_e
libheimdall_track_type_to_escat( libheimdall_track_type_t i_type )
{
    switch( i_type )
    {
        case libheimdall_track_audio:
            return AUDIO_ES;
        case libheimdall_track_video:
            return VIDEO_ES;
        case libheimdall_track_text:
            return SPU_ES;
        case libheimdall_track_unknown:
        default:
            return UNKNOWN_ES;
    }
}

typedef struct libheimdall_media_trackpriv_t
{
    libheimdall_media_track_t t;
    union {
        libheimdall_audio_track_t audio;
        libheimdall_video_track_t video;
        libheimdall_subtitle_track_t subtitle;
    };
    heimdall_es_id_t *es_id;
    char *item_str_id;
    heimdall_atomic_rc_t rc;
} libheimdall_media_trackpriv_t;

static inline const libheimdall_media_trackpriv_t *
libheimdall_media_track_to_priv( const libheimdall_media_track_t *track )
{
    return container_of( track, const libheimdall_media_trackpriv_t, t );
}

void
libheimdall_media_trackpriv_from_es( libheimdall_media_trackpriv_t *trackpriv,
                                const es_format_t *es  );

libheimdall_media_track_t *
libheimdall_media_track_create_from_player_track( const struct heimdall_player_track *track );

libheimdall_media_tracklist_t *
libheimdall_media_tracklist_from_item( input_item_t *item, libheimdall_track_type_t type );

libheimdall_media_tracklist_t *
libheimdall_media_tracklist_from_player( heimdall_player_t *player,
                                    libheimdall_track_type_t type, bool selected );

#endif
