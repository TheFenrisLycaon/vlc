/*****************************************************************************
 * media_list.c: libheimdall new API media list functions
 *****************************************************************************
 * Copyright (C) 2007 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Pierre d'Herbemont <pdherbemont # videolan.org>
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

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>
#include <heimdall/libheimdall_media_list.h>

#include <heimdall_common.h>
#include <heimdall_atomic.h>

#include "libheimdall_internal.h"
#include "media_internal.h" // libheimdall_media_new_from_input_item()
#include "media_list_internal.h"

/*
 * Private functions
 */

/**************************************************************************
 *       static mlist_is_writable (private)
 **************************************************************************/
static inline
bool mlist_is_writable( libheimdall_media_list_t *p_mlist )
{
    if( !p_mlist||p_mlist->b_read_only )
    {
        /* We are read-only from user side */
        libheimdall_printerr( "Attempt to write a read-only media list" );
        return false;
    }
    return true;
}

/*
 * Public libheimdall functions
 */

/**************************************************************************
 *       libheimdall_media_list_new (Public)
 *
 * Init an object.
 **************************************************************************/
libheimdall_media_list_t *libheimdall_media_list_new(void)
{
    libheimdall_media_list_t * p_mlist;

    p_mlist = malloc(sizeof(libheimdall_media_list_t));
    if( unlikely(p_mlist == NULL) )
    {
        libheimdall_printerr( "Not enough memory" );
        return NULL;
    }

    p_mlist->b_read_only = false;

    heimdall_mutex_init( &p_mlist->object_lock );
    heimdall_atomic_rc_init( &p_mlist->rc );

    heimdall_array_init( &p_mlist->items );
    assert( p_mlist->items.i_count == 0 );
    p_mlist->p_md = NULL;
    p_mlist->p_internal_md = NULL;

    return p_mlist;
}

void libheimdall_media_list_internal_clear( libheimdall_media_list_t *p_mlist )
{
    for( size_t i = 0; i < heimdall_array_count( &p_mlist->items ); i++ )
    {
        libheimdall_media_t* p_md = heimdall_array_item_at_index( &p_mlist->items, i );
        libheimdall_media_release( p_md );
    }

    heimdall_array_clear( &p_mlist->items );
}

/**************************************************************************
 *       libheimdall_media_list_release (Public)
 *
 * Release an object.
 **************************************************************************/
void libheimdall_media_list_release( libheimdall_media_list_t * p_mlist )
{
    if( !heimdall_atomic_rc_dec( &p_mlist->rc ) )
        return;

    /* Refcount null, time to free */

    libheimdall_media_release( p_mlist->p_md );

    libheimdall_media_list_internal_clear( p_mlist );

    free( p_mlist );
}

/**************************************************************************
 *       libheimdall_media_list_retain (Public)
 *
 * Increase an object refcount.
 **************************************************************************/
libheimdall_media_list_t *libheimdall_media_list_retain( libheimdall_media_list_t * p_mlist )
{
    heimdall_atomic_rc_inc( &p_mlist->rc );
    return p_mlist;
}

/**************************************************************************
 *       set_media (Public)
 **************************************************************************/
void libheimdall_media_list_set_media( libheimdall_media_list_t * p_mlist,
                                  libheimdall_media_t * p_md )

{
    heimdall_mutex_lock( &p_mlist->object_lock );
    if( p_mlist->p_internal_md || !mlist_is_writable(p_mlist) )
    {
        heimdall_mutex_unlock( &p_mlist->object_lock );
        return;
    }
    libheimdall_media_release( p_mlist->p_md );
    libheimdall_media_retain( p_md );
    p_mlist->p_md = p_md;
    heimdall_mutex_unlock( &p_mlist->object_lock );
}

/**************************************************************************
 *       media (Public)
 *
 * If this media_list comes is a media's subitems,
 * This holds the corresponding media.
 * This md is also seen as the information holder for the media_list.
 * Indeed a media_list can have meta information through this
 * media.
 **************************************************************************/
libheimdall_media_t *
libheimdall_media_list_media( libheimdall_media_list_t * p_mlist )
{
    libheimdall_media_t *p_md;

    heimdall_mutex_lock( &p_mlist->object_lock );
    p_md = p_mlist->p_internal_md ? p_mlist->p_internal_md : p_mlist->p_md;
    if( p_md )
        libheimdall_media_retain( p_md );
    heimdall_mutex_unlock( &p_mlist->object_lock );

    return p_md;
}

/**************************************************************************
 *       libheimdall_media_list_count (Public)
 *
 * Lock should be held when entering.
 **************************************************************************/
int libheimdall_media_list_count( libheimdall_media_list_t * p_mlist )
{
    return heimdall_array_count( &p_mlist->items );
}

/**************************************************************************
 *       libheimdall_media_list_add_media (Public)
 *
 * Lock should be held when entering.
 **************************************************************************/
int libheimdall_media_list_add_media( libheimdall_media_list_t * p_mlist,
                                 libheimdall_media_t * p_md )
{
    if( !mlist_is_writable(p_mlist) )
        return -1;
    libheimdall_media_list_internal_add_media( p_mlist, p_md );
    return 0;
}

/* LibHeimdall internal version */
void libheimdall_media_list_internal_add_media( libheimdall_media_list_t * p_mlist,
                                           libheimdall_media_t * p_md )
{
    libheimdall_media_retain( p_md );

    heimdall_array_append_or_abort( &p_mlist->items, p_md );
}

/**************************************************************************
 *       libheimdall_media_list_insert_media (Public)
 *
 * Lock should be hold when entering.
 **************************************************************************/
int libheimdall_media_list_insert_media( libheimdall_media_list_t * p_mlist,
                                    libheimdall_media_t * p_md,
                                    int index )
{
    if( !mlist_is_writable(p_mlist) )
        return -1;
    libheimdall_media_list_internal_insert_media( p_mlist, p_md, index );
    return 0;
}

/* LibHeimdall internal version */
void libheimdall_media_list_internal_insert_media( libheimdall_media_list_t * p_mlist,
                                              libheimdall_media_t * p_md,
                                              int index )
{
    libheimdall_media_retain( p_md );

    heimdall_array_insert_or_abort( &p_mlist->items, p_md, index );
}

/**************************************************************************
 *       libheimdall_media_list_remove_index (Public)
 *
 * Lock should be held when entering.
 **************************************************************************/
int libheimdall_media_list_remove_index( libheimdall_media_list_t * p_mlist,
                                     int index )
{
    if( !mlist_is_writable(p_mlist) )
        return -1;
    return libheimdall_media_list_internal_remove_index( p_mlist, index );
}

/* LibHeimdall internal version */
int libheimdall_media_list_internal_remove_index( libheimdall_media_list_t * p_mlist,
                                             int index )
{
    libheimdall_media_t * p_md;

    if( (size_t) index >= heimdall_array_count( &p_mlist->items ))
    {
        libheimdall_printerr( "Index out of bounds" );
        return -1;
    }

    p_md = heimdall_array_item_at_index( &p_mlist->items, index );

    heimdall_array_remove( &p_mlist->items, index );

    libheimdall_media_release( p_md );
    return 0;
}

/**************************************************************************
 *       libheimdall_media_list_item_at_index (Public)
 *
 * Lock should be held when entering.
 **************************************************************************/
libheimdall_media_t *
libheimdall_media_list_item_at_index( libheimdall_media_list_t * p_mlist,
                                 int index )
{
    libheimdall_media_t * p_md;

    if( (size_t) index >= heimdall_array_count( &p_mlist->items ))
    {
        libheimdall_printerr( "Index out of bounds" );
        return NULL;
    }

    p_md = heimdall_array_item_at_index( &p_mlist->items, index );
    libheimdall_media_retain( p_md );
    return p_md;
}

/**************************************************************************
 *       libheimdall_media_list_index_of_item (Public)
 *
 * Lock should be held when entering.
 * Warning: this function returns the first matching item.
 **************************************************************************/
int libheimdall_media_list_index_of_item( libheimdall_media_list_t * p_mlist,
                                     libheimdall_media_t * p_searched_md )
{
    int idx = heimdall_array_index_of_item( &p_mlist->items, p_searched_md );
    if( idx == -1 )
        libheimdall_printerr( "Media not found" );

    return idx;
}

/**************************************************************************
 *       libheimdall_media_list_is_readonly (Public)
 *
 * This indicates if this media list is read-only from a user point of view
 **************************************************************************/
bool libheimdall_media_list_is_readonly( libheimdall_media_list_t * p_mlist )
{
    return p_mlist->b_read_only;
}

/**************************************************************************
 *       libheimdall_media_list_lock (Public)
 *
 * The lock must be held in access operations. It is never used in the
 * Public method.
 **************************************************************************/
void libheimdall_media_list_lock( libheimdall_media_list_t * p_mlist )
{
    heimdall_mutex_lock( &p_mlist->object_lock );
}


/**************************************************************************
 *       libheimdall_media_list_unlock (Public)
 *
 * The lock must be held in access operations
 **************************************************************************/
void libheimdall_media_list_unlock( libheimdall_media_list_t * p_mlist )
{
    heimdall_mutex_unlock( &p_mlist->object_lock );
}
