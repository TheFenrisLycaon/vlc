/*****************************************************************************
 * libheimdall_media_list_player.h:  libheimdall_media_list API
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

#ifndef LIBHEIMDALL_MEDIA_LIST_PLAYER_H
#define LIBHEIMDALL_MEDIA_LIST_PLAYER_H 1

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_media_player.h>

# ifdef __cplusplus
extern "C" {
# endif

typedef struct libheimdall_instance_t libheimdall_instance_t;
typedef struct libheimdall_media_player_t libheimdall_media_player_t;
typedef struct libheimdall_media_list_t libheimdall_media_list_t;
typedef struct libheimdall_media_t libheimdall_media_t;

/** \defgroup libheimdall_media_list_player LibHeimdall media list player
 * \ingroup libheimdall
 * The LibHeimdall media list player plays a @ref libheimdall_media_list_t list of media,
 * in a certain order.
 * This is required to especially support playlist files.
 * The normal @ref libheimdall_media_player_t LibHeimdall media player can only play a
 * single media, and does not handle playlist files properly.
 * @{
 * \file
 * LibHeimdall media list player external API
 */

typedef struct libheimdall_media_list_player_t libheimdall_media_list_player_t;

/**
 *  Defines playback modes for playlist.
 */
typedef enum libheimdall_playback_mode_t
{
    libheimdall_playback_mode_default,
    libheimdall_playback_mode_loop,
    libheimdall_playback_mode_repeat
} libheimdall_playback_mode_t;

/**
 * Create new media_list_player.
 *
 * \param p_instance libheimdall instance
 * \param cbs callback to listen to events (can be NULL). The pointed
 * struct must be kept alive (and not modified) by the caller until
 * the returned media list player is released.
 * \param cbs_opaque opaque pointer used by the callbacks
 * \return media list player instance or NULL on error
 *         (it must be released by libheimdall_media_list_player_release())
 */
LIBHEIMDALL_API libheimdall_media_list_player_t *
    libheimdall_media_list_player_new( libheimdall_instance_t * p_instance,
                                  const struct libheimdall_media_player_cbs *cbs,
                                  void *cbs_opaque );

/**
 * Release a media_list_player after use
 * Decrement the reference count of a media list player object. If the
 * reference count is 0, then libheimdall_media_list_player_release() will
 * release the media list player object. If the media list player object
 * has been released, then it should not be used again.
 *
 * \param p_mlp media list player instance
 */
LIBHEIMDALL_API void
    libheimdall_media_list_player_release( libheimdall_media_list_player_t * p_mlp );

/**
 * Retain a reference to a media player list object. Use
 * libheimdall_media_list_player_release() to decrement reference count.
 *
 * \param p_mlp media player list object
 * \return the same object
 */
LIBHEIMDALL_API libheimdall_media_list_player_t *
    libheimdall_media_list_player_retain( libheimdall_media_list_player_t *p_mlp );

/**
 * Get media player of the media_list_player instance.
 *
 * \param p_mlp media list player instance
 * \return media player instance
 * \note the caller is responsible for releasing the returned instance
 *       with libheimdall_media_player_release().
 */
LIBHEIMDALL_API libheimdall_media_player_t *
    libheimdall_media_list_player_get_media_player(libheimdall_media_list_player_t * p_mlp);

/**
 * Set the media list associated with the player
 *
 * If the current media is part of the new list, it keeps playing and the
 * playback continues from its position in the new list. Otherwise, the
 * playback switches to the first media that followed it in the previous
 * list and is part of the new one, or stops if there is none.
 *
 * \param p_mlp media list player instance
 * \param p_mlist list of media
 *
 * \note the caller must not hold p_mlist's lock
 *       (libheimdall_media_list_lock()) when calling this function.
 */
LIBHEIMDALL_API void
    libheimdall_media_list_player_set_media_list(
                                     libheimdall_media_list_player_t * p_mlp,
                                     libheimdall_media_list_t * p_mlist );

/**
 * Play media list
 *
 * \param p_mlp media list player instance
 */
LIBHEIMDALL_API
void libheimdall_media_list_player_play(libheimdall_media_list_player_t * p_mlp);

/**
 * Toggle pause (or resume) media list
 *
 * \param p_mlp media list player instance
 */
LIBHEIMDALL_API
void libheimdall_media_list_player_pause(libheimdall_media_list_player_t * p_mlp);

/**
 * Pause or resume media list
 *
 * \param p_mlp media list player instance
 * \param do_pause play/resume if zero, pause if non-zero
 * \version LibHeimdall 3.0.0 or later
 */
LIBHEIMDALL_API
void libheimdall_media_list_player_set_pause(libheimdall_media_list_player_t * p_mlp,
                                        int do_pause);

/**
 * Is media list playing?
 *
 * \param p_mlp media list player instance
 *
 * \retval true playing
 * \retval false not playing
 */
LIBHEIMDALL_API bool
libheimdall_media_list_player_is_playing(libheimdall_media_list_player_t * p_mlp);

/**
 * Get current libheimdall_state of media list player
 *
 * \param p_mlp media list player instance
 * \return libheimdall_state_t for media list player
 */
LIBHEIMDALL_API libheimdall_state_t
    libheimdall_media_list_player_get_state( libheimdall_media_list_player_t * p_mlp );

/**
 * Play media list item at position index
 *
 * \param p_mlp media list player instance
 * \param i_index index in media list to play
 * \return 0 upon success -1 if the item wasn't found
 */
LIBHEIMDALL_API
int libheimdall_media_list_player_play_item_at_index(libheimdall_media_list_player_t * p_mlp,
                                                int i_index);

/**
 * Play the given media item
 *
 * \param p_mlp media list player instance
 * \param p_md the media instance
 * \return 0 upon success, -1 if the media is not part of the media list
 */
LIBHEIMDALL_API
int libheimdall_media_list_player_play_item(libheimdall_media_list_player_t * p_mlp,
                                       libheimdall_media_t * p_md);

/**
 * Stop playing media list
 *
 * \param p_mlp media list player instance
 */
LIBHEIMDALL_API void
    libheimdall_media_list_player_stop_async( libheimdall_media_list_player_t * p_mlp);

/**
 * Play next item from media list
 *
 * \param p_mlp media list player instance
 * \return 0 upon success -1 if there is no next item
 */
LIBHEIMDALL_API
int libheimdall_media_list_player_next(libheimdall_media_list_player_t * p_mlp);

/**
 * Play previous item from media list
 *
 * \param p_mlp media list player instance
 * \return 0 upon success -1 if there is no previous item
 */
LIBHEIMDALL_API
int libheimdall_media_list_player_previous(libheimdall_media_list_player_t * p_mlp);



/**
 * Sets the playback mode for the playlist
 *
 * \param p_mlp media list player instance
 * \param e_mode playback mode specification
 */
LIBHEIMDALL_API
void libheimdall_media_list_player_set_playback_mode(libheimdall_media_list_player_t * p_mlp,
                                                libheimdall_playback_mode_t e_mode );

/** @} media_list_player */

# ifdef __cplusplus
}
# endif

#endif /* LIBHEIMDALL_MEDIA_LIST_PLAYER_H */
