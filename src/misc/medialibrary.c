/*****************************************************************************
 * medialib.cpp: medialibrary module
 *****************************************************************************
 * Copyright © 2015-2016 VLC authors, VideoLAN and VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
#include <heimdall_media_library.h>
#include <heimdall_modules.h>
#include <heimdall_list.h>
#include <heimdall_threads.h>
#include "../libheimdall.h"

#include <assert.h>

struct heimdall_ml_event_callback_t
{
    heimdall_ml_callback_t pf_cb;
    void* p_data;
    struct heimdall_list node;
};

struct heimdall_medialibrary_t
{
    heimdall_medialibrary_module_t m;

    heimdall_mutex_t lock;
    struct heimdall_list cbs;
};

static heimdall_medialibrary_t* ml_priv( heimdall_medialibrary_module_t* p_ml )
{
    return container_of( p_ml, struct heimdall_medialibrary_t, m );
}

static void heimdall_ml_event_send( heimdall_medialibrary_module_t* p_ml, const heimdall_ml_event_t* p_event )
{
    heimdall_medialibrary_t* p_priv = ml_priv( p_ml );
    heimdall_mutex_lock( &p_priv->lock );
    struct heimdall_ml_event_callback_t* p_cb;
    heimdall_list_foreach( p_cb, &p_priv->cbs, node )
    {
        p_cb->pf_cb( p_cb->p_data, p_event );
    }
    heimdall_mutex_unlock( &p_priv->lock );
}

heimdall_ml_event_callback_t*
heimdall_ml_event_register_callback( heimdall_medialibrary_t* p_ml, heimdall_ml_callback_t cb,
                                void* p_data )
{
    struct heimdall_ml_event_callback_t* p_cb = malloc( sizeof( *p_cb ) );
    if ( unlikely( p_cb == NULL ) )
        return NULL;
    p_cb->pf_cb = cb;
    p_cb->p_data = p_data;
    heimdall_mutex_lock( &p_ml->lock );
    heimdall_list_append( &p_cb->node, &p_ml->cbs );
    heimdall_mutex_unlock( &p_ml->lock );
    return p_cb;
}

void heimdall_ml_event_unregister_callback( heimdall_medialibrary_t* p_ml,
                                       heimdall_ml_event_callback_t* p_cb )
{
    heimdall_mutex_lock( &p_ml->lock );
    heimdall_list_remove( &p_cb->node );
    heimdall_mutex_unlock( &p_ml->lock );
    free( p_cb );
}

void heimdall_ml_event_unregister_from_callback( heimdall_medialibrary_t* p_ml,
                                            heimdall_ml_event_callback_t* p_cb )
{
    heimdall_mutex_assert( &p_ml->lock );
    heimdall_list_remove( &p_cb->node );
    free( p_cb );
}

static const heimdall_medialibrary_callbacks_t callbacks = {
    .pf_send_event = &heimdall_ml_event_send
};

heimdall_medialibrary_t* libheimdall_MlCreate( libheimdall_int_t* p_libheimdall  )
{
    heimdall_medialibrary_t *p_ml = heimdall_custom_create( HEIMDALL_OBJECT( p_libheimdall ),
                                                  sizeof( *p_ml ), "medialibrary" );
    if ( unlikely( p_ml == NULL ) )
        return NULL;
    heimdall_mutex_init( &p_ml->lock );
    heimdall_list_init( &p_ml->cbs );
    p_ml->m.cbs = &callbacks;
    p_ml->m.p_module = module_need( &p_ml->m, "medialibrary", NULL, false );
    if ( p_ml->m.p_module == NULL )
    {
        heimdall_object_delete(&p_ml->m);
        return NULL;
    }
    return p_ml;
}

void libheimdall_MlRelease( heimdall_medialibrary_t* p_ml )
{
    assert( p_ml != NULL );
    module_unneed( &p_ml->m, p_ml->m.p_module );
    assert( heimdall_list_is_empty( &p_ml->cbs ) );
    heimdall_object_delete(&p_ml->m);
}

#undef heimdall_ml_instance_get
heimdall_medialibrary_t* heimdall_ml_instance_get( heimdall_object_t* p_obj )
{
    libheimdall_priv_t* p_priv = libheimdall_priv( heimdall_object_instance(p_obj) );
    return p_priv->p_media_library;
}

static void heimdall_ml_thumbnails_release( heimdall_ml_thumbnail_t *p_thumbnails )
{
    for ( int i = 0; i < HEIMDALL_ML_THUMBNAIL_SIZE_COUNT; ++i )
        free( p_thumbnails[i].psz_mrl );
}

static void heimdall_ml_show_release_inner( heimdall_ml_show_t* p_show )
{
    free( p_show->psz_artwork_mrl );
    free( p_show->psz_name );
    free( p_show->psz_summary );
    free( p_show->psz_tvdb_id );
}

void heimdall_ml_show_release( heimdall_ml_show_t* p_show )
{
    if ( p_show == NULL )
        return;
    heimdall_ml_show_release_inner( p_show );
    free( p_show );
}

static void heimdall_ml_media_release_tracks_inner( heimdall_ml_media_track_list_t* p_tracks )
{
    if ( p_tracks == NULL )
        return;
    for ( size_t i = 0; i < p_tracks->i_nb_items; ++i )
    {
        heimdall_ml_media_track_t* p_track = &p_tracks->p_items[i];
        free( p_track->psz_codec );
        free( p_track->psz_language );
        free( p_track->psz_description );

        if ( p_track->i_type == HEIMDALL_ML_TRACK_TYPE_SUBTITLE )
            free( p_track->s.psz_encoding );
    }
    free( p_tracks );
}

static void heimdall_ml_media_release_inner( heimdall_ml_media_t* p_media )
{
    heimdall_ml_file_list_release( p_media->p_files );
    heimdall_ml_media_release_tracks_inner( p_media->p_tracks );
    free( p_media->psz_filename );
    free( p_media->psz_title );
    heimdall_ml_thumbnails_release( p_media->thumbnails );
    switch( p_media->i_subtype )
    {
        case HEIMDALL_ML_MEDIA_SUBTYPE_ALBUMTRACK:
            break;
        case HEIMDALL_ML_MEDIA_SUBTYPE_SHOW_EPISODE:
            free( p_media->show_episode.psz_summary );
            free( p_media->show_episode.psz_tvdb_id );
            break;
        case HEIMDALL_ML_MEDIA_SUBTYPE_MOVIE:
            free( p_media->movie.psz_summary );
            free( p_media->movie.psz_imdb_id );
            break;
        default:
            break;
    }
}

static void heimdall_ml_artist_release_inner( heimdall_ml_artist_t* p_artist )
{
    heimdall_ml_thumbnails_release( p_artist->thumbnails );
    free( p_artist->psz_name );
    free( p_artist->psz_shortbio );
    free( p_artist->psz_mb_id );
}

void heimdall_ml_artist_release( heimdall_ml_artist_t* p_artist )
{
    if ( p_artist == NULL )
        return;
    heimdall_ml_artist_release_inner( p_artist );
    free( p_artist );
}

static void heimdall_ml_album_release_inner( heimdall_ml_album_t* p_album )
{
    heimdall_ml_thumbnails_release( p_album->thumbnails );
    free( p_album->psz_artist );
    free( p_album->psz_summary );
    free( p_album->psz_title );
}

void heimdall_ml_album_release( heimdall_ml_album_t* p_album )
{
    if ( p_album == NULL )
        return;
    heimdall_ml_album_release_inner( p_album );
    free( p_album );
}

void heimdall_ml_genre_release( heimdall_ml_genre_t* p_genre )
{
    if ( p_genre == NULL )
        return;
    heimdall_ml_thumbnails_release( p_genre->thumbnails );
    free( p_genre->psz_name );
    free( p_genre );
}

//-------------------------------------------------------------------------------------------------
// Group

static void heimdall_ml_group_release_inner( heimdall_ml_group_t* p_group )
{
    free( p_group->psz_name );
}

void heimdall_ml_group_release( heimdall_ml_group_t* p_group )
{
    if ( p_group == NULL )
        return;
    heimdall_ml_group_release_inner( p_group );
    free( p_group );
}

//-------------------------------------------------------------------------------------------------

static void heimdall_ml_playlist_release_inner( heimdall_ml_playlist_t* p_playlist )
{
    free( p_playlist->psz_name );
    free( p_playlist->psz_mrl );
    free( p_playlist->psz_artwork_mrl );
}

void heimdall_ml_playlist_release( heimdall_ml_playlist_t* p_playlist )
{
    if ( p_playlist == NULL )
        return;
    heimdall_ml_playlist_release_inner( p_playlist );
    free( p_playlist );
}


void heimdall_ml_media_release( heimdall_ml_media_t* p_media )
{
    if ( p_media == NULL )
        return;
    heimdall_ml_media_release_inner( p_media );
    free( p_media );
}

static void heimdall_folder_release_inner( heimdall_ml_folder_t* p_folder )
{
    free( p_folder->psz_name );
    free( p_folder->psz_mrl );
}

void heimdall_ml_folder_release( heimdall_ml_folder_t* p_folder )
{
    if ( p_folder == NULL )
        return;
    heimdall_folder_release_inner( p_folder );
    free( p_folder );
}

/* Lists release */

void heimdall_ml_label_list_release( heimdall_ml_label_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        free( p_list->p_items[i].psz_name );
    free( p_list );
}

void heimdall_ml_file_list_release( heimdall_ml_file_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        free( p_list->p_items[i].psz_mrl );
    free( p_list );
}

void heimdall_ml_artist_list_release( heimdall_ml_artist_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        heimdall_ml_artist_release_inner( &p_list->p_items[i] );
    free( p_list );
}


void heimdall_ml_media_list_release( heimdall_ml_media_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        heimdall_ml_media_release_inner( &p_list->p_items[i] );
    free( p_list );
}

void heimdall_ml_album_list_release( heimdall_ml_album_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        heimdall_ml_album_release_inner( &p_list->p_items[i] );
    free( p_list );
}

void heimdall_ml_show_list_release( heimdall_ml_show_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        heimdall_ml_show_release_inner( &p_list->p_items[i] );
    free( p_list );
}

void heimdall_ml_genre_list_release( heimdall_ml_genre_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        free( p_list->p_items[i].psz_name );
    free( p_list );
}

void heimdall_ml_group_list_release( heimdall_ml_group_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        heimdall_ml_group_release_inner( &p_list->p_items[i] );
    free( p_list );
}

void heimdall_ml_playlist_list_release( heimdall_ml_playlist_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        heimdall_ml_playlist_release_inner( &p_list->p_items[i] );
    free( p_list );
}

void heimdall_ml_folder_list_release( heimdall_ml_folder_list_t* p_list )
{
    if ( p_list == NULL )
        return;
    for ( size_t i = 0; i < p_list->i_nb_items; ++i )
        heimdall_folder_release_inner( &p_list->p_items[i] );
    free( p_list );
}

void heimdall_ml_playback_states_all_release( heimdall_ml_playback_states_all* prefs )
{
    free( prefs->current_video_track );
    free( prefs->current_audio_track );
    free( prefs->current_subtitle_track );
    free( prefs->aspect_ratio );
    free( prefs->crop );
    free( prefs->deinterlace );
    free( prefs->video_filter );
}

static void heimdall_ml_bookmark_release_inner( heimdall_ml_bookmark_t* bookmark )
{
    free( bookmark->psz_name );
    free( bookmark->psz_description );
}

void heimdall_ml_bookmark_release( heimdall_ml_bookmark_t* bookmark )
{
    if ( bookmark == NULL )
        return;
    heimdall_ml_bookmark_release_inner( bookmark );
    free( bookmark );
}

void heimdall_ml_bookmark_list_release( heimdall_ml_bookmark_list_t* list )
{
    if ( list == NULL )
        return;
    for ( size_t i = 0; i < list->i_nb_items; ++i )
        heimdall_ml_bookmark_release_inner( &list->p_items[i] );
    free( list );
}

void* heimdall_ml_get( heimdall_medialibrary_t* p_ml, int i_query, ... )
{
    assert( p_ml != NULL );
    va_list args;
    va_start( args, i_query );
    void* res = p_ml->m.pf_get( &p_ml->m, i_query, args );
    va_end( args );
    return res;
}

int heimdall_ml_control( heimdall_medialibrary_t* p_ml, int i_query, ... )
{
    va_list args;
    va_start( args, i_query );
    int i_res = p_ml->m.pf_control( &p_ml->m, i_query, args );
    va_end( args );
    return i_res;
}

int heimdall_ml_list( heimdall_medialibrary_t* p_ml, int i_query,
                             const heimdall_ml_query_params_t* p_params, ... )
{
    va_list args;
    va_start( args, p_params );
    int i_res = p_ml->m.pf_list( &p_ml->m, i_query, p_params, args );
    va_end( args );
    return i_res;
}
