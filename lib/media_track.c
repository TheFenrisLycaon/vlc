/*****************************************************************************
 * media_track.c: Libheimdall API media track
 *****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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

#include <assert.h>
#include <errno.h>
#include <stdckdint.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>

#include "libheimdall_internal.h"
#include "media_internal.h"

struct libheimdall_media_tracklist_t
{
    size_t count;
    libheimdall_media_trackpriv_t *tracks[];
};

void
libheimdall_media_trackpriv_from_es( libheimdall_media_trackpriv_t *trackpriv,
                                const es_format_t *es  )
{
    libheimdall_media_track_t *track = &trackpriv->t;

    track->i_codec = es->i_codec;
    track->i_original_fourcc = es->i_original_fourcc;
    track->i_id = es->i_id;

    track->i_profile = es->i_profile;
    track->i_level = es->i_level;

    track->i_bitrate = es->i_bitrate;
    track->psz_language = es->psz_language != NULL ? strdup(es->psz_language) : NULL;
    track->psz_description = es->psz_description != NULL ? strdup(es->psz_description) : NULL;
    track->psz_id = NULL;
    track->id_stable = false;
    track->psz_name = NULL;
    track->selected = false;

    switch( es->i_cat )
    {
    case UNKNOWN_ES:
    default:
        track->i_type = libheimdall_track_unknown;
        break;
    case VIDEO_ES:
        track->u.video = &trackpriv->video;
        track->i_type = libheimdall_track_video;
        track->u.video->i_height = es->video.i_visible_height;
        track->u.video->i_width = es->video.i_visible_width;
        track->u.video->i_sar_num = es->video.i_sar_num;
        track->u.video->i_sar_den = es->video.i_sar_den;
        track->u.video->i_frame_rate_num = es->video.i_frame_rate;
        track->u.video->i_frame_rate_den = es->video.i_frame_rate_base;

        assert( es->video.orientation >= ORIENT_TOP_LEFT &&
                es->video.orientation <= ORIENT_RIGHT_BOTTOM );
        track->u.video->i_orientation = (int) es->video.orientation;

        assert( ( es->video.projection_mode >= PROJECTION_MODE_RECTANGULAR &&
                es->video.projection_mode <= PROJECTION_MODE_EQUIRECTANGULAR ) ||
                ( es->video.projection_mode == PROJECTION_MODE_CUBEMAP_LAYOUT_STANDARD ) );
        track->u.video->i_projection = (int) es->video.projection_mode;

        heimdall_viewpoint_to_euler(&es->video.pose,
                               &track->u.video->pose.f_yaw,
                               &track->u.video->pose.f_pitch,
                               &track->u.video->pose.f_roll);
        track->u.video->pose.f_field_of_view = es->video.pose.fov;

        assert( es->video.multiview_mode >= MULTIVIEW_2D &&
                es->video.multiview_mode <= MULTIVIEW_STEREO_CHECKERBOARD );
        track->u.video->i_multiview = (int) es->video.multiview_mode;
        break;
    case AUDIO_ES:
        track->u.audio = &trackpriv->audio;
        track->i_type = libheimdall_track_audio;
        track->u.audio->i_channels = es->audio.i_channels;
        track->u.audio->i_rate = es->audio.i_rate;
        break;
    case SPU_ES:
        track->u.subtitle = &trackpriv->subtitle;
        track->i_type = libheimdall_track_text;
        track->u.subtitle->psz_encoding = es->subs.psz_encoding != NULL ?
                                        strdup(es->subs.psz_encoding) : NULL;
        break;
    }
}

static libheimdall_media_trackpriv_t *
libheimdall_media_trackpriv_new( void )
{
    libheimdall_media_trackpriv_t *trackpriv =
        malloc( sizeof (libheimdall_media_trackpriv_t ) );

    if( trackpriv == NULL )
        return NULL;

    trackpriv->es_id = NULL;
    trackpriv->item_str_id = NULL;
    heimdall_atomic_rc_init( &trackpriv->rc );
    return trackpriv;
}

static void libheimdall_media_track_clean( libheimdall_media_track_t *track )
{
    free( track->psz_language );
    free( track->psz_description );
    free( track->psz_name );
    switch( track->i_type )
    {
    case libheimdall_track_audio:
        break;
    case libheimdall_track_video:
        break;
    case libheimdall_track_text:
        free( track->u.subtitle->psz_encoding );
        break;
    case libheimdall_track_unknown:
    default:
        break;
    }
}

libheimdall_media_track_t *
libheimdall_media_track_retain( libheimdall_media_track_t *track )
{
    libheimdall_media_trackpriv_t *trackpriv =
        container_of( track, libheimdall_media_trackpriv_t, t );
    heimdall_atomic_rc_inc( &trackpriv->rc );
    return track;
}

void
libheimdall_media_track_release( libheimdall_media_track_t *track )
{
    libheimdall_media_trackpriv_t *trackpriv =
        container_of( track, libheimdall_media_trackpriv_t, t );

    if( heimdall_atomic_rc_dec( &trackpriv->rc ) )
    {
        libheimdall_media_track_clean( track );
        if( trackpriv->es_id )
            heimdall_es_id_Release( trackpriv->es_id );
        free( trackpriv->item_str_id );
        free( trackpriv );
    }
}

static libheimdall_media_tracklist_t *
libheimdall_media_tracklist_alloc( size_t count )
{
    size_t size;
    if (ckd_mul(&size, count, sizeof (libheimdall_media_trackpriv_t *)) ||
        ckd_add(&size, size, sizeof (libheimdall_media_tracklist_t)))
        return NULL;

    libheimdall_media_tracklist_t *list = malloc( size );
    if( list == NULL )
        return NULL;

    list->count = 0;
    return list;
}

libheimdall_media_tracklist_t *
libheimdall_media_tracklist_from_item( input_item_t *item, libheimdall_track_type_t type )
{
    size_t count = 0;
    const enum es_format_category_e cat = libheimdall_track_type_to_escat( type );

    for( size_t i = 0; i < item->es_vec.size; ++i )
    {
        const es_format_t *es_fmt = &item->es_vec.data[i].es;
        if( es_fmt->i_cat == cat )
            count++;
    }

    libheimdall_media_tracklist_t *list = libheimdall_media_tracklist_alloc( count );

    if( count == 0 || list == NULL )
        return list;

    for( size_t i = 0; i < item->es_vec.size; ++i )
    {
        const struct input_item_es *item_es = &item->es_vec.data[i];
        const es_format_t *es_fmt = &item_es->es;
        if( es_fmt->i_cat == cat )
        {
            libheimdall_media_trackpriv_t *trackpriv = libheimdall_media_trackpriv_new();
            if( trackpriv == NULL )
            {
                libheimdall_media_tracklist_delete( list );
                return NULL;
            }
            list->tracks[list->count++] = trackpriv;
            libheimdall_media_trackpriv_from_es( trackpriv, es_fmt );

            trackpriv->item_str_id = strdup( item_es->id );
            if( trackpriv->item_str_id == NULL )
            {
                libheimdall_media_tracklist_delete( list );
                return NULL;
            }
            trackpriv->t.psz_id = trackpriv->item_str_id;
            trackpriv->t.id_stable = item_es->id_stable;
        }
    }

    return list;
}

static void
libheimdall_media_trackpriv_from_player_track( libheimdall_media_trackpriv_t *trackpriv,
                                          const struct heimdall_player_track *track )
{
    libheimdall_media_trackpriv_from_es( trackpriv, &track->fmt );

    trackpriv->es_id = heimdall_es_id_Hold( track->es_id );

    trackpriv->t.psz_id = heimdall_es_id_GetStrId( track->es_id );
    trackpriv->t.id_stable = heimdall_es_id_IsStrIdStable( track->es_id );
    trackpriv->t.psz_name = strdup( track->name );
    trackpriv->t.selected = track->selected;
}

libheimdall_media_track_t *
libheimdall_media_track_create_from_player_track( const struct heimdall_player_track *track )
{
    libheimdall_media_trackpriv_t *trackpriv = libheimdall_media_trackpriv_new();
    if( trackpriv == NULL )
        return NULL;
    libheimdall_media_trackpriv_from_player_track( trackpriv, track );
    return &trackpriv->t;
}

libheimdall_media_tracklist_t *
libheimdall_media_tracklist_from_player( heimdall_player_t *player,
                                    libheimdall_track_type_t type, bool selected )
{
    const enum es_format_category_e cat = libheimdall_track_type_to_escat( type );

    const size_t total_count = heimdall_player_GetTrackCount( player, cat );
    size_t count = 0;

    if( selected )
    {
        size_t selected_count = 0;
        for( size_t i = 0; i < total_count; ++i )
        {
            const struct heimdall_player_track *track =
                heimdall_player_GetTrackAt( player, cat, i );
            assert( track );
            if( track->selected )
                selected_count++;
        }
        count = selected_count;
    }
    else
        count = total_count;

    libheimdall_media_tracklist_t *list = libheimdall_media_tracklist_alloc( count );

    if( count == 0 || list == NULL )
        return list;

    for( size_t i = 0; i < total_count; ++i )
    {
        const struct heimdall_player_track *track =
            heimdall_player_GetTrackAt( player, cat, i );
        assert( track );

        if( selected && !track->selected )
            continue;

        libheimdall_media_trackpriv_t *trackpriv = libheimdall_media_trackpriv_new();
        if( trackpriv == NULL )
        {
            libheimdall_media_tracklist_delete( list );
            return NULL;
        }
        list->tracks[list->count++] = trackpriv;
        libheimdall_media_trackpriv_from_player_track( trackpriv, track );
    }

    return list;
}

size_t
libheimdall_media_tracklist_count( const libheimdall_media_tracklist_t *list )
{
    return list->count;
}

libheimdall_media_track_t *
libheimdall_media_tracklist_at( libheimdall_media_tracklist_t *list, size_t idx )
{
    assert( idx < list->count );
    return &list->tracks[idx]->t;
}

void
libheimdall_media_tracklist_delete( libheimdall_media_tracklist_t *list )
{
    for( size_t i = 0; i < list->count; ++i )
    {
        libheimdall_media_trackpriv_t *trackpriv = list->tracks[i];
        libheimdall_media_track_release( &trackpriv->t );
    }
    free( list );
}
