/*****************************************************************************
 * media_list_player.c: libheimdall new API media_list player functions
 *****************************************************************************
 * Copyright (C) 2007-2015 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Pierre d'Herbemont <pdherbemont # videolan.org>
 *          Niles Bindel <zaggal69 # gmail.com>
 *          Rémi Denis-Courmont
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

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_renderer_discoverer.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>
#include <heimdall/libheimdall_media_list.h>
#include <heimdall/libheimdall_media_player.h>
#include <heimdall/libheimdall_media_list_player.h>
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_atomic.h>
#include <heimdall_vector.h>

#include "libheimdall_internal.h"

#include "media_internal.h" // Abuse, could and should be removed
#include "media_player_internal.h"
#include "media_list_path.h"

//#define DEBUG_MEDIA_LIST_PLAYER

/* This is a very dummy implementation of playlist on top of
 * media_list and media_player.
 *
 * All this code is doing is simply computing the next item
 * of a tree of media_list (see get_next_index()), and play
 * the next item when the current is over.
 */

struct libheimdall_media_list_player_t
{
    bool                        dead;
    libheimdall_media_list_path_t    current_playing_item_path;
    libheimdall_media_t *            p_current_playing_item;
    libheimdall_media_list_t *       p_mlist;
    libheimdall_media_player_t *     p_mi;
    libheimdall_playback_mode_t      e_playback_mode;

    heimdall_player_listener_id *    internal_listener;

    heimdall_atomic_rc_t             rc;
};

/*
 * Forward declaration
 */

static
int set_relative_playlist_position_and_play(libheimdall_media_list_player_t *p_mlp,
                                            bool next);
static void stop(libheimdall_media_list_player_t * p_mlp);

/*
 * Private functions
 */

/**************************************************************************
 * Shortcuts
 **************************************************************************/
static inline void lock(libheimdall_media_list_player_t * p_mlp)
{
    // Obtain an access to this structure
    heimdall_player_Lock(p_mlp->p_mi->player);
}

static inline void unlock(libheimdall_media_list_player_t * p_mlp)
{
    heimdall_player_Unlock(p_mlp->p_mi->player);
}

/**************************************************************************
 *       get_next_path (private)
 *
 *  Returns the path to the next item in the list.
 *  If looping is specified and the current item is the last list item in
 *  the list it will return the first item in the list.
 **************************************************************************/
static libheimdall_media_list_path_t
get_next_path(libheimdall_media_list_player_t * p_mlp, bool b_loop)
{
    /* We are entered with libheimdall_media_list_lock(p_mlp->p_list) */
    libheimdall_media_list_path_t ret;
    libheimdall_media_list_t * p_parent_of_playing_item;
    libheimdall_media_list_t * p_sublist_of_playing_item;

    if (!p_mlp->current_playing_item_path)
    {
        if (!libheimdall_media_list_count(p_mlp->p_mlist))
            return NULL;
        return libheimdall_media_list_path_with_root_index(0);
    }

    p_sublist_of_playing_item = libheimdall_media_list_sublist_at_path(
                            p_mlp->p_mlist,
                            p_mlp->current_playing_item_path);

    /* If item just gained a sublist just play it */
    if (p_sublist_of_playing_item)
    {
        int i_count = libheimdall_media_list_count(p_sublist_of_playing_item);
        libheimdall_media_list_release(p_sublist_of_playing_item);
        if (i_count > 0)
            return libheimdall_media_list_path_copy_by_appending(p_mlp->current_playing_item_path, 0);
    }

    /* Try to catch parent element */
    p_parent_of_playing_item = libheimdall_media_list_parentlist_at_path(p_mlp->p_mlist,
                            p_mlp->current_playing_item_path);

    int depth = libheimdall_media_list_path_depth(p_mlp->current_playing_item_path);
    if (depth < 1 || !p_parent_of_playing_item)
    {
        if (p_parent_of_playing_item)
            libheimdall_media_list_release(p_parent_of_playing_item);
        return NULL;
    }

    ret = libheimdall_media_list_path_copy(p_mlp->current_playing_item_path);
    ret[depth - 1]++; /* set to next element */

    /* If this goes beyond the end of the list */
    while(ret[depth-1] >= libheimdall_media_list_count(p_parent_of_playing_item))
    {
        depth--;
        if (depth <= 0)
        {
            if(b_loop)
            {
                ret[0] = 0;
                ret[1] = -1;
                break;
            }
            else
            {
                free(ret);
                libheimdall_media_list_release(p_parent_of_playing_item);
                return NULL;
            }
        }
        ret[depth] = -1;
        ret[depth-1]++;
        libheimdall_media_list_release(p_parent_of_playing_item);
        p_parent_of_playing_item  = libheimdall_media_list_parentlist_at_path(
                                        p_mlp->p_mlist,
                                        ret);
    }

    libheimdall_media_list_release(p_parent_of_playing_item);
    return ret;
}

/**************************************************************************
 *       find_last_item (private)
 *
 *  Returns the path of the last descendant of a given item path.
 *  Note: Due to the recursive nature of the function and the need to free
 *        media list paths, paths passed in may be freed if they are replaced.
          Recommended usage is to set return value to the same path that was
          passed to the function (i.e. item = find_last_item(list, item); )
 **************************************************************************/
static libheimdall_media_list_path_t
find_last_item( libheimdall_media_list_t * p_mlist, libheimdall_media_list_path_t current_item )
{
    libheimdall_media_list_t * p_sublist = libheimdall_media_list_sublist_at_path(p_mlist, current_item);
    libheimdall_media_list_path_t last_item_path = current_item;

    if(p_sublist)
    {
        int i_count = libheimdall_media_list_count(p_sublist);
        if(i_count > 0)
        {
            /* Add the last sublist item to the path. */
            last_item_path = libheimdall_media_list_path_copy_by_appending(current_item, i_count - 1);
            free(current_item);
            /* Check that sublist item for more descendants. */
            last_item_path = find_last_item(p_mlist, last_item_path);
        }

        libheimdall_media_list_release(p_sublist);
    }

    return last_item_path;
}

/**************************************************************************
 *       get_previous_path (private)
 *
 *  Returns the path to the preceding item in the list.
 *  If looping is specified and the current item is the first list item in
 *  the list it will return the last descendant of the last item in the list.
 **************************************************************************/
static libheimdall_media_list_path_t
get_previous_path(libheimdall_media_list_player_t * p_mlp, bool b_loop)
{
    /* We are entered with libheimdall_media_list_lock(p_mlp->p_list) */
    libheimdall_media_list_path_t ret;
    libheimdall_media_list_t * p_parent_of_playing_item;

    if (!p_mlp->current_playing_item_path)
    {
        if (!libheimdall_media_list_count(p_mlp->p_mlist))
            return NULL;
        return libheimdall_media_list_path_with_root_index(0);
    }

    /* Try to catch parent element */
    p_parent_of_playing_item = libheimdall_media_list_parentlist_at_path(
                                            p_mlp->p_mlist,
                                            p_mlp->current_playing_item_path);

    int depth = libheimdall_media_list_path_depth(p_mlp->current_playing_item_path);
    if (depth < 1 || !p_parent_of_playing_item)
        return NULL;

    /* Set the return path to the current path */
    ret = libheimdall_media_list_path_copy(p_mlp->current_playing_item_path);

    /* Change the return path to the previous list entry */
    ret[depth - 1]--; /* set to previous element */
    ret[depth] = -1;

    /* Is the return path is beyond the start of the current list? */
    if(ret[depth - 1] < 0)
    {
        /* Move to parent of current item */
        depth--;

        /* Are we at the root level of the tree? */
        if (depth <= 0)
        {
            // Is looping enabled?
            if(b_loop)
            {
                int i_count = libheimdall_media_list_count(p_parent_of_playing_item);

                /* Set current play item to the last element in the list */
                ret[0] = i_count - 1;
                ret[1] = -1;

                /* Set the return path to the last descendant item of the current item */
                ret = find_last_item(p_mlp->p_mlist, ret);
            }
            else
            {
                /* No looping so return empty path. */
                free(ret);
                ret = NULL;
            }
        }
        else
        {
            /* This is the case of moving backward from the beginning of the
            *  subitem list to its parent item.
            *  This ensures that current path is properly terminated to
            *  use that parent.
            */
            ret[depth] = -1;
        }
    }
    else
    {
        ret = find_last_item(p_mlp->p_mlist, ret);
    }

    libheimdall_media_list_release(p_parent_of_playing_item);
    return ret;
}

/**************************************************************************
 *       set_current_playing_item (private)
 *
 * Playlist lock should be held
 **************************************************************************/
static libheimdall_media_t *
set_current_playing_item(libheimdall_media_list_player_t * p_mlp, libheimdall_media_list_path_t path)
{
    /* First, save the new path that we are going to play */
    if (p_mlp->current_playing_item_path != path)
    {
        free(p_mlp->current_playing_item_path);
        p_mlp->current_playing_item_path = path;
    }

    if (!path)
        return NULL;

    return libheimdall_media_list_item_at_path(p_mlp->p_mlist, path);
}

/**************************************************************************
 *       find_next_media_locked (private)
 *
 * Find the media that should play after p_mlp->current_playing_item_path,
 * according to the current playback mode.
 *
 * Playlist lock must be held.
 **************************************************************************/
static libheimdall_media_t *
find_next_media_locked(libheimdall_media_list_player_t * p_mlp)
{
    libheimdall_media_list_path_t path;
    if (p_mlp->e_playback_mode != libheimdall_playback_mode_repeat)
    {
        bool b_loop = (p_mlp->e_playback_mode == libheimdall_playback_mode_loop);
        path = get_next_path(p_mlp, b_loop);
    }
    else
        path = libheimdall_media_list_path_copy(p_mlp->current_playing_item_path);

    libheimdall_media_t *md = NULL;
    if (path != NULL)
    {
        md = libheimdall_media_list_item_at_path(p_mlp->p_mlist, path);
        free(path);
    }
    return md;
}

static bool
path_equals(const libheimdall_media_list_path_t a, const libheimdall_media_list_path_t b)
{
    int depth = libheimdall_media_list_path_depth(a);
    return depth == libheimdall_media_list_path_depth(b)
        && memcmp(a, b, depth * sizeof(*a)) == 0;
}

typedef struct HEIMDALL_VECTOR(libheimdall_media_t *) media_vector_t;

/**************************************************************************
 *       append_next_media_locked (private)
 *
 * Append the media following p_mlp->current_playing_item_path to
 * following, in playback order, until the end of the list or until
 * reaching stop_path. current_playing_item_path is used as the cursor and
 * is left NULL or pointing to stop_path.
 *
 * Playlist lock must be held.
 **************************************************************************/
static void
append_next_media_locked(libheimdall_media_list_player_t * p_mlp,
                         media_vector_t *following,
                         const libheimdall_media_list_path_t stop_path)
{
    for (;;)
    {
        libheimdall_media_list_path_t path = get_next_path(p_mlp, false);
        free(p_mlp->current_playing_item_path);
        p_mlp->current_playing_item_path = path;

        if (path == NULL || (stop_path != NULL && path_equals(path, stop_path)))
            return;

        libheimdall_media_t *md = libheimdall_media_list_item_at_path(p_mlp->p_mlist, path);
        if (md != NULL && !heimdall_vector_push(following, md))
            libheimdall_media_release(md);
    }
}

/**************************************************************************
 *       get_following_media_locked (private)
 *
 * Collect the media that would play after p_mlp->current_playing_item_path,
 * in playback order, wrapping around in loop mode.
 *
 * Playlist lock must be held.
 **************************************************************************/
static void
get_following_media_locked(libheimdall_media_list_player_t * p_mlp,
                           media_vector_t *following)
{
    libheimdall_media_list_path_t current = p_mlp->current_playing_item_path;

    p_mlp->current_playing_item_path = libheimdall_media_list_path_copy(current);
    append_next_media_locked(p_mlp, following, NULL);

    if (p_mlp->e_playback_mode == libheimdall_playback_mode_loop)
    {
        /* Start over from the first item, up to the current one */
        free(p_mlp->current_playing_item_path);
        p_mlp->current_playing_item_path = NULL;
        append_next_media_locked(p_mlp, following, current);
    }

    free(p_mlp->current_playing_item_path);
    p_mlp->current_playing_item_path = current;
}

static void
internal_player_media_changed(heimdall_player_t *player, input_item_t *new_media,
                              void *opaque)
{
    (void) player;
    libheimdall_media_list_player_t *p_mlp = opaque;

    libheimdall_media_list_lock(p_mlp->p_mlist);

    /* Update the internal current path from the new media */
    libheimdall_media_list_path_t path =
        libheimdall_media_list_path_of_item(p_mlp->p_mlist,
                                      input_item_GetLibheimdallOwner(new_media));
    if (p_mlp->current_playing_item_path != path)
    {
        free(p_mlp->current_playing_item_path);
        p_mlp->current_playing_item_path = path;
    }

    /* Find and set the next media */
    libheimdall_media_t *md = find_next_media_locked(p_mlp);

    libheimdall_media_list_unlock(p_mlp->p_mlist);

    libheimdall_media_player_set_next_media(p_mlp->p_mi, md);
    libheimdall_media_release(md);
}

static void
internal_player_subitems_changed(heimdall_player_t *player, input_item_t *media,
                                 const input_item_node_t *new_subitems, void *opaque)
{
    (void) new_subitems;
    internal_player_media_changed(player, media, opaque);
}


/*
 * Public libheimdall functions
 */

/**************************************************************************
 *         new (Public)
 **************************************************************************/
libheimdall_media_list_player_t *
libheimdall_media_list_player_new(libheimdall_instance_t * p_instance,
                             const struct libheimdall_media_player_cbs *cbs,
                             void *cbs_opaque)
{
    libheimdall_media_list_player_t * p_mlp;
    p_mlp = calloc( 1, sizeof(libheimdall_media_list_player_t) );
    if (unlikely(p_mlp == NULL))
    {
        libheimdall_printerr("Not enough memory");
        return NULL;
    }

    heimdall_atomic_rc_init(&p_mlp->rc);
    p_mlp->dead = false;

    /* Create the underlying media_player */
    p_mlp->p_mi = libheimdall_media_player_new(p_instance, cbs, cbs_opaque);
    if( p_mlp->p_mi == NULL )
        goto error;

    heimdall_player_t *player = p_mlp->p_mi->player;
    static const struct heimdall_player_cbs internal_players_cbs =
    {
        .on_current_media_changed = internal_player_media_changed,
        .on_media_subitems_changed = internal_player_subitems_changed,
    };

    heimdall_player_Lock(player);
    p_mlp->internal_listener =
        heimdall_player_AddListener(player, &internal_players_cbs, p_mlp);
    heimdall_player_Unlock(player);

    return p_mlp;
error:
    free(p_mlp);
    return NULL;
}

/**************************************************************************
 *         release (Public)
 **************************************************************************/
void libheimdall_media_list_player_release(libheimdall_media_list_player_t * p_mlp)
{
    if (!p_mlp)
        return;

    if (!heimdall_atomic_rc_dec(&p_mlp->rc))
        return;

    heimdall_player_t *player = p_mlp->p_mi->player;
    heimdall_player_Lock(player);
    heimdall_player_RemoveListener(player, p_mlp->internal_listener);
    heimdall_player_Unlock(player);

    libheimdall_media_player_release(p_mlp->p_mi);

    if (p_mlp->p_mlist)
        libheimdall_media_list_release(p_mlp->p_mlist);

    free(p_mlp->current_playing_item_path);
    free(p_mlp);
}

/**************************************************************************
 *        retain (Public)
 **************************************************************************/
libheimdall_media_list_player_t *libheimdall_media_list_player_retain(libheimdall_media_list_player_t * p_mlp)
{
    assert(p_mlp);
    heimdall_atomic_rc_inc(&p_mlp->rc);
    return p_mlp;
}

/**************************************************************************
 *        get_media_player (Public)
 **************************************************************************/
libheimdall_media_player_t * libheimdall_media_list_player_get_media_player(libheimdall_media_list_player_t * p_mlp)
{
    lock(p_mlp);
    libheimdall_media_player_t *p_mi = p_mlp->p_mi;
    libheimdall_media_player_retain(p_mi);
    unlock(p_mlp);
    return p_mi;
}

/**************************************************************************
 *       set_media_list (Public)
 **************************************************************************/
void libheimdall_media_list_player_set_media_list(libheimdall_media_list_player_t * p_mlp, libheimdall_media_list_t * p_mlist)
{
    assert (p_mlist);

    lock(p_mlp);

    libheimdall_media_t *p_current_media = p_mlp->current_playing_item_path
        ? libheimdall_media_player_get_media(p_mlp->p_mi) : NULL;

    /* If the current media isn't part of the new list, playback continues
     * with the first media that followed it in the old list and is part of
     * the new one: collect them before the old list goes away. */
    media_vector_t following = HEIMDALL_VECTOR_INITIALIZER;
    if (p_current_media)
    {
        libheimdall_media_list_lock(p_mlist);
        libheimdall_media_list_path_t path =
            libheimdall_media_list_path_of_item(p_mlist, p_current_media);
        libheimdall_media_list_unlock(p_mlist);

        if (path == NULL)
        {
            libheimdall_media_list_lock(p_mlp->p_mlist);
            get_following_media_locked(p_mlp, &following);
            libheimdall_media_list_unlock(p_mlp->p_mlist);
        }
        free(path);
    }

    if (p_mlp->p_mlist)
        libheimdall_media_list_release(p_mlp->p_mlist);
    libheimdall_media_list_retain(p_mlist);

    p_mlp->p_mlist = p_mlist;

    if (p_current_media)
    {
        libheimdall_media_list_lock(p_mlp->p_mlist);

        libheimdall_media_t *p_following_media = NULL;
        libheimdall_media_list_path_t path =
            libheimdall_media_list_path_of_item(p_mlp->p_mlist, p_current_media);
        for (size_t i = 0; path == NULL && i < following.size; i++)
        {
            p_following_media = following.data[i];
            path = libheimdall_media_list_path_of_item(p_mlp->p_mlist,
                                                  p_following_media);
        }

        free(p_mlp->current_playing_item_path);
        p_mlp->current_playing_item_path = path;

        if (path == NULL)
        {
            /* Nothing left to play from the new list */
            libheimdall_media_list_unlock(p_mlp->p_mlist);
            stop(p_mlp);
        }
        else if (p_following_media == NULL)
        {
            /* The current media keeps playing, queue its new next media */
            libheimdall_media_t *md = find_next_media_locked(p_mlp);

            libheimdall_media_list_unlock(p_mlp->p_mlist);

            libheimdall_media_player_set_next_media(p_mlp->p_mi, md);
            libheimdall_media_release(md);
        }
        else
        {
            /* The current media was removed, switch to its follower */
            libheimdall_media_list_unlock(p_mlp->p_mlist);

            bool b_playing = libheimdall_media_player_is_playing(p_mlp->p_mi);
            libheimdall_media_player_set_media(p_mlp->p_mi, p_following_media);
            if (b_playing)
                libheimdall_media_player_play(p_mlp->p_mi);
        }

        libheimdall_media_release(p_current_media);
    }

    libheimdall_media_t *md;
    heimdall_vector_foreach(md, &following)
        libheimdall_media_release(md);
    heimdall_vector_destroy(&following);

    unlock(p_mlp);
}

/**************************************************************************
 *        Play (Public)
 **************************************************************************/
void libheimdall_media_list_player_play(libheimdall_media_list_player_t * p_mlp)
{
    lock(p_mlp);
    if (!p_mlp->current_playing_item_path)
    {
        set_relative_playlist_position_and_play(p_mlp, true);
        unlock(p_mlp);
        return; /* Will set to play */
    }
    libheimdall_media_player_play(p_mlp->p_mi);
    unlock(p_mlp);
}


/**************************************************************************
 *        Pause (Public)
 **************************************************************************/
void libheimdall_media_list_player_pause(libheimdall_media_list_player_t * p_mlp)
{
    lock(p_mlp);
    libheimdall_media_player_pause(p_mlp->p_mi);
    unlock(p_mlp);
}

void libheimdall_media_list_player_set_pause(libheimdall_media_list_player_t * p_mlp,
                                        int do_pause)
{
    lock(p_mlp);
    libheimdall_media_player_set_pause(p_mlp->p_mi, do_pause);
    unlock(p_mlp);
}

/**************************************************************************
 *        is_playing (Public)
 **************************************************************************/
bool libheimdall_media_list_player_is_playing(libheimdall_media_list_player_t * p_mlp)
{
    libheimdall_state_t state = libheimdall_media_player_get_state(p_mlp->p_mi);
    return (state == libheimdall_Opening) || (state == libheimdall_Playing);
}

/**************************************************************************
 *        State (Public)
 **************************************************************************/
libheimdall_state_t
libheimdall_media_list_player_get_state(libheimdall_media_list_player_t * p_mlp)
{
    return libheimdall_media_player_get_state(p_mlp->p_mi);
}

/**************************************************************************
 *        Play item at index (Public)
 **************************************************************************/
int libheimdall_media_list_player_play_item_at_index(libheimdall_media_list_player_t * p_mlp, int i_index)
{
    libheimdall_media_list_lock(p_mlp->p_mlist);
    libheimdall_media_list_path_t path = libheimdall_media_list_path_with_root_index(i_index);
    libheimdall_media_t *new_media = set_current_playing_item(p_mlp, path);
    libheimdall_media_list_unlock(p_mlp->p_mlist);

    libheimdall_media_player_set_media(p_mlp->p_mi, new_media);
    libheimdall_media_release(new_media);
    libheimdall_media_player_play(p_mlp->p_mi);

    return new_media ? 0 : -1;
}

/**************************************************************************
 *        Play item (Public)
 **************************************************************************/
int libheimdall_media_list_player_play_item(libheimdall_media_list_player_t * p_mlp, libheimdall_media_t * p_md)
{
    libheimdall_media_list_lock(p_mlp->p_mlist);
    libheimdall_media_list_path_t path = libheimdall_media_list_path_of_item(p_mlp->p_mlist, p_md);
    if (!path)
    {
        libheimdall_printerr("Item not found in media list");
        libheimdall_media_list_unlock(p_mlp->p_mlist);
        return -1;
    }

    libheimdall_media_t *new_media = set_current_playing_item(p_mlp, path);
    libheimdall_media_list_unlock(p_mlp->p_mlist);

    libheimdall_media_player_set_media(p_mlp->p_mi, new_media);
    libheimdall_media_release(new_media);
    libheimdall_media_player_play(p_mlp->p_mi);

    return new_media ? 0 : -1;
}

/**************************************************************************
 *       Stop (Private)
 *
 * Lock must be held.
 **************************************************************************/
static void stop(libheimdall_media_list_player_t * p_mlp)
{
    libheimdall_media_player_stop_async(p_mlp->p_mi);

    free(p_mlp->current_playing_item_path);
    p_mlp->current_playing_item_path = NULL;
}

/**************************************************************************
 *       Stop (Public)
 **************************************************************************/
void libheimdall_media_list_player_stop_async(libheimdall_media_list_player_t * p_mlp)
{
    lock(p_mlp);
    stop(p_mlp);
    unlock(p_mlp);
}

/**************************************************************************
 *       Set relative playlist position and play (Private)
 *
 * Sets the currently played item to the given relative play item position
 * (based on the currently playing item) and then begins the new item playback.
 * Lock must be held.
 **************************************************************************/
static int set_relative_playlist_position_and_play(
                                      libheimdall_media_list_player_t * p_mlp,
                                      bool next)
{
    if (!p_mlp->p_mlist)
    {
        libheimdall_printerr("No media list");
        return -1;
    }

    libheimdall_media_list_lock(p_mlp->p_mlist);

    libheimdall_media_list_path_t path = p_mlp->current_playing_item_path;
    libheimdall_media_t *new_media;

    if(p_mlp->e_playback_mode != libheimdall_playback_mode_repeat)
    {
        bool b_loop = (p_mlp->e_playback_mode == libheimdall_playback_mode_loop);

        if (next)
        {
            path = get_next_path(p_mlp, b_loop);
            new_media = set_current_playing_item(p_mlp, path);
        }
        else
        {
            path = get_previous_path(p_mlp, b_loop);
            new_media = set_current_playing_item(p_mlp, path);
        }
    }
    else
    {
        new_media = set_current_playing_item(p_mlp, path);
    }

#ifdef DEBUG_MEDIA_LIST_PLAYER
    printf("Playing:");
    libheimdall_media_list_path_dump(path);
#endif

    libheimdall_media_list_unlock(p_mlp->p_mlist);

    if (!new_media)
        return -1;

    libheimdall_media_player_set_media(p_mlp->p_mi, new_media);
    libheimdall_media_release(new_media);
    libheimdall_media_player_play(p_mlp->p_mi);

    return 0;
}

/**************************************************************************
 *       Next (Public)
 **************************************************************************/
int libheimdall_media_list_player_next(libheimdall_media_list_player_t * p_mlp)
{
    lock(p_mlp);
    int failure = set_relative_playlist_position_and_play(p_mlp, true);
    unlock(p_mlp);
    return failure;
}

/**************************************************************************
 *       Previous (Public)
 **************************************************************************/
int libheimdall_media_list_player_previous(libheimdall_media_list_player_t * p_mlp)
{
    lock(p_mlp);
    int failure = set_relative_playlist_position_and_play(p_mlp, false);
    unlock(p_mlp);
    return failure;
}

/**************************************************************************
 *       Set Playback Mode (Public)
 **************************************************************************/
void libheimdall_media_list_player_set_playback_mode(
                                            libheimdall_media_list_player_t * p_mlp,
                                            libheimdall_playback_mode_t e_mode )
{
    lock(p_mlp);
    p_mlp->e_playback_mode = e_mode;
    unlock(p_mlp);
}
