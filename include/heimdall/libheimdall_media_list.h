/*****************************************************************************
 * libheimdall_media_list.h:  libheimdall_media_list API
 *****************************************************************************
 * Copyright (C) 1998-2008 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Pierre d'Herbemont
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

#ifndef LIBHEIMDALL_MEDIA_LIST_H
#define LIBHEIMDALL_MEDIA_LIST_H 1

typedef struct libheimdall_media_t libheimdall_media_t;

# ifdef __cplusplus
extern "C" {
# endif

/** \defgroup libheimdall_media_list LibHeimdall media list
 * \ingroup libheimdall
 * A LibHeimdall media list holds multiple @ref libheimdall_media_t media descriptors.
 * @{
 * \file
 * LibHeimdall media list (playlist) external API
 */

typedef struct libheimdall_media_list_t libheimdall_media_list_t;

/**
 * Create an empty media list.
 *
 * \return empty media list, or NULL on error
 */
LIBHEIMDALL_API libheimdall_media_list_t *libheimdall_media_list_new(void);

/**
 * Release media list created with libheimdall_media_list_new().
 *
 * \param p_ml a media list created with libheimdall_media_list_new()
 */
LIBHEIMDALL_API void
    libheimdall_media_list_release( libheimdall_media_list_t *p_ml );

/**
 * Retain reference to a media list
 *
 * \param p_ml a media list created with libheimdall_media_list_new()
 * \return the same object
 */
LIBHEIMDALL_API libheimdall_media_list_t *
    libheimdall_media_list_retain( libheimdall_media_list_t *p_ml );

/**
 * Associate media instance with this media list instance.
 * If another media instance was present it will be released.
 * The libheimdall_media_list_lock should NOT be held upon entering this function.
 *
 * \param p_ml a media list instance
 * \param p_md media instance to add
 */
LIBHEIMDALL_API void
libheimdall_media_list_set_media( libheimdall_media_list_t *p_ml, libheimdall_media_t *p_md );

/**
 * Get media instance from this media list instance. This action will increase
 * the refcount on the media instance.
 * The libheimdall_media_list_lock should NOT be held upon entering this function.
 *
 * \param p_ml a media list instance
 * \return media instance
 */
LIBHEIMDALL_API libheimdall_media_t *
    libheimdall_media_list_media( libheimdall_media_list_t *p_ml );

/**
 * Add media instance to media list
 * The libheimdall_media_list_lock should be held upon entering this function.
 *
 * \param p_ml a media list instance
 * \param p_md a media instance
 * \return 0 on success, -1 if the media list is read-only
 */
LIBHEIMDALL_API int
libheimdall_media_list_add_media( libheimdall_media_list_t *p_ml, libheimdall_media_t *p_md );

/**
 * Insert media instance in media list on a position
 * The libheimdall_media_list_lock should be held upon entering this function.
 *
 * \param p_ml a media list instance
 * \param p_md a media instance
 * \param i_pos position in array where to insert
 * \return 0 on success, -1 if the media list is read-only
 */
LIBHEIMDALL_API int
libheimdall_media_list_insert_media( libheimdall_media_list_t *p_ml,
                                libheimdall_media_t *p_md, int i_pos );

/**
 * Remove media instance from media list on a position
 * The libheimdall_media_list_lock should be held upon entering this function.
 *
 * \param p_ml a media list instance
 * \param i_pos position in array where to insert
 * \return 0 on success, -1 if the list is read-only or the item was not found
 */
LIBHEIMDALL_API int
libheimdall_media_list_remove_index( libheimdall_media_list_t *p_ml, int i_pos );

/**
 * Get count on media list items
 * The libheimdall_media_list_lock should be held upon entering this function.
 *
 * \param p_ml a media list instance
 * \return number of items in media list
 */
LIBHEIMDALL_API int
    libheimdall_media_list_count( libheimdall_media_list_t *p_ml );

/**
 * List media instance in media list at a position
 * The libheimdall_media_list_lock should be held upon entering this function.
 *
 * \param p_ml a media list instance
 * \param i_pos position in array where to insert
 * \return media instance at position i_pos, or NULL if not found.
 * In case of success, libheimdall_media_retain() is called to increase the refcount
 * on the media.
 */
LIBHEIMDALL_API libheimdall_media_t *
    libheimdall_media_list_item_at_index( libheimdall_media_list_t *p_ml, int i_pos );
/**
 * Find index position of List media instance in media list.
 * Warning: the function will return the first matched position.
 * The libheimdall_media_list_lock should be held upon entering this function.
 *
 * \param p_ml a media list instance
 * \param p_md media instance
 * \return position of media instance or -1 if media not found
 */
LIBHEIMDALL_API int
    libheimdall_media_list_index_of_item( libheimdall_media_list_t *p_ml,
                                     libheimdall_media_t *p_md );

/**
 * This indicates if this media list is read-only from a user point of view
 *
 * \param p_ml media list instance
 * \retval true read-only
 * \retval false read/write
 */
LIBHEIMDALL_API bool libheimdall_media_list_is_readonly(libheimdall_media_list_t *p_ml);

/**
 * Get lock on media list items
 *
 * \param p_ml a media list instance
 */
LIBHEIMDALL_API void
    libheimdall_media_list_lock( libheimdall_media_list_t *p_ml );

/**
 * Release lock on media list items
 * The libheimdall_media_list_lock should be held upon entering this function.
 *
 * \param p_ml a media list instance
 */
LIBHEIMDALL_API void
    libheimdall_media_list_unlock( libheimdall_media_list_t *p_ml );

/** @} media_list */

# ifdef __cplusplus
}
# endif

#endif /* _LIBHEIMDALL_MEDIA_LIST_H */
