// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * media_player.c: Libheimdall API Media Instance management functions
 *****************************************************************************
 * Copyright (C) 2005-2015 VLC authors and VideoLAN
 * Copyright (C) 2020-2026 Alexandre Janniaux <ajanni@videolabs.io>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <stdckdint.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_renderer_discoverer.h>
#include <heimdall/libheimdall_picture.h>
#include <heimdall/libheimdall_media.h>

#include <heimdall_demux.h>
#include <heimdall_vout.h>
#include <heimdall_aout.h>
#include <heimdall_subpicture.h>
#include <heimdall_actions.h>
#include <heimdall_modules.h>

#include "libheimdall_internal.h"
#include "media_player_internal.h"
#include "renderer_discoverer_internal.h"
#include "picture_internal.h"

#define ES_INIT (-2) /* -1 is reserved for ES deselect */

static int
snapshot_was_taken( heimdall_object_t *p_this, char const *psz_cmd,
                    heimdall_value_t oldval, heimdall_value_t newval, void *p_data );

static void libheimdall_media_player_destroy( libheimdall_media_player_t *p_mi );

static libheimdall_state_t
libheimdall_state_from_player(enum heimdall_player_state state)
{
    switch (state) {
        case HEIMDALL_PLAYER_STATE_STOPPED:
            return libheimdall_Stopped;
        case HEIMDALL_PLAYER_STATE_STOPPING:
            return libheimdall_Stopping;
        case HEIMDALL_PLAYER_STATE_STARTED:
            return libheimdall_Opening;
        case HEIMDALL_PLAYER_STATE_PLAYING:
            return libheimdall_Playing;
        case HEIMDALL_PLAYER_STATE_PAUSED:
            return libheimdall_Paused;
        default:
            heimdall_assert_unreachable();
    }
}

// player callbacks

static void
on_current_media_changed(heimdall_player_t *player, input_item_t *new_media,
                         void *data)
{
    (void) player;
    libheimdall_media_player_t *mp = data;

    libheimdall_media_t *libmedia;
    if (new_media != NULL)
    {
        libmedia = input_item_GetLibheimdallOwner(new_media);
        assert(libmedia != NULL);
    }
    else
        libmedia = NULL;

    if (mp->cbs == NULL || mp->cbs->on_media_changed == NULL)
        return;

    mp->cbs->on_media_changed(mp->cbs_opaque, libmedia);
}

static void
on_stopping_current_media(heimdall_player_t *player, input_item_t *item,
                          enum heimdall_player_media_stopping_reason stopping_reason,
                          void *data)
{
    assert(item != NULL);
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_media_stopping == NULL)
        return;

    libheimdall_media_t *media = input_item_GetLibheimdallOwner(item);
    assert(media != NULL);

    static_assert((int) HEIMDALL_PLAYER_MEDIA_STOPPING_ERROR == libheimdall_stopping_reason_error &&
                  (int) HEIMDALL_PLAYER_MEDIA_STOPPING_EOS == libheimdall_stopping_reason_eos &&
                  (int) HEIMDALL_PLAYER_MEDIA_STOPPING_USER == libheimdall_stopping_reason_user,
                  "libheimdall_stopping_reason_t mismatch");

    mp->cbs->on_media_stopping(mp->cbs_opaque, media, (int) stopping_reason);
}

static void
on_state_changed(heimdall_player_t *player, enum heimdall_player_state new_state,
                 void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_state_changed == NULL)
        return;

    mp->cbs->on_state_changed(mp->cbs_opaque,
                              libheimdall_state_from_player(new_state));
}

static void
on_recording_changed(heimdall_player_t *player, bool recording, void *data)
{
    (void) player;
    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_recording_changed == NULL)
        return;

    /* "record-file" is only valid when recording ends */
    char *file_path = !recording ? var_GetString(mp, "record-file") : NULL;

    mp->cbs->on_recording_changed(mp->cbs_opaque, recording, file_path);

    free(file_path);
}

static void
on_error_changed(heimdall_player_t *player, enum heimdall_player_error error, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_state_changed == NULL)
        return;

    libheimdall_state_t state;
    switch (error) {
        case HEIMDALL_PLAYER_ERROR_NONE:
            state = libheimdall_state_from_player(heimdall_player_GetState(player));
            break;
        case HEIMDALL_PLAYER_ERROR_GENERIC:
            state = libheimdall_Error;
            break;
        default:
            heimdall_assert_unreachable();
    }

    mp->cbs->on_state_changed(mp->cbs_opaque, state);
}

static void
on_buffering_changed(heimdall_player_t *player, float new_buffering, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_buffering_changed == NULL)
        return;

    mp->cbs->on_buffering_changed(mp->cbs_opaque, new_buffering);
}

static void
on_rate_changed(heimdall_player_t *player, float new_rate, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_rate_changed == NULL)
        return;

    mp->cbs->on_rate_changed(mp->cbs_opaque, new_rate);
}

static void
on_capabilities_changed(heimdall_player_t *player, int old_caps, int new_caps, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_capabilities_changed == NULL)
        return;

    static_assert(HEIMDALL_PLAYER_CAP_SEEK == libheimdall_capability_seek &&
                  HEIMDALL_PLAYER_CAP_PAUSE == libheimdall_capability_pause &&
                  HEIMDALL_PLAYER_CAP_CHANGE_RATE == libheimdall_capability_change_rate &&
                  HEIMDALL_PLAYER_CAP_REWIND == libheimdall_capability_rewind,
                  "libheimdall_capability_t mismatch");

    mp->cbs->on_capabilities_changed(mp->cbs_opaque, old_caps, new_caps);
}

static void
on_position_changed(heimdall_player_t *player, heimdall_tick_t new_time, double new_pos,
                    void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_position_changed == NULL)
        return;

    mp->cbs->on_position_changed(mp->cbs_opaque, libheimdall_time_from_heimdall_tick(new_time),
                                 new_pos);
}

static void
on_length_changed(heimdall_player_t *player, heimdall_tick_t new_length, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_length_changed == NULL)
        return;

    mp->cbs->on_length_changed(mp->cbs_opaque, libheimdall_time_from_heimdall_tick(new_length));
}

static int
track_type_from_cat(enum es_format_category_e cat)
{
    switch (cat)
    {
        case VIDEO_ES:
            return libheimdall_track_video;
        case AUDIO_ES:
            return libheimdall_track_audio;
        case SPU_ES:
            return libheimdall_track_text;
        default:
            return libheimdall_track_unknown;
    }
}

static void
on_track_list_changed(heimdall_player_t *player, enum heimdall_player_list_action action,
                      const struct heimdall_player_track *track, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_track_list_changed == NULL)
        return;

    static_assert((int) HEIMDALL_PLAYER_LIST_ADDED == libheimdall_list_action_added &&
                  (int) HEIMDALL_PLAYER_LIST_REMOVED == libheimdall_list_action_removed &&
                  (int) HEIMDALL_PLAYER_LIST_UPDATED == libheimdall_list_action_updated,
                  "libheimdall_list_action_t mismatch");

    mp->cbs->on_track_list_changed(mp->cbs_opaque, (int) action,
                                   track_type_from_cat(track->fmt.i_cat),
                                   heimdall_es_id_GetStrId(track->es_id));
}

static void
on_track_selection_changed(heimdall_player_t *player, heimdall_es_id_t *unselected_id,
                           heimdall_es_id_t *selected_id, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_track_selection_changed == NULL)
        return;

    assert(unselected_id != NULL || selected_id != NULL);

    libheimdall_track_type_t type = libheimdall_track_unknown;
    const char *unselected_str_id = NULL, *selected_str_id = NULL;

    if (unselected_id != NULL)
    {
        type = track_type_from_cat(heimdall_es_id_GetCat(unselected_id));
        unselected_str_id = heimdall_es_id_GetStrId(unselected_id);
    }
    if (selected_id != NULL)
    {
        type = track_type_from_cat(heimdall_es_id_GetCat(selected_id));
        selected_str_id = heimdall_es_id_GetStrId(selected_id);
    }

    if (type != libheimdall_track_unknown)
        mp->cbs->on_track_selection_changed(mp->cbs_opaque, type,
                                            unselected_str_id, selected_str_id);
}

static void
on_program_list_changed(heimdall_player_t *player,
                        enum heimdall_player_list_action action,
                        const struct heimdall_player_program *prgm, void* data)
{
    (void) player;
    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_program_list_changed == NULL)
        return;

    mp->cbs->on_program_list_changed(mp->cbs_opaque, (int) action, prgm->group_id);
}

static void
on_program_selection_changed(heimdall_player_t *player, int unselected_id,
                             int selected_id, void *data)
{
    (void) player;
    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_program_selection_changed == NULL)
        return;

    mp->cbs->on_program_selection_changed(mp->cbs_opaque, unselected_id,
                                          selected_id);
}

static void
on_titles_changed(heimdall_player_t *player,
                  heimdall_player_title_list *titles, void *data)
{
    (void) player;
    (void) titles;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_titles_changed == NULL)
        return;

    mp->cbs->on_titles_changed(mp->cbs_opaque);
}

#define LIBHEIMDALL_TITLE_FROM_HEIMDALL(title) { \
    .i_duration = libheimdall_time_from_heimdall_tick(title->length), \
    .psz_name = (char *) title->name, \
    .i_flags = title->flags, \
}

#define LIBHEIMDALL_CHAPTER_FROM_HEIMDALL(chapter) { \
    .i_time_offset = libheimdall_time_from_heimdall_tick(chapter->time), \
    .i_duration = 0, \
    .psz_name = (char *) chapter->name, \
}

static void
on_title_selection_changed(heimdall_player_t *player,
                           const struct heimdall_player_title *new_title,
                           size_t new_idx, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_title_selection_changed == NULL)
        return;

    const libheimdall_title_description_t libtitle = LIBHEIMDALL_TITLE_FROM_HEIMDALL(new_title);

    mp->cbs->on_title_selection_changed(mp->cbs_opaque, &libtitle, new_idx);
}

static void
on_chapter_selection_changed(heimdall_player_t *player,
                             const struct heimdall_player_title *title,
                             size_t title_idx,
                             const struct heimdall_player_chapter *new_chapter,
                             size_t new_chapter_idx,
                             void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_chapter_selection_changed == NULL)
        return;

    const libheimdall_title_description_t libtitle = LIBHEIMDALL_TITLE_FROM_HEIMDALL(title);
    const libheimdall_chapter_description_t libchapter = LIBHEIMDALL_CHAPTER_FROM_HEIMDALL(new_chapter);

    mp->cbs->on_chapter_selection_changed(mp->cbs_opaque, &libtitle, title_idx,
                                          &libchapter, new_chapter_idx);
}

static void on_media_meta_changed(heimdall_player_t *player,
                                  input_item_t *media, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    assert(media != NULL);
    libheimdall_media_t *libmedia = input_item_GetLibheimdallOwner(media);
    assert(libmedia != NULL);

    libheimdall_media_parsed_status_t expected = libheimdall_media_parsed_status_none;
    if (atomic_compare_exchange_strong(&libmedia->parsed_status, &expected,
                                       libheimdall_media_parsed_status_done)
     && mp->cbs != NULL && mp->cbs->on_media_parsed != NULL)
        mp->cbs->on_media_parsed(mp->cbs_opaque, libmedia);

    if (mp->cbs != NULL && mp->cbs->on_media_meta_changed != NULL)
        mp->cbs->on_media_meta_changed(mp->cbs_opaque, libmedia);
}

static void
on_media_subitems_changed(heimdall_player_t *player, input_item_t *media,
                          const input_item_node_t *new_subitems, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    assert(media != NULL);
    libheimdall_media_t *libmedia = input_item_GetLibheimdallOwner(media);
    assert(libmedia != NULL);

    libheimdall_media_add_subtree(libmedia, new_subitems);
    if (mp->cbs != NULL && mp->cbs->on_media_subitems_changed != NULL)
        mp->cbs->on_media_subitems_changed(mp->cbs_opaque, libmedia);
}

static void
on_media_attachments_added(heimdall_player_t *player, input_item_t *item,
                           input_attachment_t *const *array, size_t count,
                           void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;
    assert(item == NULL || input_item_GetLibheimdallOwner(item) != NULL);

    if (mp->cbs != NULL && mp->cbs->on_media_attachments_added != NULL)
    {
        libheimdall_media_t *p_md = item != NULL
                             ? input_item_GetLibheimdallOwner(item) : NULL;
        libheimdall_picture_list_t *list =
            libheimdall_picture_list_from_attachments(array, count);
        if (list == NULL)
            return;
        if (libheimdall_picture_list_count(list) == 0)
        {
            libheimdall_picture_list_destroy(list);
            return;
        }
        mp->cbs->on_media_attachments_added(mp->cbs_opaque, p_md, list);
        libheimdall_picture_list_destroy(list);
    }
}

static void
on_cork_changed(heimdall_player_t *player, unsigned cork_count, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_cork_changed == NULL)
        return;

    if (cork_count > 1)
        return; /* already notified */

    mp->cbs->on_cork_changed(mp->cbs_opaque, cork_count == 1);
}

static void
on_vout_changed(heimdall_player_t *player, enum heimdall_player_vout_action action,
                vout_thread_t *vout, enum heimdall_vout_order order,
                heimdall_es_id_t *es_id, void *data)
{
    (void) action;
    (void) vout;
    (void) order;

    if (heimdall_es_id_GetCat(es_id) != VIDEO_ES)
        return;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_vout_changed == NULL)
        return;

    size_t count;
    vout_thread_t **vouts = heimdall_player_vout_HoldAll(player, &count);
    if (!vouts)
        return;
    for (size_t i = 0; i < count; ++i)
        vout_Release(vouts[i]);
    free(vouts);

    mp->cbs->on_vout_changed(mp->cbs_opaque, count);
}

static void
on_next_frame_status(heimdall_player_t *player, int status, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_next_frame_status == NULL)
        return;

    mp->cbs->on_next_frame_status(mp->cbs_opaque, status);
}

static void
on_prev_frame_status(heimdall_player_t *player, int status, void *data)
{
    (void) player;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_prev_frame_status == NULL)
        return;

    mp->cbs->on_prev_frame_status(mp->cbs_opaque, status);
}

// player aout callbacks

static void
on_volume_changed(audio_output_t *aout, float new_volume, void *data)
{
    (void) aout;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_audio_volume_changed == NULL)
        return;

    mp->cbs->on_audio_volume_changed(mp->cbs_opaque, new_volume);
}

static void
on_mute_changed(audio_output_t *aout, bool new_muted, void *data)
{
    (void) aout;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_audio_mute_changed == NULL)
        return;

    mp->cbs->on_audio_mute_changed(mp->cbs_opaque, new_muted);
}

static void
on_audio_device_changed(audio_output_t *aout, const char *device, void *data)
{
    (void) aout;

    libheimdall_media_player_t *mp = data;

    if (mp->cbs == NULL || mp->cbs->on_audio_device_changed == NULL)
        return;

    mp->cbs->on_audio_device_changed(mp->cbs_opaque, device);
}

static const struct heimdall_player_cbs heimdall_player_cbs = {
    .on_current_media_changed = on_current_media_changed,
    .on_state_changed = on_state_changed,
    .on_error_changed = on_error_changed,
    .on_buffering_changed = on_buffering_changed,
    .on_rate_changed = on_rate_changed,
    .on_capabilities_changed = on_capabilities_changed,
    .on_position_changed = on_position_changed,
    .on_length_changed = on_length_changed,
    .on_track_list_changed = on_track_list_changed,
    .on_track_selection_changed = on_track_selection_changed,
    .on_program_list_changed = on_program_list_changed,
    .on_program_selection_changed = on_program_selection_changed,
    .on_titles_changed = on_titles_changed,
    .on_title_selection_changed = on_title_selection_changed,
    .on_chapter_selection_changed = on_chapter_selection_changed,
    .on_media_meta_changed = on_media_meta_changed,
    .on_media_subitems_changed = on_media_subitems_changed,
    .on_media_attachments_added = on_media_attachments_added,
    .on_cork_changed = on_cork_changed,
    .on_vout_changed = on_vout_changed,
    .on_recording_changed = on_recording_changed,
    .on_stopping_current_media = on_stopping_current_media,
    .on_next_frame_status = on_next_frame_status,
    .on_prev_frame_status = on_prev_frame_status,
};

static const struct heimdall_player_aout_cbs heimdall_player_aout_cbs = {
    .on_volume_changed = on_volume_changed,
    .on_mute_changed = on_mute_changed,
    .on_device_changed = on_audio_device_changed,
};

/**************************************************************************
 * Snapshot Taken Event.
 *
 * FIXME: This snapshot API interface makes no sense in media_player.
 *************************************************************************/
static int snapshot_was_taken(heimdall_object_t *p_this, char const *psz_cmd,
                              heimdall_value_t oldval, heimdall_value_t newval, void *p_data )
{
    HEIMDALL_UNUSED(psz_cmd); HEIMDALL_UNUSED(oldval); HEIMDALL_UNUSED(p_this);

    libheimdall_media_player_t *mp = p_data;

    mp->cbs->on_screenshot_taken(mp->cbs_opaque, newval.psz_string);

    return HEIMDALL_SUCCESS;
}

/**************************************************************************
 * Create a Media Instance object.
 *
 * Refcount strategy:
 * - All items created by _new start with a refcount set to 1.
 * - Accessor _release decrease the refcount by 1, if after that
 *   operation the refcount is 0, the object is destroyed.
 * - Accessor _retain increase the refcount by 1 (XXX: to implement)
 *
 * Object locking strategy:
 * - No lock held while in constructor.
 * - When accessing any member variable this lock is held. (XXX who locks?)
 * - When attempting to destroy the object the lock is also held.
 **************************************************************************/
libheimdall_media_player_t *
libheimdall_media_player_new( libheimdall_instance_t *instance,
                         const struct libheimdall_media_player_cbs *cbs,
                         void *cbs_opaque )
{
    libheimdall_media_player_t * mp;
    int doinherit;

    /* No different versions to handle for now */
    if (cbs != NULL)
        assert(cbs->version <= 0);

    assert(instance);

    mp = heimdall_object_create (instance->p_libheimdall_int, sizeof(*mp));
    if (unlikely(mp == NULL))
    {
        libheimdall_printerr("Not enough memory");
        return NULL;
    }

    mp->cbs = cbs;
    mp->cbs_opaque = cbs_opaque;

    /* Input */
    var_Create (mp, "rate", HEIMDALL_VAR_FLOAT|HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "sout", HEIMDALL_VAR_STRING);
    var_Create (mp, "demux-filter", HEIMDALL_VAR_STRING);

    /* Video */
    var_Create (mp, "vout", HEIMDALL_VAR_STRING|HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "window", HEIMDALL_VAR_STRING);
    var_Create (mp, "gl", HEIMDALL_VAR_STRING);
    var_Create (mp, "gles2", HEIMDALL_VAR_STRING);
    var_Create (mp, "vmem-lock", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "vmem-unlock", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "vmem-display", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "vmem-data", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "vmem-setup", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "vmem-cleanup", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "vmem-chroma", HEIMDALL_VAR_STRING);
    var_Create (mp, "vmem-width", HEIMDALL_VAR_INTEGER);
    var_Create (mp, "vmem-height", HEIMDALL_VAR_INTEGER);
    var_Create (mp, "vmem-pitch", HEIMDALL_VAR_INTEGER);

    var_Create (mp, "vout-cb-type", HEIMDALL_VAR_INTEGER );
    var_Create( mp, "vout-cb-opaque", HEIMDALL_VAR_ADDRESS );
    var_Create( mp, "vout-cb-setup", HEIMDALL_VAR_ADDRESS );
    var_Create( mp, "vout-cb-cleanup", HEIMDALL_VAR_ADDRESS );
    var_Create( mp, "vout-cb-window-cb", HEIMDALL_VAR_ADDRESS );
    var_Create( mp, "vout-cb-update-output", HEIMDALL_VAR_ADDRESS );
    var_Create( mp, "vout-cb-swap", HEIMDALL_VAR_ADDRESS );
    var_Create( mp, "vout-cb-get-proc-address", HEIMDALL_VAR_ADDRESS );
    var_Create( mp, "vout-cb-make-current", HEIMDALL_VAR_ADDRESS );
    var_Create( mp, "vout-cb-metadata", HEIMDALL_VAR_ADDRESS );
    var_Create( mp, "vout-cb-select-plane", HEIMDALL_VAR_ADDRESS );

    var_Create (mp, "dec-dev", HEIMDALL_VAR_STRING);

    mp->vout.default_gl = var_GetString(mp, "gl");
    mp->vout.default_gles2 = var_GetString(mp, "gles2");
    mp->vout.default_vout = var_GetString(mp, "vout");
    mp->vout.default_dec_dev = var_GetString(mp, "dec-dev");

    var_Create (mp, "drawable-xid", HEIMDALL_VAR_INTEGER);
#if defined (_WIN32) || defined (__OS2__)
    var_Create (mp, "drawable-hwnd", HEIMDALL_VAR_INTEGER);
#endif
#ifdef __APPLE__
    var_Create (mp, "drawable-nsobject", HEIMDALL_VAR_ADDRESS);
#endif
#ifdef __ANDROID__
    var_Create (mp, "drawable-androidwindow", HEIMDALL_VAR_ADDRESS);
#endif

    var_Create (mp, "keyboard-events", HEIMDALL_VAR_BOOL);
    var_SetBool (mp, "keyboard-events", true);
    var_Create (mp, "mouse-events", HEIMDALL_VAR_BOOL);
    var_SetBool (mp, "mouse-events", true);

    var_Create (mp, "fullscreen", HEIMDALL_VAR_BOOL);
    var_Create (mp, "autoscale", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "fit", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "zoom", HEIMDALL_VAR_FLOAT | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "aspect-ratio", HEIMDALL_VAR_STRING);
    var_Create (mp, "crop", HEIMDALL_VAR_STRING);
    var_Create (mp, "deinterlace", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "deinterlace-mode", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "projection-mode", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "video-stereo-mode", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT);

    var_Create (mp, "vbi-page", HEIMDALL_VAR_INTEGER);
    var_SetInteger (mp, "vbi-page", 100);

    var_Create (mp, "video-filter", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "sub-source", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "sub-filter", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);

    var_Create (mp, "osd", HEIMDALL_VAR_BOOL); // off
    var_Create (mp, "spu-fill", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT);

    doinherit = module_exists("marq") ? HEIMDALL_VAR_DOINHERIT : 0;
    var_Create(mp, "marq-marquee", HEIMDALL_VAR_STRING);
    var_Create(mp, "marq-color", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "marq-opacity", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "marq-position", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "marq-refresh", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "marq-size", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "marq-timeout", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "marq-x", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "marq-y", HEIMDALL_VAR_INTEGER | doinherit);

    doinherit = module_exists("logo") ? HEIMDALL_VAR_DOINHERIT : 0;
    var_Create(mp, "logo-file", HEIMDALL_VAR_STRING);
    var_Create(mp, "logo-x", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "logo-y", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "logo-delay", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "logo-repeat", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "logo-opacity", HEIMDALL_VAR_INTEGER | doinherit);
    var_Create(mp, "logo-position", HEIMDALL_VAR_INTEGER | doinherit);

    var_Create(mp, "contrast", HEIMDALL_VAR_FLOAT);
    var_SetFloat(mp, "contrast", 1.f);
    var_Create(mp, "brightness", HEIMDALL_VAR_FLOAT);
    var_SetFloat(mp, "brightness", 1.f);
    var_Create(mp, "hue", HEIMDALL_VAR_FLOAT);
    var_Create(mp, "saturation", HEIMDALL_VAR_FLOAT);
    var_SetFloat(mp, "saturation", 1.f);
    var_Create(mp, "gamma", HEIMDALL_VAR_FLOAT);
    var_SetFloat(mp, "gamma", 1.f);

     /* Audio */
    var_Create (mp, "aout", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "audio-device", HEIMDALL_VAR_STRING);
    var_Create (mp, "mute", HEIMDALL_VAR_BOOL);
    var_Create (mp, "volume", HEIMDALL_VAR_FLOAT);
    var_Create (mp, "corks", HEIMDALL_VAR_INTEGER);
    var_Create (mp, "audio-filter", HEIMDALL_VAR_STRING);
    var_Create (mp, "role", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);
    var_Create (mp, "amem-data", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "amem-setup", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "amem-cleanup", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "amem-play", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "amem-pause", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "amem-resume", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "amem-flush", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "amem-drain", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "amem-set-volume", HEIMDALL_VAR_ADDRESS);
    var_Create (mp, "amem-format", HEIMDALL_VAR_STRING);
    var_Create (mp, "amem-rate", HEIMDALL_VAR_INTEGER);
    var_Create (mp, "amem-channels", HEIMDALL_VAR_INTEGER);

    /* Video Title */
    var_Create (mp, "video-title-show", HEIMDALL_VAR_BOOL);
    var_Create (mp, "video-title-position", HEIMDALL_VAR_INTEGER);
    var_Create (mp, "video-title-timeout", HEIMDALL_VAR_INTEGER);

    /* Equalizer */
    var_Create (mp, "equalizer-preamp", HEIMDALL_VAR_FLOAT);
    var_Create (mp, "equalizer-heimdallfreqs", HEIMDALL_VAR_BOOL);
    var_Create (mp, "equalizer-bands", HEIMDALL_VAR_STRING);
    var_Create (mp, "equalizer-preset", HEIMDALL_VAR_STRING);

    /* variables for signalling creation of new files */
    var_Create(mp, "record-file", HEIMDALL_VAR_STRING);

    mp->timer.id = NULL;
    mp->p_libheimdall_instance = instance;
    /* use a reentrant lock to allow calling libheimdall functions from callbacks */
    mp->player = heimdall_player_New(HEIMDALL_OBJECT(mp), HEIMDALL_PLAYER_LOCK_REENTRANT);
    if (unlikely(!mp->player))
        goto error1;
    heimdall_cond_init(&mp->wait);

    heimdall_player_Lock(mp->player);

    mp->listener = heimdall_player_AddListener(mp->player, &heimdall_player_cbs, mp);
    if (unlikely(!mp->listener))
        goto error2;

    mp->aout_listener =
        heimdall_player_aout_AddListener(mp->player, &heimdall_player_aout_cbs, mp);
    if (unlikely(!mp->aout_listener))
        goto error3;

    heimdall_player_Unlock(mp->player);

    heimdall_atomic_rc_init(&mp->rc);

    /* Snapshot initialization */
    /* Attach a var callback to the global object to provide the glue between
     * vout_thread that generates the event and media_player that re-emits it
     * to the user via the on_screenshot_taken callback.
     *
     * FIXME: It's unclear why we want to put this in public API, and why we
     * want to expose it in such a limiting and ugly way.
     */
    if (mp->cbs != NULL && mp->cbs->on_screenshot_taken != NULL)
        var_AddCallback(heimdall_object_instance(mp),
                        "snapshot-file", snapshot_was_taken, mp);

    libheimdall_retain(instance);
    return mp;

error3:
    heimdall_player_RemoveListener(mp->player, mp->listener);
error2:
    heimdall_player_Unlock(mp->player);
    heimdall_player_Delete(mp->player);
error1:
    heimdall_object_delete(mp);
    return NULL;
}

/**************************************************************************
 * Create a Media Instance object with a media descriptor.
 **************************************************************************/
libheimdall_media_player_t *
libheimdall_media_player_new_from_media( libheimdall_instance_t *inst,
                                    libheimdall_media_t * p_md,
                                    const struct libheimdall_media_player_cbs *cbs,
                                    void *cbs_opaque )
{
    libheimdall_media_player_t * p_mi;

    p_mi = libheimdall_media_player_new( inst, cbs, cbs_opaque );
    if( !p_mi )
        return NULL;

    heimdall_player_Lock(p_mi->player);
    int ret = heimdall_player_SetCurrentMedia(p_mi->player, p_md->p_input_item);
    heimdall_player_Unlock(p_mi->player);

    if (ret != HEIMDALL_SUCCESS)
    {
        libheimdall_media_player_release(p_mi);
        return NULL;
    }

    return p_mi;
}

/**************************************************************************
 * Destroy a Media Instance object (libheimdall internal)
 *
 * Warning: No lock held here, but hey, this is internal. Caller must lock.
 **************************************************************************/
static void libheimdall_media_player_destroy( libheimdall_media_player_t *p_mi )
{
    assert( p_mi );

    /* Detach Callback from the main libheimdall object */
    if (p_mi->cbs != NULL && p_mi->cbs->on_screenshot_taken != NULL)
        var_DelCallback( heimdall_object_instance(p_mi),
                         "snapshot-file", snapshot_was_taken, p_mi );

    heimdall_player_Lock(p_mi->player);
    heimdall_player_aout_RemoveListener(p_mi->player, p_mi->aout_listener);
    heimdall_player_RemoveListener(p_mi->player, p_mi->listener);
    heimdall_player_Unlock(p_mi->player);

    heimdall_player_Delete(p_mi->player);

    free(p_mi->vout.default_dec_dev);
    free(p_mi->vout.default_vout);
    free(p_mi->vout.default_gl);
    free(p_mi->vout.default_gles2);

    libheimdall_instance_t *instance = p_mi->p_libheimdall_instance;
    heimdall_object_delete(p_mi);
    libheimdall_release(instance);
}

/**************************************************************************
 * Release a Media Instance object.
 *
 * Function does the locking.
 **************************************************************************/
void libheimdall_media_player_release( libheimdall_media_player_t *p_mi )
{
    assert( p_mi );
    if( !heimdall_atomic_rc_dec( &p_mi->rc ) )
        return;

    libheimdall_media_player_destroy( p_mi );
}

void libheimdall_media_player_lock( libheimdall_media_player_t *mp )
{
    heimdall_player_Lock(mp->player);
}

void libheimdall_media_player_unlock( libheimdall_media_player_t *mp )
{
    heimdall_player_Unlock(mp->player);
}

void libheimdall_media_player_wait( libheimdall_media_player_t *mp )
{
    heimdall_player_CondWait(mp->player, &mp->wait);
}

void libheimdall_media_player_signal( libheimdall_media_player_t *mp )
{
    heimdall_cond_broadcast(&mp->wait);
}

/**************************************************************************
 * Retain a Media Instance object.
 *
 * Caller must hold the lock.
 **************************************************************************/
libheimdall_media_player_t *libheimdall_media_player_retain( libheimdall_media_player_t *p_mi )
{
    assert( p_mi );
    heimdall_atomic_rc_inc( &p_mi->rc );
    return p_mi;
}

/**************************************************************************
 * Set the Media descriptor associated with the instance.
 *
 * Enter without lock -- function will lock the object.
 **************************************************************************/
void libheimdall_media_player_set_media(
                            libheimdall_media_player_t *p_mi,
                            libheimdall_media_t *p_md )
{
    heimdall_player_Lock(p_mi->player);

    heimdall_player_SetCurrentMedia(p_mi->player, p_md ? p_md->p_input_item : NULL);

    heimdall_player_Unlock(p_mi->player);
}

/**************************************************************************
 * Get the Media descriptor associated with the instance.
 **************************************************************************/
libheimdall_media_t *
libheimdall_media_player_get_media( libheimdall_media_player_t *p_mi )
{
    heimdall_player_Lock(p_mi->player);

    input_item_t *item = heimdall_player_GetCurrentMedia(p_mi->player);
    libheimdall_media_t *p_m;
    if (item != NULL)
        p_m = input_item_GetLibheimdallOwner(input_item_Hold(item));
    else
        p_m = NULL;

    heimdall_player_Unlock(p_mi->player);

    return p_m;
}

void libheimdall_media_player_set_next_media( libheimdall_media_player_t *p_mi,
                                         libheimdall_media_t *p_md )
{
    heimdall_player_Lock(p_mi->player);

    heimdall_player_SetNextMedia(p_mi->player, p_md ? p_md->p_input_item : NULL);

    heimdall_player_Unlock(p_mi->player);
}

libheimdall_media_t *
libheimdall_media_player_get_next_media( libheimdall_media_player_t *p_mi )
{
    heimdall_player_Lock(p_mi->player);

    input_item_t *item = heimdall_player_GetNextMedia(p_mi->player);
    libheimdall_media_t *p_m;
    if (item != NULL)
        p_m = input_item_GetLibheimdallOwner(input_item_Hold(item));
    else
        p_m = NULL;

    heimdall_player_Unlock(p_mi->player);

    return p_m;
}

/**************************************************************************
 * Tell media player to start playing.
 **************************************************************************/
int libheimdall_media_player_play( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    int ret = heimdall_player_Start(player);
    if (ret == HEIMDALL_SUCCESS)
    {
        if (heimdall_player_IsPaused(player))
            heimdall_player_Resume(player);
    }

    heimdall_player_Unlock(player);
    return ret;
}

void libheimdall_media_player_set_pause( libheimdall_media_player_t *p_mi, int paused )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    if (paused)
    {
        if (heimdall_player_CanPause(player))
            heimdall_player_Pause(player);
        else
            heimdall_player_Stop(player);
    }
    else
    {
        heimdall_player_Resume(player);
    }

    heimdall_player_Unlock(player);
}

/**************************************************************************
 * Toggle pause.
 **************************************************************************/
void libheimdall_media_player_pause( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_TogglePause(player);

    heimdall_player_Unlock(player);
}

/**************************************************************************
 * Tells whether the media player is currently playing.
 **************************************************************************/
bool libheimdall_media_player_is_playing(libheimdall_media_player_t *p_mi)
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    bool ret = heimdall_player_IsStarted(player) && !heimdall_player_IsPaused(player);

    heimdall_player_Unlock(player);
    return ret;
}

/**************************************************************************
 * Stop playing.
 **************************************************************************/
int libheimdall_media_player_stop_async( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    int ret = heimdall_player_Stop(player);

    heimdall_player_Unlock(player);

    return ret;
}

int libheimdall_media_player_set_renderer( libheimdall_media_player_t *p_mi,
                                      libheimdall_renderer_item_t *p_litem )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_renderer_item_t *renderer = libheimdall_renderer_item_to_heimdall(p_litem);
    heimdall_player_SetRenderer(player, renderer);

    heimdall_player_Unlock(player);
    return 0;
}

static void libheimdall_media_player_switch_vout(libheimdall_media_player_t *player,
                                            libheimdall_media_player_vout_detach_cb vout_detach)
{
    if (player->vout.window_detach != NULL)
        player->vout.window_detach(player);
    player->vout.window_detach = vout_detach;
}

static void libheimdall_media_player_detach_video_callbacks(libheimdall_media_player_t *player)
{
    var_SetAddress(player, "vmem-lock", NULL);
    var_SetAddress(player, "vmem-unlock", NULL);
    var_SetAddress(player, "vmem-display", NULL);
    var_SetAddress(player, "vmem-data", NULL);
    var_SetString(player, "dec-dev", player->vout.default_dec_dev);
    var_SetString(player, "vout", player->vout.default_vout);
    var_SetString(player, "window", "any");
}

void libheimdall_video_set_callbacks( libheimdall_media_player_t *mp,
    void *(*lock_cb) (void *, void **),
    void (*unlock_cb) (void *, void *, void *const *),
    void (*display_cb) (void *, void *),
    void *opaque )
{
    libheimdall_media_player_switch_vout(mp, libheimdall_media_player_detach_video_callbacks);
    var_SetAddress( mp, "vmem-lock", lock_cb );
    var_SetAddress( mp, "vmem-unlock", unlock_cb );
    var_SetAddress( mp, "vmem-display", display_cb );
    var_SetAddress( mp, "vmem-data", opaque );
    var_SetString( mp, "dec-dev", "none" );
    var_SetString( mp, "vout", "vmem" );
    var_SetString( mp, "window", "dummy" );
}

void libheimdall_video_set_format_callbacks( libheimdall_media_player_t *mp,
                                        libheimdall_video_format_cb setup,
                                        libheimdall_video_cleanup_cb cleanup )
{
    var_SetAddress( mp, "vmem-setup", setup );
    var_SetAddress( mp, "vmem-cleanup", cleanup );
}

void libheimdall_video_set_format( libheimdall_media_player_t *mp, const char *chroma,
                              unsigned width, unsigned height, unsigned pitch )
{
    var_SetString( mp, "vmem-chroma", chroma );
    var_SetInteger( mp, "vmem-width", width );
    var_SetInteger( mp, "vmem-height", height );
    var_SetInteger( mp, "vmem-pitch", pitch );
}

static void libheimdall_media_player_detach_output_callbacks(libheimdall_media_player_t *player)
{
    var_SetString(player, "dec-dev", player->vout.default_dec_dev);
    var_SetString(player, "vout", player->vout.default_vout);
    var_SetString(player, "gl", player->vout.default_gl);
    var_SetString(player, "gles2", player->vout.default_gles2);
    var_SetString(player, "window", "any");

    var_SetInteger(player, "vout-cb-type", libheimdall_video_engine_disable);
    var_SetAddress(player, "vout-cb-opaque", NULL);
    var_SetAddress(player, "vout-cb-setup", NULL);
    var_SetAddress(player, "vout-cb-cleanup", NULL);
    var_SetAddress(player, "vout-cb-window-cb", NULL);
    var_SetAddress(player, "vout-cb-update-output", NULL);
    var_SetAddress(player, "vout-cb-swap", NULL);
    var_SetAddress(player, "vout-cb-get-proc-address", NULL);
    var_SetAddress(player, "vout-cb-make-current", NULL);
    var_SetAddress(player, "vout-cb-metadata", NULL);
    var_SetAddress(player, "vout-cb-select-plane", NULL);
}

bool libheimdall_video_set_output_callbacks(libheimdall_media_player_t *mp,
                                       libheimdall_video_engine_t engine,
                                       libheimdall_video_output_setup_cb setup_cb,
                                       libheimdall_video_output_cleanup_cb cleanup_cb,
                                       libheimdall_video_output_set_window_cb set_window_cb,
                                       libheimdall_video_update_output_cb update_output_cb,
                                       libheimdall_video_swap_cb swap_cb,
                                       libheimdall_video_makeCurrent_cb makeCurrent_cb,
                                       libheimdall_video_getProcAddress_cb getProcAddress_cb,
                                       libheimdall_video_frameMetadata_cb metadata_cb,
                                       libheimdall_video_output_select_plane_cb select_plane_cb,
                                       void *opaque)
{
    static_assert(libheimdall_video_engine_disable == 0, "No engine set must default to 0");


    if (engine == libheimdall_video_engine_disable)
    {
        libheimdall_media_player_switch_vout(mp, NULL);
        return true;
    }

    var_SetString( mp, "window", "wextern");

    if (engine == libheimdall_video_engine_gles2)
    {
        var_SetString(mp, "vout", "gles2");
        var_SetString(mp, "gles2", "vgl");
        libheimdall_media_player_switch_vout(mp, libheimdall_media_player_detach_output_callbacks);
    }
    else if (engine == libheimdall_video_engine_opengl)
    {
        var_SetString(mp, "vout", "gl");
        var_SetString(mp, "gl", "vgl");
        libheimdall_media_player_switch_vout(mp, libheimdall_media_player_detach_output_callbacks);
    }
    else if (engine == libheimdall_video_engine_d3d11)
    {
        var_SetString(mp, "vout", "d3d11drawable");
        var_SetString(mp, "dec-dev", "d3d11");
        libheimdall_media_player_switch_vout(mp, libheimdall_media_player_detach_output_callbacks);
    }
    else if (engine == libheimdall_video_engine_d3d9)
    {
        var_SetString(mp, "vout", "direct3d9");
        var_SetString(mp, "dec-dev", "d3d9");
        libheimdall_media_player_switch_vout(mp, libheimdall_media_player_detach_output_callbacks);
    }
    else if (engine == libheimdall_video_engine_anw)
    {
        /* Force android-display is using MediaCodec or fallback to GL (any) */
        var_SetString(mp, "vout", "android-display,any");
        var_SetString(mp, "dec-dev", "android");
        var_SetString(mp, "window", "android");
        libheimdall_media_player_switch_vout(mp, libheimdall_media_player_detach_output_callbacks);
    }
    else
        return false;

    var_SetInteger( mp, "vout-cb-type", engine );
    var_SetAddress( mp, "vout-cb-opaque", opaque );
    var_SetAddress( mp, "vout-cb-setup", setup_cb );
    var_SetAddress( mp, "vout-cb-cleanup", cleanup_cb );
    var_SetAddress( mp, "vout-cb-window-cb", set_window_cb );
    var_SetAddress( mp, "vout-cb-update-output", update_output_cb );
    var_SetAddress( mp, "vout-cb-swap", swap_cb );
    var_SetAddress( mp, "vout-cb-get-proc-address", getProcAddress_cb );
    var_SetAddress( mp, "vout-cb-make-current", makeCurrent_cb );
    var_SetAddress( mp, "vout-cb-metadata", metadata_cb );
    var_SetAddress( mp, "vout-cb-select-plane", select_plane_cb );
    return true;
}

static void libheimdall_media_player_detach_nsobject(libheimdall_media_player_t *player)
{
    var_SetAddress(player, "drawable-nsobject", NULL);
}

/**************************************************************************
 * set_nsobject
 **************************************************************************/
void libheimdall_media_player_set_nsobject( libheimdall_media_player_t *p_mi,
                                        void * drawable )
{
    assert (p_mi != NULL);
    libheimdall_media_player_switch_vout(p_mi, libheimdall_media_player_detach_nsobject);

#ifdef __APPLE__
    var_SetAddress (p_mi, "drawable-nsobject", drawable);
#else
    (void)drawable;
    libheimdall_printerr ("can't set nsobject: APPLE build required");
    assert(false);
    var_SetString (p_mi, "window", "none");
#endif
}

/**************************************************************************
 * get_nsobject
 **************************************************************************/
void * libheimdall_media_player_get_nsobject( libheimdall_media_player_t *p_mi )
{
    assert (p_mi != NULL);
#ifdef __APPLE__
    return var_GetAddress (p_mi, "drawable-nsobject");
#else
    (void) p_mi;
    return NULL;
#endif
}

static void libheimdall_media_player_detach_xwindow(libheimdall_media_player_t *player)
{
    /* Window variable is not configurable from libheimdall */
    var_SetString(player, "window", "any");
    var_SetInteger(player, "drawable-xid", 0);
}

/**************************************************************************
 * set_xwindow
 **************************************************************************/
void libheimdall_media_player_set_xwindow( libheimdall_media_player_t *p_mi,
                                      uint32_t drawable )
{
    assert (p_mi != NULL);
    libheimdall_media_player_switch_vout(p_mi, libheimdall_media_player_detach_xwindow);

    var_SetString (p_mi, "window", drawable ? "embed-xid,any" : "any");
    var_SetInteger (p_mi, "drawable-xid", drawable);
}

/**************************************************************************
 * get_xwindow
 **************************************************************************/
uint32_t libheimdall_media_player_get_xwindow( libheimdall_media_player_t *p_mi )
{
    return var_GetInteger (p_mi, "drawable-xid");
}

static void libheimdall_media_player_detach_hwnd(libheimdall_media_player_t *player)
{
    /* Window variable is not configurable from libheimdall */
    var_SetString(player, "window", "any");
    var_SetInteger(player, "drawable-hwnd", 0);
}

/**************************************************************************
 * set_hwnd
 **************************************************************************/
void libheimdall_media_player_set_hwnd( libheimdall_media_player_t *p_mi,
                                   void *drawable )
{
    assert (p_mi != NULL);
    libheimdall_media_player_switch_vout(p_mi, libheimdall_media_player_detach_hwnd);
#if defined (_WIN32) || defined (__OS2__)
    var_SetString (p_mi, "window",
                   (drawable != NULL) ? "embed-hwnd,any" : "any");
    var_SetInteger (p_mi, "drawable-hwnd", (uintptr_t)drawable);
#else
    (void) drawable;
    libheimdall_printerr ("can't set hwnd: WIN32 build required");
    assert(false);
    var_SetString (p_mi, "window", "none");
#endif
}

/**************************************************************************
 * get_hwnd
 **************************************************************************/
void *libheimdall_media_player_get_hwnd( libheimdall_media_player_t *p_mi )
{
    assert (p_mi != NULL);
#if defined (_WIN32) || defined (__OS2__)
    return (void *)(uintptr_t)var_GetInteger (p_mi, "drawable-hwnd");
#else
    (void) p_mi;
    return NULL;
#endif
}

static void libheimdall_media_player_detach_android_context(libheimdall_media_player_t *player)
{
    var_SetAddress(player, "drawable-androidwindow", NULL);
}


/**************************************************************************
 * set_android_context
 **************************************************************************/
void libheimdall_media_player_set_android_context( libheimdall_media_player_t *p_mi,
                                              void *p_awindow_handler )
{
    assert (p_mi != NULL);
    libheimdall_media_player_switch_vout(p_mi, libheimdall_media_player_detach_android_context);
#ifdef __ANDROID__
    var_SetAddress (p_mi, "drawable-androidwindow", p_awindow_handler);
#else
    (void) p_awindow_handler;
    libheimdall_printerr ("can't set android context: ANDROID build required");
    assert(false);
    var_SetString (p_mi, "window", "none");
#endif
}

void libheimdall_audio_set_callbacks( libheimdall_media_player_t *mp,
                                 libheimdall_audio_play_cb play_cb,
                                 libheimdall_audio_pause_cb pause_cb,
                                 libheimdall_audio_resume_cb resume_cb,
                                 libheimdall_audio_flush_cb flush_cb,
                                 libheimdall_audio_drain_cb drain_cb,
                                 void *opaque )
{
    var_SetAddress( mp, "amem-play", play_cb );
    var_SetAddress( mp, "amem-pause", pause_cb );
    var_SetAddress( mp, "amem-resume", resume_cb );
    var_SetAddress( mp, "amem-flush", flush_cb );
    var_SetAddress( mp, "amem-drain", drain_cb );
    var_SetAddress( mp, "amem-data", opaque );
    var_SetString( mp, "aout", "amem,none" );

    heimdall_player_aout_Reset( mp->player );
}

void libheimdall_audio_set_volume_callback( libheimdall_media_player_t *mp,
                                       libheimdall_audio_set_volume_cb cb )
{
    var_SetAddress( mp, "amem-set-volume", cb );

    heimdall_player_aout_Reset( mp->player );
}

void libheimdall_audio_set_format_callbacks( libheimdall_media_player_t *mp,
                                        libheimdall_audio_setup_cb setup,
                                        libheimdall_audio_cleanup_cb cleanup )
{
    var_SetAddress( mp, "amem-setup", setup );
    var_SetAddress( mp, "amem-cleanup", cleanup );

    heimdall_player_aout_Reset( mp->player );
}

void libheimdall_audio_set_format( libheimdall_media_player_t *mp, const char *format,
                              unsigned rate, unsigned channels )
{
    var_SetString( mp, "amem-format", format );
    var_SetInteger( mp, "amem-rate", rate );
    var_SetInteger( mp, "amem-channels", channels );

    heimdall_player_aout_Reset( mp->player );
}


/**************************************************************************
 * Getters for stream information
 **************************************************************************/
libheimdall_time_t libheimdall_media_player_get_length(
                             libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_tick_t length = heimdall_player_GetLength(player);
    libheimdall_time_t i_time = libheimdall_time_from_heimdall_tick(length);

    heimdall_player_Unlock(player);
    return i_time;
}

libheimdall_time_t libheimdall_media_player_get_time( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_tick_t tick = heimdall_player_GetTime(player);
    libheimdall_time_t i_time = libheimdall_time_from_heimdall_tick(tick);

    heimdall_player_Unlock(player);
    return i_time;
}

static int
set_time( libheimdall_media_player_t *p_mi, libheimdall_time_t i_time, bool b_fast,
          enum heimdall_player_whence whence )
{
    heimdall_tick_t tick = heimdall_tick_from_libheimdall_time(i_time);

    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    enum heimdall_player_seek_speed speed = b_fast ? HEIMDALL_PLAYER_SEEK_FAST
                                              : HEIMDALL_PLAYER_SEEK_PRECISE;
    heimdall_player_SeekByTime(player, tick, speed, whence);

    heimdall_player_Unlock(player);

    /* may not fail anymore, keep int not to break the API */
    return 0;
}

int libheimdall_media_player_set_time( libheimdall_media_player_t *p_mi,
                                  libheimdall_time_t i_time, bool b_fast )
{
    return set_time( p_mi, i_time, b_fast, HEIMDALL_PLAYER_WHENCE_ABSOLUTE );
}

int libheimdall_media_player_jump_time( libheimdall_media_player_t *p_mi,
                                   libheimdall_time_t i_time )
{
    return set_time( p_mi, i_time, false, HEIMDALL_PLAYER_WHENCE_RELATIVE );
}

int libheimdall_media_player_set_position( libheimdall_media_player_t *p_mi,
                                      double position, bool b_fast )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    enum heimdall_player_seek_speed speed = b_fast ? HEIMDALL_PLAYER_SEEK_FAST
                                              : HEIMDALL_PLAYER_SEEK_PRECISE;
    heimdall_player_SeekByPos(player, position, speed, HEIMDALL_PLAYER_WHENCE_ABSOLUTE);

    heimdall_player_Unlock(player);

    /* may not fail anymore, keep int not to break the API */
    return 0;
}

double libheimdall_media_player_get_position( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    double f_position = heimdall_player_GetPosition(player);

    heimdall_player_Unlock(player);
    return f_position;
}

int
libheimdall_media_player_set_abloop_time( libheimdall_media_player_t *p_mi,
                                     libheimdall_time_t a_time, libheimdall_time_t b_time )
{
    heimdall_tick_t a_tick = heimdall_tick_from_libheimdall_time(a_time);
    if (a_tick >= 0)
        a_tick += HEIMDALL_TICK_0;

    heimdall_tick_t b_tick = heimdall_tick_from_libheimdall_time(b_time);
    if (b_tick >= 0)
        b_tick += HEIMDALL_TICK_0;

    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    int ret = heimdall_player_SetAtoBLoopTime(player, a_tick, b_tick);

    heimdall_player_Unlock(player);
    return ret == HEIMDALL_SUCCESS ? 0 : -1;
}

int
libheimdall_media_player_set_abloop_position( libheimdall_media_player_t *p_mi,
                                         double a_pos, double b_pos )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    int ret = heimdall_player_SetAtoBLoopPosition(player, a_pos, b_pos);

    heimdall_player_Unlock(player);
    return ret == HEIMDALL_SUCCESS ? 0 : -1;
}

int
libheimdall_media_player_reset_abloop( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    int ret = heimdall_player_ResetAtoBLoop(player);

    heimdall_player_Unlock(player);
    return ret == HEIMDALL_SUCCESS ? 0 : -1;
}

libheimdall_abloop_t
libheimdall_media_player_get_abloop( libheimdall_media_player_t *p_mi,
                                libheimdall_time_t *a_time, double *a_pos,
                                libheimdall_time_t *b_time, double *b_pos )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_tick_t a_ticks, b_ticks;
    int ret = heimdall_player_GetAtoBLoop(player, &a_ticks, a_pos, &b_ticks, b_pos);

    heimdall_player_Unlock(player);

    if (a_time != NULL)
        *a_time = a_ticks == HEIMDALL_TICK_INVALID ? -1 :
                  libheimdall_time_from_heimdall_tick(a_ticks);
    if (b_time != NULL)
        *b_time = b_ticks == HEIMDALL_TICK_INVALID ? -1 :
                  libheimdall_time_from_heimdall_tick(b_ticks);

    return ret;
}

void libheimdall_media_player_set_chapter( libheimdall_media_player_t *p_mi,
                                      int chapter )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_SelectChapterIdx(player, chapter);

    heimdall_player_Unlock(player);
}

int libheimdall_media_player_get_chapter( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    ssize_t i_chapter = heimdall_player_GetSelectedChapterIdx(player);

    heimdall_player_Unlock(player);
    return i_chapter;
}

int libheimdall_media_player_get_chapter_count( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    const struct heimdall_player_title *title = heimdall_player_GetSelectedTitle(player);
    int ret = title ? (int) title->chapter_count : -1;

    heimdall_player_Unlock(player);
    return ret;
}

int libheimdall_media_player_get_chapter_count_for_title(
                                 libheimdall_media_player_t *p_mi,
                                 int i_title )
{
    assert(i_title >= 0);
    size_t idx = i_title;
    int ret = -1;

    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_title_list *titles = heimdall_player_GetTitleList(player);
    if (!titles)
        goto end;

    size_t titles_count = heimdall_player_title_list_GetCount(titles);
    if (idx >= titles_count)
       goto end;

    const struct heimdall_player_title *title =
        heimdall_player_title_list_GetAt(titles, idx);
    assert(title);

    ret = title->chapter_count;

end:
    heimdall_player_Unlock(player);
    return ret;
}

void libheimdall_media_player_set_title( libheimdall_media_player_t *p_mi,
                                    unsigned i_title )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_SelectTitleIdx(player, i_title);

    heimdall_player_Unlock(player);
}

int libheimdall_media_player_get_title( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    ssize_t i_title = heimdall_player_GetSelectedTitleIdx(player);

    heimdall_player_Unlock(player);
    return i_title;
}

int libheimdall_media_player_get_title_count( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_title_list *titles = heimdall_player_GetTitleList(player);
    int ret = titles ? (int) heimdall_player_title_list_GetCount(titles) : -1;

    heimdall_player_Unlock(player);
    return ret;
}

int libheimdall_media_player_get_full_title_descriptions( libheimdall_media_player_t *p_mi,
                                                     libheimdall_title_description_t *** pp_titles )
{
    assert( p_mi );

    int ret = -1;

    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_title_list *titles = heimdall_player_GetTitleList(player);
    if (!titles)
        goto end;

    size_t count = heimdall_player_title_list_GetCount(titles);

    libheimdall_title_description_t **descs = heimdall_alloc(count, sizeof(*descs));
    if (count > 0 && !descs)
        goto end;

    for (size_t i = 0; i < count; i++)
    {
        const struct heimdall_player_title *title =
            heimdall_player_title_list_GetAt(titles, i);
        libheimdall_title_description_t *desc = malloc(sizeof(*desc));
        if (!desc)
        {
            libheimdall_title_descriptions_release(descs, i);
            goto end;
        }

        descs[i] = desc;

        /* we want to return microseconds to match the rest of the API */
        desc->i_duration = libheimdall_time_from_heimdall_tick(title->length);
        desc->i_flags = title->flags;
        desc->psz_name = title->name ? strdup(title->name) : NULL;
    }

    ret = count;
    *pp_titles = descs;

end:
    heimdall_player_Unlock(player);
    return ret;
}

void libheimdall_title_descriptions_release( libheimdall_title_description_t **p_titles,
                                        unsigned i_count )
{
    for (unsigned i = 0; i < i_count; i++ )
    {
        if ( !p_titles[i] )
            continue;

        free( p_titles[i]->psz_name );
        free( p_titles[i] );
    }
    free( p_titles );
}

int libheimdall_media_player_get_full_chapter_descriptions( libheimdall_media_player_t *p_mi,
                                                      int i_chapters_of_title,
                                                      libheimdall_chapter_description_t *** pp_chapters )
{
    assert( p_mi );

    int ret = -1;

    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_title_list *titles = heimdall_player_GetTitleList(player);
    if (!titles)
        goto end;

    size_t titles_count = heimdall_player_title_list_GetCount(titles);

    ssize_t title_idx;
    if (i_chapters_of_title < 0)
    {
        title_idx = heimdall_player_GetSelectedTitleIdx(player);
        if (title_idx == -1)
            goto end;

        assert((size_t)title_idx < titles_count);
    }
    else
    {
        title_idx = i_chapters_of_title;

        if ((size_t) title_idx  >= titles_count)
           goto end;
    }

    const struct heimdall_player_title *title =
        heimdall_player_title_list_GetAt(titles, title_idx);
    assert(title);

    size_t i_chapter_count = title->chapter_count;

    libheimdall_chapter_description_t **descs =
        heimdall_alloc(i_chapter_count, sizeof(*descs));
    if (i_chapter_count > 0 && !descs)
        goto end;

    for (size_t i = 0; i < i_chapter_count; i++)
    {
        const struct heimdall_player_chapter *chapter = &title->chapters[i];
        libheimdall_chapter_description_t *desc = malloc(sizeof(*desc));
        if (!desc)
        {
            libheimdall_chapter_descriptions_release(descs, i);
            goto end;
        }

        descs[i] = desc;

        heimdall_tick_t chapter_end = i < i_chapter_count - 1
                               ? title->chapters[i + 1].time
                               : title->length;
        desc->i_time_offset = libheimdall_time_from_heimdall_tick(chapter->time);
        desc->psz_name = chapter->name ? strdup(chapter->name) : NULL;
        desc->i_duration = libheimdall_time_from_heimdall_tick(chapter_end) - desc->i_time_offset;
    }

    ret = i_chapter_count;
    *pp_chapters = descs;

end:
    heimdall_player_Unlock(player);
    return ret;
}

void libheimdall_chapter_descriptions_release( libheimdall_chapter_description_t **p_chapters,
                                          unsigned i_count )
{
    for (unsigned i = 0; i < i_count; i++ )
    {
        if ( !p_chapters[i] )
            continue;

        free( p_chapters[i]->psz_name );
        free( p_chapters[i] );
    }
    free( p_chapters );
}

void libheimdall_media_player_next_chapter( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_SelectNextChapter(player);

    heimdall_player_Unlock(player);
}

void libheimdall_media_player_previous_chapter( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_SelectPrevChapter(player);

    heimdall_player_Unlock(player);
}

int libheimdall_media_player_set_rate( libheimdall_media_player_t *p_mi, float rate )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_ChangeRate(player, rate);

    heimdall_player_Unlock(player);
    return 0;
}

float libheimdall_media_player_get_rate( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    float rate = heimdall_player_GetRate(player);

    heimdall_player_Unlock(player);
    return rate;
}

libheimdall_state_t libheimdall_media_player_get_state( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    enum heimdall_player_error error = heimdall_player_GetError(player);
    enum heimdall_player_state state = heimdall_player_GetState(player);

    heimdall_player_Unlock(player);

    if (error != HEIMDALL_PLAYER_ERROR_NONE)
        return libheimdall_Error;
    return libheimdall_state_from_player(state);
}

bool libheimdall_media_player_is_seekable(libheimdall_media_player_t *p_mi)
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    bool b_seekable = heimdall_player_CanSeek(player);

    heimdall_player_Unlock(player);
    return b_seekable;
}

void libheimdall_media_player_navigate( libheimdall_media_player_t* p_mi,
                                   unsigned navigate )
{
    static const enum heimdall_player_nav map[] =
    {
        HEIMDALL_PLAYER_NAV_ACTIVATE, HEIMDALL_PLAYER_NAV_UP, HEIMDALL_PLAYER_NAV_DOWN,
        HEIMDALL_PLAYER_NAV_LEFT, HEIMDALL_PLAYER_NAV_RIGHT, HEIMDALL_PLAYER_NAV_POPUP,
    };

    if( navigate >= sizeof(map) / sizeof(map[0]) )
      return;

    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_Navigate(player, map[navigate]);

    heimdall_player_Unlock(player);
}

bool libheimdall_media_player_can_pause(libheimdall_media_player_t *p_mi)
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    bool b_can_pause = heimdall_player_CanPause(player);

    heimdall_player_Unlock(player);
    return b_can_pause;
}

bool libheimdall_media_player_program_scrambled(libheimdall_media_player_t *p_mi)
{
    const struct heimdall_player_program *program;
    bool b_program_scrambled = false;

    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);
    program = heimdall_player_GetSelectedProgram(player);
    if (program != NULL)
        b_program_scrambled = program->scrambled;
    heimdall_player_Unlock(player);
    return b_program_scrambled;
}

void libheimdall_media_player_next_frame( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_NextVideoFrame(player);

    heimdall_player_Unlock(player);
}

void libheimdall_media_player_previous_frame( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    heimdall_player_PreviousVideoFrame(player);

    heimdall_player_Unlock(player);
}

/**
 * Private lookup table to get subpicture alignment flag values corresponding
 * to a libheimdall_position_t enumerated value.
 */
static const unsigned char position_subpicture_alignment[] = {
    [libheimdall_position_center]       = 0,
    [libheimdall_position_left]         = SUBPICTURE_ALIGN_LEFT,
    [libheimdall_position_right]        = SUBPICTURE_ALIGN_RIGHT,
    [libheimdall_position_top]          = SUBPICTURE_ALIGN_TOP,
    [libheimdall_position_top_left]     = SUBPICTURE_ALIGN_TOP | SUBPICTURE_ALIGN_LEFT,
    [libheimdall_position_top_right]    = SUBPICTURE_ALIGN_TOP | SUBPICTURE_ALIGN_RIGHT,
    [libheimdall_position_bottom]       = SUBPICTURE_ALIGN_BOTTOM,
    [libheimdall_position_bottom_left]  = SUBPICTURE_ALIGN_BOTTOM | SUBPICTURE_ALIGN_LEFT,
    [libheimdall_position_bottom_right] = SUBPICTURE_ALIGN_BOTTOM | SUBPICTURE_ALIGN_RIGHT
};

void libheimdall_media_player_set_video_title_display( libheimdall_media_player_t *p_mi, libheimdall_position_t position, unsigned timeout )
{
    assert( position >= libheimdall_position_disable && position <= libheimdall_position_bottom_right );

    if ( position != libheimdall_position_disable )
    {
        var_SetBool( p_mi, "video-title-show", true );
        var_SetInteger( p_mi, "video-title-position", position_subpicture_alignment[position] );
        var_SetInteger( p_mi, "video-title-timeout", timeout );
    }
    else
    {
        var_SetBool( p_mi, "video-title-show", false );
    }
}

libheimdall_media_tracklist_t *
libheimdall_media_player_get_tracklist(libheimdall_media_player_t *p_mi,
                                  libheimdall_track_type_t type, bool selected)
{
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    libheimdall_media_tracklist_t *list =
        libheimdall_media_tracklist_from_player(player, type, selected);

    heimdall_player_Unlock(player);

    return list;
}

libheimdall_media_track_t *
libheimdall_media_player_get_selected_track(libheimdall_media_player_t *p_mi,
                                       libheimdall_track_type_t type)
{
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    const enum es_format_category_e cat = libheimdall_track_type_to_escat(type);
    const struct heimdall_player_track *track =
        heimdall_player_GetSelectedTrack(player, cat);

    if (track == NULL)
    {
        heimdall_player_Unlock(player);
        return NULL;
    }

    libheimdall_media_track_t *libtrack =
        libheimdall_media_track_create_from_player_track(track);
    heimdall_player_Unlock(player);

    return libtrack;
}

libheimdall_media_track_t *
libheimdall_media_player_get_track_from_id( libheimdall_media_player_t *p_mi,
                                       const char *psz_id )
{
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    enum es_format_category_e cats[] = { VIDEO_ES, AUDIO_ES, SPU_ES };
    for (size_t i = 0; i < ARRAY_SIZE(cats); ++i)
    {
        enum es_format_category_e cat = cats[i];
        size_t count = heimdall_player_GetTrackCount(player, cat);

        for (size_t j = 0; j < count; ++j)
        {
            const struct heimdall_player_track *track =
                heimdall_player_GetTrackAt(player, cat, j);
            if (strcmp(psz_id, heimdall_es_id_GetStrId(track->es_id)) == 0)
            {
                libheimdall_media_track_t *libtrack =
                    libheimdall_media_track_create_from_player_track(track);
                heimdall_player_Unlock(player);
                return libtrack;

            }
        }
    }

    heimdall_player_Unlock(player);
    return NULL;
}

void
libheimdall_media_player_select_track(libheimdall_media_player_t *p_mi,
                                 const libheimdall_media_track_t *track)
{
    assert( track != NULL );
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    const libheimdall_media_trackpriv_t *trackpriv =
        libheimdall_media_track_to_priv(track);

    // It must be a player track
    assert(trackpriv->es_id);

    heimdall_player_SelectEsId(player, trackpriv->es_id,
                          HEIMDALL_PLAYER_SELECT_EXCLUSIVE);

    heimdall_player_Unlock(player);
}

void
libheimdall_media_player_unselect_track_type( libheimdall_media_player_t *p_mi,
                                         libheimdall_track_type_t type )
{
    heimdall_player_t *player = p_mi->player;
    const enum es_format_category_e cat = libheimdall_track_type_to_escat(type);

    heimdall_player_Lock(player);
    heimdall_player_UnselectTrackCategory(player, cat);
    heimdall_player_Unlock(player);
}

void
libheimdall_media_player_select_tracks(libheimdall_media_player_t *p_mi,
                                  libheimdall_track_type_t type,
                                  const libheimdall_media_track_t **tracks,
                                  size_t track_count)
{
    heimdall_player_t *player = p_mi->player;

    heimdall_es_id_t **es_id_list = heimdall_alloc(track_count + 1, sizeof(heimdall_es_id_t *));
    size_t es_id_idx = 0;

    if (es_id_list == NULL)
        return;

    const enum es_format_category_e cat = libheimdall_track_type_to_escat(type);

    heimdall_player_Lock(player);

    for (size_t i = 0; i < track_count; ++i)
    {
        const libheimdall_media_track_t *track = tracks[i];
        const libheimdall_media_trackpriv_t *trackpriv =
            libheimdall_media_track_to_priv(track);

        // It must be a player track
        assert(trackpriv->es_id);

        es_id_list[es_id_idx++] = trackpriv->es_id;
    }
    es_id_list[es_id_idx++] = NULL;
    heimdall_player_SelectEsIdList(player, cat, es_id_list);

    heimdall_player_Unlock(player);

    free(es_id_list);
}

void
libheimdall_media_player_select_tracks_by_ids( libheimdall_media_player_t *p_mi,
                                          libheimdall_track_type_t type,
                                          const char *psz_ids )
{
    const enum es_format_category_e cat = libheimdall_track_type_to_escat(type);

    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    heimdall_player_SelectTracksByStringIds(player, cat, psz_ids);

    heimdall_player_Unlock(player);
}

int libheimdall_media_player_add_slave( libheimdall_media_player_t *p_mi,
                                   libheimdall_media_slave_type_t i_type,
                                   const char *psz_uri, bool b_select )
{
    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    enum es_format_category_e cat = i_type == libheimdall_media_slave_type_subtitle
                                  ? SPU_ES
                                  : AUDIO_ES;

    int ret = heimdall_player_AddAssociatedMedia(player, cat, psz_uri, b_select,
                                            false, false);

    heimdall_player_Unlock(player);
    return ret;
}

/**
 * Maximum size of a formatted equalizer amplification band frequency value.
 *
 * The allowed value range is supposed to be constrained from -20.0 to 20.0.
 *
 * The format string " %.07f" with a minimum value of "-20" gives a maximum
 * string length of e.g. " -19.1234567", i.e. 12 bytes (not including the null
 * terminator).
 */
#define EQZ_BAND_VALUE_SIZE 12

int libheimdall_media_player_set_equalizer( libheimdall_media_player_t *p_mi, libheimdall_equalizer_t *p_equalizer )
{
    char bands[EQZ_BANDS_MAX * EQZ_BAND_VALUE_SIZE + 1];

    if( p_equalizer != NULL )
    {
        for( unsigned i = 0, c = 0; i < EQZ_BANDS_MAX; i++ )
        {
            c += snprintf( bands + c, sizeof(bands) - c, " %.07f",
                          p_equalizer->f_amp[i] );
            if( unlikely(c >= sizeof(bands)) )
                return -1;
        }

        var_SetFloat( p_mi, "equalizer-preamp", p_equalizer->f_preamp );
        var_SetString( p_mi, "equalizer-bands", bands );
    }
    var_SetString( p_mi, "audio-filter", p_equalizer ? "equalizer" : "" );

    audio_output_t *p_aout = heimdall_player_aout_Hold( p_mi->player );
    if( p_aout != NULL )
    {
        if( p_equalizer != NULL )
        {
            var_SetFloat( p_aout, "equalizer-preamp", p_equalizer->f_preamp );
            var_SetString( p_aout, "equalizer-bands", bands );
        }

        var_SetString( p_aout, "audio-filter", p_equalizer ? "equalizer" : "" );
        aout_Release(p_aout);
    }

    return 0;
}


static libheimdall_player_program_t *
libheimdall_player_program_new(const struct heimdall_player_program *program)
{
    libheimdall_player_program_t *libprogram = malloc(sizeof(*libprogram));
    if (libprogram == NULL)
        return NULL;

    libprogram->i_group_id = program->group_id;
    libprogram->psz_name = strdup(program->name);
    libprogram->b_selected = program->selected;
    libprogram->b_scrambled = program->scrambled;

    return libprogram;
}

void
libheimdall_player_program_delete( libheimdall_player_program_t *program )
{
    free( program->psz_name );
    free( program );
}

void libheimdall_media_player_select_program_id( libheimdall_media_player_t *p_mi,
                                            int program_id)
{
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    heimdall_player_SelectProgram(player, program_id);

    heimdall_player_Unlock(player);
}

libheimdall_player_program_t *
libheimdall_media_player_get_selected_program( libheimdall_media_player_t *p_mi)
{
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    const struct heimdall_player_program *program = heimdall_player_GetSelectedProgram( player );
    if( program == NULL )
    {
        heimdall_player_Unlock(player);
        return NULL;
    }
    libheimdall_player_program_t *libprogram = libheimdall_player_program_new(program);

    heimdall_player_Unlock(player);

    return libprogram;
}

libheimdall_player_program_t *
libheimdall_media_player_get_program_from_id( libheimdall_media_player_t *p_mi, int i_group_id )
{
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    libheimdall_player_program_t *libprogram = NULL;

    size_t count = heimdall_player_GetProgramCount(player);
    for (size_t i = 0; i < count; ++i)
    {
        const struct heimdall_player_program *program =
            heimdall_player_GetProgramAt(player, i);
        assert(program);
        if (program->group_id == i_group_id)
        {
            libprogram = libheimdall_player_program_new(program);
            break;
        }
    }

    heimdall_player_Unlock(player);

    return libprogram;
}

struct libheimdall_player_programlist_t
{
    size_t count;
    libheimdall_player_program_t *programs[];
};

size_t
libheimdall_player_programlist_count( const libheimdall_player_programlist_t *list )
{
    return list->count;
}

libheimdall_player_program_t *
libheimdall_player_programlist_at( libheimdall_player_programlist_t *list, size_t index )
{
    assert(index < list->count);
    return list->programs[index];
}

void
libheimdall_player_programlist_delete( libheimdall_player_programlist_t *list )
{
    for (size_t i = 0; i < list->count; ++i)
        libheimdall_player_program_delete(list->programs[i]);
    free(list);
}

libheimdall_player_programlist_t *
libheimdall_media_player_get_programlist( libheimdall_media_player_t *p_mi )
{
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    size_t count = heimdall_player_GetProgramCount(player);
    if (count == 0)
        goto error;

    size_t size;
    if (ckd_mul(&size, count, sizeof (libheimdall_player_program_t *)) ||
        ckd_add(&size, sizeof (libheimdall_player_programlist_t), size))
        goto error;

    libheimdall_player_programlist_t *list = malloc( size );
    if( list == NULL )
        goto error;

    list->count = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const struct heimdall_player_program *program =
            heimdall_player_GetProgramAt(player, i);
        assert(program);
        list->programs[i] = libheimdall_player_program_new(program);
        if (list->programs[i] == NULL)
        {
            libheimdall_player_programlist_delete(list);
            goto error;
        }

        list->count++;
    }

    heimdall_player_Unlock(player);

    return list;

error:
    heimdall_player_Unlock(player);
    return NULL;
}

static const char roles[][16] =
{
    [libheimdall_role_Music] =         "music",
    [libheimdall_role_Video] =         "video",
    [libheimdall_role_Communication] = "communication",
    [libheimdall_role_Game] =          "game",
    [libheimdall_role_Notification] =  "notification",
    [libheimdall_role_Animation] =     "animation",
    [libheimdall_role_Production] =    "production",
    [libheimdall_role_Accessibility] = "accessibility",
    [libheimdall_role_Test] =          "test",
};

int libheimdall_media_player_set_role(libheimdall_media_player_t *mp, unsigned role)
{
    if (role >= ARRAY_SIZE(roles)
     || var_SetString(mp, "role", roles[role]) != HEIMDALL_SUCCESS)
        return -1;
    return 0;
}

int libheimdall_media_player_get_role(libheimdall_media_player_t *mp)
{
    int ret = -1;
    char *str = var_GetString(mp, "role");
    if (str == NULL)
        return 0;

    for (size_t i = 0; i < ARRAY_SIZE(roles); i++)
        if (!strcmp(roles[i], str))
        {
            ret = i;
            break;
        }

    free(str);
    return ret;
}

void libheimdall_media_player_record( libheimdall_media_player_t *p_mi,
                                 bool enable,
                                 const char *path)
{
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);
    heimdall_player_SetRecordingEnabled(player, enable, path);
    heimdall_player_Unlock(player);
}

#define PLAYER_TIME_CORE_TO_LIB(point) { \
    .position = point->position, \
    .rate = point->rate, \
    .ts_us = US_FROM_HEIMDALL_TICK(point->ts), \
    .length_us = US_FROM_HEIMDALL_TICK(point->length), \
    .system_date_us = US_FROM_HEIMDALL_TICK(point->system_date), \
}

#define PLAYER_TIME_LIB_TO_CORE(point) { \
    .position = point->position, \
    .rate = point->rate, \
    .ts = HEIMDALL_TICK_FROM_US(point->ts_us), \
    .length = HEIMDALL_TICK_FROM_US(point->length_us), \
    .system_date = HEIMDALL_TICK_FROM_US(point->system_date_us), \
}

static void player_timer_on_update(const struct heimdall_player_timer_point *point,
                                   void *data)
{
    libheimdall_media_player_t *p_mi = data;

    if (p_mi->timer.seeking)
        return;

    const libheimdall_media_player_time_point_t libpoint = PLAYER_TIME_CORE_TO_LIB(point);

    p_mi->timer.cbs->on_update(p_mi->timer.cbs_data, &libpoint);
}

static void player_timer_on_paused(heimdall_tick_t system_date, void *data)
{
    libheimdall_media_player_t *p_mi = data;

    if (p_mi->timer.cbs->on_paused == NULL)
        return;

    p_mi->timer.cbs->on_paused(p_mi->timer.cbs_data,
                               US_FROM_HEIMDALL_TICK(system_date));
}

static void player_timer_on_seek(const struct heimdall_player_timer_point *point,
                                 void *data)
{
    libheimdall_media_player_t *p_mi = data;

    if (p_mi->timer.cbs->on_seek == NULL)
        return;

    if (point != NULL)
    {
        const libheimdall_media_player_time_point_t libpoint = PLAYER_TIME_CORE_TO_LIB(point);
        p_mi->timer.cbs->on_seek(p_mi->timer.cbs_data, &libpoint);
        p_mi->timer.seeking = true;
    }
    else
    {
        p_mi->timer.cbs->on_seek(p_mi->timer.cbs_data, NULL);
        p_mi->timer.seeking = false;
    }
}

int
libheimdall_media_player_watch_time(libheimdall_media_player_t *p_mi,
                               libheimdall_time_t min_period_us,
                               const struct libheimdall_media_player_watch_time_cbs *cbs,
                               void *cbs_data)
{
    assert(cbs != NULL && cbs->on_update != NULL);

    /* No different versions to handle for now */
    assert(cbs->version <= 0);

    static const struct heimdall_player_timer_cbs player_timer_cbs = {
        .on_update = player_timer_on_update,
        .on_paused = player_timer_on_paused,
        .on_seek = player_timer_on_seek,
    };

    heimdall_player_t *player = p_mi->player;
    heimdall_player_Lock(player);

    if (p_mi->timer.id != NULL)
    {
        libheimdall_printerr("libheimdall_media_player_watch_time error:"
                        "already watching for events");
        heimdall_player_Unlock(player);
        return -1;
    }

    p_mi->timer.cbs = cbs;
    p_mi->timer.cbs_data = cbs_data;
    p_mi->timer.seeking = false;

    p_mi->timer.id = heimdall_player_AddTimer(player, HEIMDALL_TICK_FROM_US(min_period_us),
                                         &player_timer_cbs, p_mi);
    heimdall_player_Unlock(player);

    if (unlikely(p_mi->timer.id == NULL))
        return -1;

    return 0;
}

void
libheimdall_media_player_unwatch_time(libheimdall_media_player_t *p_mi)
{
    heimdall_player_t *player = p_mi->player;

    heimdall_player_Lock(player);

    assert(p_mi->timer.id != NULL);
    heimdall_player_RemoveTimer(player, p_mi->timer.id);
    p_mi->timer.id = NULL;

    heimdall_player_Unlock(player);
}

int
libheimdall_media_player_time_point_interpolate(const libheimdall_media_player_time_point_t *libpoint,
                                           libheimdall_time_t system_now_us,
                                           libheimdall_time_t *out_ts_us, double *out_pos)
{
    const struct heimdall_player_timer_point point = PLAYER_TIME_LIB_TO_CORE(libpoint);

    heimdall_tick_t out_ts;
    int ret = heimdall_player_timer_point_Interpolate(&point,
                                                 HEIMDALL_TICK_FROM_US(system_now_us),
                                                 &out_ts, out_pos);
    *out_ts_us = US_FROM_HEIMDALL_TICK(out_ts);
    return ret;
}

libheimdall_time_t
libheimdall_media_player_time_point_get_next_date(const libheimdall_media_player_time_point_t *libpoint,
                                             libheimdall_time_t system_now_us,
                                             libheimdall_time_t interpolated_ts_us,
                                             libheimdall_time_t next_interval_us)
{
    const struct heimdall_player_timer_point point = PLAYER_TIME_LIB_TO_CORE(libpoint);

    heimdall_tick_t date =
        heimdall_player_timer_point_GetNextIntervalDate(&point,
                                                   HEIMDALL_TICK_FROM_US(system_now_us),
                                                   HEIMDALL_TICK_FROM_US(interpolated_ts_us),
                                                   HEIMDALL_TICK_FROM_US(next_interval_us));
    return US_FROM_HEIMDALL_TICK(date);
}

#include <heimdall_vout_display.h>

/* make sure surface structures from libheimdall can be passed as such to heimdall
   otherwise we will need wrappers between what libheimdall understands and what heimdall uses */
#define cast_  libheimdall_video_color_space_t
static_assert(libheimdall_video_colorspace_BT601  == (cast_)COLOR_SPACE_BT601 &&
              libheimdall_video_colorspace_BT709  == (cast_)COLOR_SPACE_BT709 &&
              libheimdall_video_colorspace_BT2020 == (cast_)COLOR_SPACE_BT2020
              , "libheimdall video colorspace mismatch");
#undef cast_

#define cast_  libheimdall_video_transfer_func_t
static_assert(libheimdall_video_transfer_func_LINEAR       == (cast_)TRANSFER_FUNC_LINEAR &&
              libheimdall_video_transfer_func_SRGB         == (cast_)TRANSFER_FUNC_SRGB &&
              libheimdall_video_transfer_func_BT470_BG     == (cast_)TRANSFER_FUNC_BT470_BG &&
              libheimdall_video_transfer_func_BT470_M      == (cast_)TRANSFER_FUNC_BT470_M &&
              libheimdall_video_transfer_func_BT709        == (cast_)TRANSFER_FUNC_BT709 &&
              libheimdall_video_transfer_func_PQ           == (cast_)TRANSFER_FUNC_SMPTE_ST2084 &&
              libheimdall_video_transfer_func_SMPTE_240    == (cast_)TRANSFER_FUNC_SMPTE_240 &&
              libheimdall_video_transfer_func_HLG          == (cast_)TRANSFER_FUNC_HLG
              , "libheimdall video transfer function mismatch");
#undef cast_

#define cast_  libheimdall_video_color_primaries_t
static_assert(libheimdall_video_primaries_BT601_525 == (cast_)COLOR_PRIMARIES_BT601_525 &&
              libheimdall_video_primaries_BT601_625 == (cast_)COLOR_PRIMARIES_BT601_625 &&
              libheimdall_video_primaries_BT709     == (cast_)COLOR_PRIMARIES_BT709 &&
              libheimdall_video_primaries_BT2020    == (cast_)COLOR_PRIMARIES_BT2020 &&
              libheimdall_video_primaries_DCI_P3    == (cast_)COLOR_PRIMARIES_DCI_P3 &&
              libheimdall_video_primaries_BT470_M   == (cast_)COLOR_PRIMARIES_BT470_M
              , "libheimdall video color primaries mismatch");
#undef cast_

#define cast_ libheimdall_abloop_t
static_assert(libheimdall_abloop_none == (cast_) HEIMDALL_PLAYER_ABLOOP_NONE &&
              libheimdall_abloop_a    == (cast_) HEIMDALL_PLAYER_ABLOOP_A &&
              libheimdall_abloop_b    == (cast_) HEIMDALL_PLAYER_ABLOOP_B
              , "libheimdall abloop mismatch");
#undef cast_
