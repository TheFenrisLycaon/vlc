/*****************************************************************************
 * media_list_internal.h : Definition of opaque structures for libheimdall exported API
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

#ifndef _LIBHEIMDALL_MEDIA_LIST_INTERNAL_H
#define _LIBHEIMDALL_MEDIA_LIST_INTERNAL_H 1

#include <heimdall/heimdall.h>
#include <heimdall/libheimdall_media.h>

#include <heimdall_common.h>
#include <heimdall_arrays.h>

struct libheimdall_media_list_t
{
    heimdall_mutex_t                 object_lock;
    heimdall_atomic_rc_t             rc;
    libheimdall_media_t * p_md; /* The media from which the
                                       * mlist comes, if any. */
    libheimdall_media_t * p_internal_md; /* media set from media.c */
    heimdall_array_t                items;

    /* This indicates if this media list is read-only
     * from a user point of view */
    bool                  b_read_only;
};

/* Media List */
void libheimdall_media_list_internal_clear( libheimdall_media_list_t *p_mlist );

void libheimdall_media_list_internal_add_media(
        libheimdall_media_list_t * p_mlist,
        libheimdall_media_t * p_md );

void libheimdall_media_list_internal_insert_media(
        libheimdall_media_list_t * p_mlist,
        libheimdall_media_t * p_md, int index );

int libheimdall_media_list_internal_remove_index(
        libheimdall_media_list_t * p_mlist, int index );

#endif
