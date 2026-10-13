/*****************************************************************************
 * media.c: Libheimdall API media descriptor management
 *****************************************************************************
 * Copyright (C) 2007 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Pierre d'Herbemont <pdherbemont@videolan.org>
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
#include <limits.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>
#include <heimdall/libheimdall_media_list.h> // For the subitems, here for convenience

#include <heimdall_common.h>
#include <heimdall_meta.h>
#include <heimdall_url.h>
#include <heimdall_atomic.h>

#include "../src/libheimdall.h"

#include "libheimdall_internal.h"
#include "media_internal.h"
#include "media_list_internal.h"
#include "picture_internal.h"

static const heimdall_meta_type_t libheimdall_to_heimdall_meta[] =
{
    [libheimdall_meta_Title]        = heimdall_meta_Title,
    [libheimdall_meta_Artist]       = heimdall_meta_Artist,
    [libheimdall_meta_Genre]        = heimdall_meta_Genre,
    [libheimdall_meta_Copyright]    = heimdall_meta_Copyright,
    [libheimdall_meta_Album]        = heimdall_meta_Album,
    [libheimdall_meta_TrackNumber]  = heimdall_meta_TrackNumber,
    [libheimdall_meta_Description]  = heimdall_meta_Description,
    [libheimdall_meta_Rating]       = heimdall_meta_Rating,
    [libheimdall_meta_Date]         = heimdall_meta_Date,
    [libheimdall_meta_Setting]      = heimdall_meta_Setting,
    [libheimdall_meta_URL]          = heimdall_meta_URL,
    [libheimdall_meta_Language]     = heimdall_meta_Language,
    [libheimdall_meta_NowPlaying]   = heimdall_meta_NowPlaying,
    [libheimdall_meta_Publisher]    = heimdall_meta_Publisher,
    [libheimdall_meta_EncodedBy]    = heimdall_meta_EncodedBy,
    [libheimdall_meta_ArtworkURL]   = heimdall_meta_ArtworkURL,
    [libheimdall_meta_TrackID]      = heimdall_meta_TrackID,
    [libheimdall_meta_TrackTotal]   = heimdall_meta_TrackTotal,
    [libheimdall_meta_Director]     = heimdall_meta_Director,
    [libheimdall_meta_Season]       = heimdall_meta_Season,
    [libheimdall_meta_Episode]      = heimdall_meta_Episode,
    [libheimdall_meta_ShowName]     = heimdall_meta_ShowName,
    [libheimdall_meta_Actors]       = heimdall_meta_Actors,
    [libheimdall_meta_AlbumArtist]  = heimdall_meta_AlbumArtist,
    [libheimdall_meta_DiscNumber]   = heimdall_meta_DiscNumber,
    [libheimdall_meta_DiscTotal]    = heimdall_meta_DiscTotal,
    [libheimdall_meta_Compilation]  = heimdall_meta_Compilation,
};

static_assert(
    ORIENT_TOP_LEFT     == (int) libheimdall_video_orient_top_left &&
    ORIENT_TOP_RIGHT    == (int) libheimdall_video_orient_top_right &&
    ORIENT_BOTTOM_LEFT  == (int) libheimdall_video_orient_bottom_left &&
    ORIENT_BOTTOM_RIGHT == (int) libheimdall_video_orient_bottom_right &&
    ORIENT_LEFT_TOP     == (int) libheimdall_video_orient_left_top &&
    ORIENT_LEFT_BOTTOM  == (int) libheimdall_video_orient_left_bottom &&
    ORIENT_RIGHT_TOP    == (int) libheimdall_video_orient_right_top &&
    ORIENT_RIGHT_BOTTOM == (int) libheimdall_video_orient_right_bottom,
    "Mismatch between libheimdall_video_orient_t and video_orientation_t" );

static_assert(
    PROJECTION_MODE_RECTANGULAR             == (int) libheimdall_video_projection_rectangular &&
    PROJECTION_MODE_EQUIRECTANGULAR         == (int) libheimdall_video_projection_equirectangular &&
    PROJECTION_MODE_CUBEMAP_LAYOUT_STANDARD == (int) libheimdall_video_projection_cubemap_layout_standard,
    "Mismatch between libheimdall_video_projection_t and video_projection_mode_t" );

static_assert(
    MULTIVIEW_2D                    == (int) libheimdall_video_multiview_2d &&
    MULTIVIEW_STEREO_SBS            == (int) libheimdall_video_multiview_stereo_sbs &&
    MULTIVIEW_STEREO_TB             == (int) libheimdall_video_multiview_stereo_tb &&
    MULTIVIEW_STEREO_ROW            == (int) libheimdall_video_multiview_stereo_row &&
    MULTIVIEW_STEREO_COL            == (int) libheimdall_video_multiview_stereo_col &&
    MULTIVIEW_STEREO_FRAME          == (int) libheimdall_video_multiview_stereo_frame &&
    MULTIVIEW_STEREO_CHECKERBOARD   == (int) libheimdall_video_multiview_stereo_checkerboard,
    "Mismatch between libheimdall_video_multiview_t and video_multiview_mode_t");

static libheimdall_media_t *input_item_add_subitem( libheimdall_media_t *p_md,
                                               input_item_t *item )
{
    libheimdall_media_t * p_md_child;
    libheimdall_media_list_t *p_subitems;

    p_md_child = libheimdall_media_new_from_input_item( item );

    /* Add this to our media list */
    p_subitems = p_md->p_subitems;
    libheimdall_media_list_internal_add_media( p_subitems, p_md_child );

    return p_md_child;
}

struct heimdall_item_list
{
    struct heimdall_list node;
    const input_item_node_t *item;
    libheimdall_media_t *media;
};

static struct heimdall_item_list *
wrap_item_in_list( libheimdall_media_t *media, const input_item_node_t *item )
{
    struct heimdall_item_list *node = malloc( sizeof *node );
    if( node == NULL )
        return NULL;
    node->item = item;
    node->media = media;
    return node;
}

static void input_item_add_subnode( libheimdall_media_t *md,
                                    const input_item_node_t *root )
{
    struct heimdall_list list;
    heimdall_list_init( &list );

    /* Retain the media because we don't want the search algorithm to release
     * it when its subitems get parsed. */
    libheimdall_media_retain(md);
    libheimdall_media_list_lock(md->p_subitems);
    libheimdall_media_list_internal_clear(md->p_subitems);

    struct heimdall_item_list *node_root = wrap_item_in_list( md, root );
    if( node_root == NULL )
    {
        libheimdall_media_release(md);
        goto error;
    }

    /* This is a depth-first search algorithm, so stash the root of the tree
     * first, then stash its children and loop back on the last item in the
     * list until the full subtree is parsed, and eventually the full tree is
     * parsed. */
    heimdall_list_append( &node_root->node, &list );

    while( !heimdall_list_is_empty( &list ) )
    {
        /* Pop last item in the list. */
        struct heimdall_item_list *node =
            heimdall_list_last_entry_or_null( &list, struct heimdall_item_list, node );
        heimdall_list_remove(&node->node);

        for( int i = 0; i < node->item->i_children; i++ )
        {
            input_item_node_t *child = node->item->pp_children[i];

            /* The media will be released when its children will be added to
             * the list. */
            libheimdall_media_t *md_child = input_item_add_subitem( node->media, child->p_item );
            if( md_child == NULL )
                goto error;

            /* No need to lock subitems of the submedia since the submedia is
             * not yet exposed */
            struct heimdall_item_list *submedia =
                wrap_item_in_list( md_child, child );
            if (submedia == NULL)
            {
                libheimdall_media_release( md_child );
                goto error;
            }

            /* Stash a request to parse this subtree. */
            heimdall_list_append( &submedia->node, &list );
        }

        libheimdall_media_release( node->media );
        free( node );
    }

    libheimdall_media_list_unlock(md->p_subitems);
    return;

error:
    libheimdall_printerr( "Not enough memory" );

    struct heimdall_item_list *node;
    heimdall_list_foreach( node, &list, node )
    {
        if( node->media != NULL )
            libheimdall_media_release( node->media );
        free( node );
    }

    libheimdall_media_list_unlock(md->p_subitems);
}

void libheimdall_media_add_subtree(libheimdall_media_t *p_md, const input_item_node_t *node)
{
    input_item_add_subnode( p_md, node );
}

static void media_destroy( void *libheimdall_owner )
{
    libheimdall_media_t *p_md = libheimdall_owner;

    if( p_md->p_subitems )
        libheimdall_media_list_release( p_md->p_subitems );

    free( p_md );
}

/**
 * \internal
 * Create a new media descriptor object from an input_item (Private)
 *
 * That's the generic constructor
 */
libheimdall_media_t * libheimdall_media_new_from_input_item(input_item_t *p_input_item )
{
    libheimdall_media_t * p_md;

    if (!p_input_item)
    {
        libheimdall_printerr( "No input item given" );
        return NULL;
    }

    /* The item is wrapped by a single media for its whole lifetime */
    assert( input_item_GetLibheimdallOwner( p_input_item ) == NULL );

    p_md = calloc( 1, sizeof(libheimdall_media_t) );
    if( !p_md )
    {
        libheimdall_printerr( "Not enough memory" );
        return NULL;
    }

    p_md->p_subitems = libheimdall_media_list_new();
    if( p_md->p_subitems == NULL )
    {
        free( p_md );
        return NULL;
    }
    p_md->p_subitems->b_read_only = true;
    p_md->p_subitems->p_internal_md = p_md;

    p_md->p_input_item      = p_input_item;

    input_item_SetLibheimdallOwner( p_md->p_input_item, p_md, media_destroy );
    atomic_init(&p_md->parsed_status, libheimdall_media_parsed_status_none);
    p_md->req = NULL;

    input_item_Hold( p_md->p_input_item );

    return p_md;
}

// Create a media with a certain given media resource location
libheimdall_media_t *libheimdall_media_new_location(const char * psz_mrl)
{
    input_item_t * p_input_item;
    libheimdall_media_t * p_md;

    p_input_item = input_item_New( psz_mrl, NULL );

    if (!p_input_item)
    {
        libheimdall_printerr( "Not enough memory" );
        return NULL;
    }

    p_md = libheimdall_media_new_from_input_item( p_input_item );

    /* The p_input_item is retained in libheimdall_media_new_from_input_item */
    input_item_Release( p_input_item );

    return p_md;
}

// Create a media for a certain file path
libheimdall_media_t *libheimdall_media_new_path(const char *path)
{
    char *mrl = heimdall_path2uri( path, NULL );
    if( unlikely(mrl == NULL) )
    {
        libheimdall_printerr( "%s", heimdall_strerror_c(errno) );
        return NULL;
    }

    libheimdall_media_t *m = libheimdall_media_new_location(mrl);
    free( mrl );
    return m;
}

// Create a media for an already open file descriptor
libheimdall_media_t *libheimdall_media_new_fd(int fd)
{
    char mrl[16];
    snprintf( mrl, sizeof(mrl), "fd://%d", fd );

    return libheimdall_media_new_location(mrl);
}

// Create a media with custom callbacks to read the data from
libheimdall_media_t *libheimdall_media_new_callbacks(const struct libheimdall_media_open_cbs *cbs,
                                           void *cbs_opaque)
{
    assert(cbs != NULL && cbs->read != NULL);

    /* No different versions to handle for now */
    assert(cbs->version <= 0);

    libheimdall_media_t *m = libheimdall_media_new_location("imem://");
    if (unlikely(m == NULL))
        return NULL;

    input_item_AddOpaque(m->p_input_item, "imem-data", cbs_opaque);
    input_item_AddOpaque(m->p_input_item, "imem-cbs", (void *) cbs);

    return m;
}

// Create a media as an empty node with a given name
libheimdall_media_t * libheimdall_media_new_as_node(const char *psz_name)
{
    input_item_t * p_input_item;
    libheimdall_media_t * p_md;

    p_input_item = input_item_New( INPUT_ITEM_URI_NOP, psz_name );

    if (!p_input_item)
    {
        libheimdall_printerr( "Not enough memory" );
        return NULL;
    }

    p_md = libheimdall_media_new_from_input_item( p_input_item );
    input_item_Release( p_input_item );

    return p_md;
}

// Add an option to the media descriptor
void libheimdall_media_add_option( libheimdall_media_t * p_md,
                              const char * psz_option )
{
    libheimdall_media_add_option_flag( p_md, psz_option,
                          HEIMDALL_INPUT_OPTION_UNIQUE|HEIMDALL_INPUT_OPTION_TRUSTED );
}

// Same as libheimdall_media_add_option but with configurable flags
void libheimdall_media_add_option_flag( libheimdall_media_t * p_md,
                                   const char * ppsz_option,
                                   unsigned i_flags )
{
    input_item_AddOption( p_md->p_input_item, ppsz_option, i_flags );
}

// Delete a media descriptor object
void libheimdall_media_release( libheimdall_media_t *p_md )
{
    if (!p_md)
        return;

    /* The media shares the reference count of its input item and is
     * destroyed with it, see media_destroy() */
    input_item_Release( p_md->p_input_item );
}

// Retain a media descriptor object
libheimdall_media_t *libheimdall_media_retain( libheimdall_media_t *p_md )
{
    assert (p_md);
    input_item_Hold( p_md->p_input_item );
    return p_md;
}

// Duplicate a media descriptor object
libheimdall_media_t *
libheimdall_media_duplicate( libheimdall_media_t *p_md_orig )
{

    input_item_t *dup = input_item_Copy( p_md_orig->p_input_item );
    if( dup == NULL )
        return NULL;
    libheimdall_media_t *p_md = libheimdall_media_new_from_input_item( dup );
    input_item_Release( dup );

    return p_md;
}

// Get mrl from a media descriptor object
char *
libheimdall_media_get_mrl( libheimdall_media_t * p_md )
{
    assert( p_md );
    return input_item_GetURI( p_md->p_input_item );
}

// Getter for meta information
char *libheimdall_media_get_meta( libheimdall_media_t *p_md, libheimdall_meta_t e_meta )
{
    char *psz_meta = NULL;

    if( e_meta == libheimdall_meta_NowPlaying )
    {
        psz_meta = input_item_GetNowPlayingFb( p_md->p_input_item );
    }
    else if( e_meta == libheimdall_meta_Title )
    {
        psz_meta = input_item_GetTitleFbName( p_md->p_input_item );
    }
    else
    {
        psz_meta = input_item_GetMeta( p_md->p_input_item,
                                             libheimdall_to_heimdall_meta[e_meta] );
    }
    return psz_meta;
}

// Set the meta of the media
void libheimdall_media_set_meta( libheimdall_media_t *p_md, libheimdall_meta_t e_meta, const char *psz_value )
{
    assert( p_md );
    input_item_SetMeta( p_md->p_input_item, libheimdall_to_heimdall_meta[e_meta], psz_value );
}

// Getter for meta extra information
char *libheimdall_media_get_meta_extra( libheimdall_media_t *p_md, const char *psz_name )
{
    assert( p_md );
    return input_item_GetMetaExtra( p_md->p_input_item, psz_name );
}

// Set the meta extra of the media
void libheimdall_media_set_meta_extra( libheimdall_media_t *p_md, const char *psz_name, const char *psz_value)
{
    assert( p_md );
    input_item_SetMetaExtra( p_md->p_input_item, psz_name, psz_value );
}

// Getter for meta extra names
unsigned libheimdall_media_get_meta_extra_names( libheimdall_media_t *p_md, char ***pppsz_names )
{
    assert( p_md && pppsz_names );
    return input_item_GetMetaExtraNames( p_md->p_input_item, pppsz_names );
}

// Release a media meta extra names
void libheimdall_media_meta_extra_names_release( char **ppsz_names, unsigned i_count )
{
    if( i_count > 0 )
    {
        assert( ppsz_names );
        for ( unsigned i = 0; i < i_count; i++ )
            free( ppsz_names[i] );
    }
    free( ppsz_names );
}

// Save the meta previously set
int libheimdall_media_save_meta( libheimdall_instance_t *inst, libheimdall_media_t *p_md )
{
    assert( p_md );
    heimdall_object_t *p_obj = HEIMDALL_OBJECT(inst->p_libheimdall_int);
    return input_item_WriteMeta( p_obj, p_md->p_input_item ) == HEIMDALL_SUCCESS;
}

// Get subitems of media descriptor object.
libheimdall_media_list_t *
libheimdall_media_subitems( libheimdall_media_t * p_md )
{
    libheimdall_media_list_retain( p_md->p_subitems );
    return p_md->p_subitems;
}

// Getter for statistics information
bool libheimdall_media_get_stats(libheimdall_media_t *p_md,
                            libheimdall_media_stats_t *p_stats)
{
    input_item_t *item = p_md->p_input_item;

    if( !p_md->p_input_item )
        return false;

    heimdall_mutex_lock( &item->lock );

    input_stats_t *p_itm_stats = p_md->p_input_item->p_stats;
    if( p_itm_stats == NULL )
    {
        heimdall_mutex_unlock( &item->lock );
        return false;
    }

    p_stats->i_read_bytes = p_itm_stats->i_read_bytes;
    p_stats->f_input_bitrate = p_itm_stats->f_input_bitrate;

    p_stats->i_demux_read_bytes = p_itm_stats->i_demux_read_bytes;
    p_stats->f_demux_bitrate = p_itm_stats->f_demux_bitrate;
    p_stats->i_demux_corrupted = p_itm_stats->i_demux_corrupted;
    p_stats->i_demux_discontinuity = p_itm_stats->i_demux_discontinuity;

    p_stats->i_decoded_video = p_itm_stats->i_decoded_video;
    p_stats->i_decoded_audio = p_itm_stats->i_decoded_audio;

    p_stats->i_displayed_pictures = p_itm_stats->i_displayed_pictures;
    p_stats->i_late_pictures = p_itm_stats->i_late_pictures;
    p_stats->i_lost_pictures = p_itm_stats->i_lost_pictures;

    p_stats->i_played_abuffers = p_itm_stats->i_played_abuffers;
    p_stats->i_lost_abuffers = p_itm_stats->i_lost_abuffers;

    heimdall_mutex_unlock( &item->lock );
    return true;
}

// Get duration of media object (in us)
libheimdall_time_t
libheimdall_media_get_duration( libheimdall_media_t * p_md )
{
    assert( p_md );

    if( !p_md->p_input_item )
    {
        libheimdall_printerr( "No input item" );
        return -1;
    }

    heimdall_tick_t i_duration = input_item_GetDuration( p_md->p_input_item );

    if( i_duration == 0 && !input_item_IsPreparsed( p_md->p_input_item ) )
        return -1;

    return libheimdall_time_from_heimdall_tick( i_duration );
}

int
libheimdall_media_get_filestat( libheimdall_media_t *p_md, unsigned type, uint64_t *out )
{
    assert( p_md );
    assert( out );

    if( !p_md->p_input_item )
    {
        libheimdall_printerr( "No input item" );
        return -1;
    }

    const char *name;
    switch (type)
    {
        case libheimdall_media_filestat_mtime:   name = "mtime"; break;
        case libheimdall_media_filestat_size:    name = "size"; break;
        default:
            libheimdall_printerr( "unknown libheimdall_media_stat" );
            return -1;
    };

    char *str = input_item_GetInfo( p_md->p_input_item, ".stat", name );
    if( str == NULL )
        return 0;

    char *end;
    unsigned long long val = strtoull( str, &end, 10 );

    if( *end != '\0' )
    {
        free( str );
        return -1;
    }
    free( str );

    *out = val;
    return 1;
}

bool
libheimdall_media_is_parsed(libheimdall_media_t *media)
{
    return atomic_load(&media->parsed_status) == libheimdall_media_parsed_status_done;
}

// Sets media descriptor's user_data
void
libheimdall_media_set_user_data( libheimdall_media_t * p_md, void * p_new_user_data )
{
    assert( p_md );
    p_md->p_user_data = p_new_user_data;
}

// Get media descriptor's user_data
void *
libheimdall_media_get_user_data( libheimdall_media_t * p_md )
{
    assert( p_md );
    return p_md->p_user_data;
}

libheimdall_media_tracklist_t *
libheimdall_media_get_tracklist( libheimdall_media_t *p_md, libheimdall_track_type_t type )
{
    assert( p_md );

    input_item_t *p_input_item = p_md->p_input_item;

    heimdall_mutex_lock( &p_input_item->lock );
    libheimdall_media_tracklist_t *list =
        libheimdall_media_tracklist_from_item( p_input_item, type );
    heimdall_mutex_unlock( &p_input_item->lock );

    return list;
}

// Get codec description from media elementary stream
const char *
libheimdall_media_get_codec_description( libheimdall_track_type_t i_type,
                                    uint32_t i_codec )
{
    return heimdall_fourcc_GetDescription( libheimdall_track_type_to_escat( i_type),
                                      i_codec );
}

// Get the media type of the media descriptor object
libheimdall_media_type_t libheimdall_media_get_type( libheimdall_media_t *p_md )
{
    assert( p_md );

    enum input_item_type_e i_type;
    input_item_t *p_input_item = p_md->p_input_item;

    heimdall_mutex_lock( &p_input_item->lock );
    i_type = p_md->p_input_item->i_type;
    heimdall_mutex_unlock( &p_input_item->lock );

    switch( i_type )
    {
    case ITEM_TYPE_FILE:
        return libheimdall_media_type_file;
    case ITEM_TYPE_NODE:
    case ITEM_TYPE_DIRECTORY:
        return libheimdall_media_type_directory;
    case ITEM_TYPE_DISC:
        return libheimdall_media_type_disc;
    case ITEM_TYPE_STREAM:
        return libheimdall_media_type_stream;
    case ITEM_TYPE_PLAYLIST:
        return libheimdall_media_type_playlist;
    default:
        return libheimdall_media_type_unknown;
    }
}

// Add a slave to the media descriptor
int libheimdall_media_slaves_add( libheimdall_media_t *p_md,
                             libheimdall_media_slave_type_t i_type,
                             unsigned int i_priority,
                             const char *psz_uri )
{
    assert( p_md && psz_uri );
    input_item_t *p_input_item = p_md->p_input_item;

    enum slave_type i_input_slave_type;
    switch( i_type )
    {
    case libheimdall_media_slave_type_subtitle:
        i_input_slave_type = SLAVE_TYPE_SPU;
        break;
    case libheimdall_media_slave_type_generic:
        i_input_slave_type = SLAVE_TYPE_GENERIC;
        break;
    default:
        heimdall_assert_unreachable();
        return -1;
    }

    enum slave_priority i_input_slave_priority;
    switch( i_priority )
    {
    case 0:
        i_input_slave_priority = SLAVE_PRIORITY_MATCH_NONE;
        break;
    case 1:
        i_input_slave_priority = SLAVE_PRIORITY_MATCH_RIGHT;
        break;
    case 2:
        i_input_slave_priority = SLAVE_PRIORITY_MATCH_LEFT;
        break;
    case 3:
        i_input_slave_priority = SLAVE_PRIORITY_MATCH_ALL;
        break;
    default:
    case 4:
        i_input_slave_priority = SLAVE_PRIORITY_USER;
        break;
    }

    input_item_slave_t *p_slave = input_item_slave_New( psz_uri,
                                                      i_input_slave_type,
                                                      i_input_slave_priority );
    if( p_slave == NULL )
        return -1;
    return input_item_AddSlave( p_input_item, p_slave ) == HEIMDALL_SUCCESS ? 0 : -1;
}

// Clear all slaves of the media descriptor
void libheimdall_media_slaves_clear( libheimdall_media_t *p_md )
{
    assert( p_md );
    input_item_t *p_input_item = p_md->p_input_item;

    heimdall_mutex_lock( &p_input_item->lock );
    for( int i = 0; i < p_input_item->i_slaves; i++ )
        input_item_slave_Delete( p_input_item->pp_slaves[i] );
    TAB_CLEAN( p_input_item->i_slaves, p_input_item->pp_slaves );
    heimdall_mutex_unlock( &p_input_item->lock );
}

// Get a media descriptor's slave list
unsigned int libheimdall_media_slaves_get( libheimdall_media_t *p_md,
                                      libheimdall_media_slave_t ***ppp_slaves )
{
    assert( p_md && ppp_slaves );
    input_item_t *p_input_item = p_md->p_input_item;
    *ppp_slaves = NULL;

    heimdall_mutex_lock( &p_input_item->lock );

    int i_count = p_input_item->i_slaves;
    if( i_count <= 0 )
        return heimdall_mutex_unlock( &p_input_item->lock ), 0;

    libheimdall_media_slave_t **pp_slaves = calloc( i_count, sizeof(*pp_slaves) );
    if( pp_slaves == NULL )
        return heimdall_mutex_unlock( &p_input_item->lock ), 0;

    for( int i = 0; i < i_count; ++i )
    {
        input_item_slave_t *p_item_slave = p_input_item->pp_slaves[i];
        assert( p_item_slave->i_priority >= SLAVE_PRIORITY_MATCH_NONE );

        /* also allocate psz_uri buffer at the end of the struct */
        libheimdall_media_slave_t *p_slave = malloc( sizeof(*p_slave) +
                                                strlen( p_item_slave->psz_uri )
                                                + 1 );
        if( p_slave == NULL )
        {
            libheimdall_media_slaves_release(pp_slaves, i);
            return heimdall_mutex_unlock( &p_input_item->lock ), 0;
        }
        p_slave->psz_uri = (char *) ((uint8_t *)p_slave) + sizeof(*p_slave);
        strcpy( p_slave->psz_uri, p_item_slave->psz_uri );

        switch( p_item_slave->i_type )
        {
        case SLAVE_TYPE_SPU:
            p_slave->i_type = libheimdall_media_slave_type_subtitle;
            break;
        case SLAVE_TYPE_GENERIC:
            p_slave->i_type = libheimdall_media_slave_type_generic;
            break;
        default:
            heimdall_assert_unreachable();
        }

        switch( p_item_slave->i_priority )
        {
        case SLAVE_PRIORITY_MATCH_NONE:
            p_slave->i_priority = 0;
            break;
        case SLAVE_PRIORITY_MATCH_RIGHT:
            p_slave->i_priority = 1;
            break;
        case SLAVE_PRIORITY_MATCH_LEFT:
            p_slave->i_priority = 2;
            break;
        case SLAVE_PRIORITY_MATCH_ALL:
            p_slave->i_priority = 3;
            break;
        case SLAVE_PRIORITY_USER:
            p_slave->i_priority = 4;
            break;
        default:
            heimdall_assert_unreachable();
        }
        pp_slaves[i] = p_slave;
    }
    heimdall_mutex_unlock( &p_input_item->lock );

    *ppp_slaves = pp_slaves;
    return i_count;
}

// Release a media descriptor's slave list
void libheimdall_media_slaves_release( libheimdall_media_slave_t **pp_slaves,
                                  unsigned int i_count )
{
    if( i_count > 0 )
    {
        assert( pp_slaves );
        for( unsigned int i = 0; i < i_count; ++i )
            free( pp_slaves[i] );
    }
    free( pp_slaves );
}
