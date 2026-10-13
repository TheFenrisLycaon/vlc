/*****************************************************************************
 * heimdall_playlist_export.h
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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

#ifndef HEIMDALL_PLAYLIST_EXPORT_H
#define HEIMDALL_PLAYLIST_EXPORT_H

#include <heimdall_playlist.h>

/** API for playlist export modules */

/**
 * Opaque structure giving a read-only view of a playlist.
 *
 * The view is only valid until the playlist lock is released.
 */
struct heimdall_playlist_view;

/**
 * Return the number of items in the view.
 *
 * The underlying playlist must be locked.
 *
 * \param view the playlist view
 */
HEIMDALL_API size_t
heimdall_playlist_view_Count(struct heimdall_playlist_view *view);

/**
 * Return the item at a given index.
 *
 * The index must be in range (less than heimdall_playlist_view_Count()).
 *
 * The underlying playlist must be locked.
 *
 * \param view  the playlist view
 * \param index the index
 * \return the playlist item
 */
HEIMDALL_API heimdall_playlist_item_t *
heimdall_playlist_view_Get(struct heimdall_playlist_view *view, size_t index);

/**
 * Structure received by playlist export module.
 */
struct heimdall_playlist_export
{
    struct heimdall_object_t obj;
    char *base_url;
    FILE *file;
    struct heimdall_playlist_view *playlist_view;
};

#endif
