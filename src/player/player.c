/*****************************************************************************
 * player.c: Player implementation
 *****************************************************************************
 * Copyright © 2018-2019 VLC authors and VideoLAN
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

#include <limits.h>

#include <heimdall_common.h>
#include "player.h"
#include <heimdall_aout.h>
#include <heimdall_renderer_discovery.h>
#include <heimdall_tick.h>
#include <heimdall_decoder.h>
#include <heimdall_memstream.h>
#include <heimdall_http.h>
#include <heimdall_vout_osd.h>

#include "../libheimdall.h"
#include "input/resource.h"
#include "audio_output/aout_internal.h"

static_assert(HEIMDALL_PLAYER_CAP_SEEK == HEIMDALL_INPUT_CAPABILITIES_SEEKABLE &&
              HEIMDALL_PLAYER_CAP_PAUSE == HEIMDALL_INPUT_CAPABILITIES_PAUSEABLE &&
              HEIMDALL_PLAYER_CAP_CHANGE_RATE == HEIMDALL_INPUT_CAPABILITIES_CHANGE_RATE &&
              HEIMDALL_PLAYER_CAP_REWIND == HEIMDALL_INPUT_CAPABILITIES_REWINDABLE,
              "player/input capabilities mismatch");

static_assert(HEIMDALL_PLAYER_TITLE_MENU == INPUT_TITLE_MENU &&
              HEIMDALL_PLAYER_TITLE_INTERACTIVE == INPUT_TITLE_INTERACTIVE &&
              HEIMDALL_PLAYER_TITLE_MAIN == INPUT_TITLE_MAIN,
              "player/input title flag mismatch");

#define heimdall_player_foreach_inputs(it) \
    for (struct heimdall_player_input *it = player->input; it != NULL; it = NULL)

int
heimdall_player_OpenNextMedia(heimdall_player_t *player)
{
    assert(player->input == NULL);

    /* Tracks string ids are only remembered for one media */
    free(player->video_string_ids);
    free(player->audio_string_ids);
    free(player->sub_string_ids);
    player->video_string_ids = player->audio_string_ids =
    player->sub_string_ids = NULL;

    int ret = HEIMDALL_SUCCESS;
    if (player->releasing_media)
    {
        assert(player->media);
        input_item_Release(player->media);
        player->media = NULL;
        player->releasing_media = false;
    }
    else
    {
        if (!player->next_media)
            return HEIMDALL_EGENERIC;

        if (player->media)
            input_item_Release(player->media);
        player->media = player->next_media;
        player->next_media = NULL;

        struct heimdall_player_input *input = player->input =
            heimdall_player_input_New(player, player->media);
        if (!input)
        {
            input_item_Release(player->media);
            player->media = NULL;
            ret = HEIMDALL_ENOMEM;
        }
    }
    heimdall_player_SendEvent(player, on_current_media_changed, player->media);
    if (player->input && player->input->ml.delay_restore)
    {
        heimdall_player_SendEvent(player, on_playback_restore_queried);
        player->input->ml.delay_restore = false;
    }
    return ret;
}

static void
heimdall_player_CancelWaitError(heimdall_player_t *player)
{
    if (player->eos_burst_count != 0)
    {
        player->eos_burst_count = 0;
        heimdall_cond_signal(&player->start_delay_cond);
    }
}

static bool
heimdall_list_HasInput(struct heimdall_list *list, struct heimdall_player_input *input)
{
    struct heimdall_player_input *other_input;
    heimdall_list_foreach(other_input, list, node)
    {
        if (other_input == input)
            return true;
    }
    return false;
}

static void
heimdall_player_destructor_AddInput(heimdall_player_t *player,
                               struct heimdall_player_input *input)
{
    if (input->started)
    {
        input->started = false;
        /* Add this input to the stop list: it will be stopped by the
         * destructor thread */
        assert(!heimdall_list_HasInput(&player->mainloop.stopping_inputs, input));
        assert(!heimdall_list_HasInput(&player->mainloop.joinable_inputs, input));
        heimdall_list_append(&input->node, &player->mainloop.stop_inputs);
    }
    else
    {
        /* Add this input to the joinable list: it will be deleted by the
         * destructor thread */
        assert(!heimdall_list_HasInput(&player->mainloop.stop_inputs, input));
        assert(!heimdall_list_HasInput(&player->mainloop.joinable_inputs, input));
        heimdall_list_append(&input->node, &player->mainloop.joinable_inputs);
    }

    heimdall_cond_signal(&input->player->mainloop.wait);
}

void
heimdall_player_destructor_AddStoppingInput(heimdall_player_t *player,
                                       struct heimdall_player_input *input)
{
    /* Add this input to the stopping list */
    if (heimdall_list_HasInput(&player->mainloop.stop_inputs, input))
        heimdall_list_remove(&input->node);
    if (!heimdall_list_HasInput(&player->mainloop.stopping_inputs, input))
    {
        heimdall_list_append(&input->node, &player->mainloop.stopping_inputs);
        heimdall_cond_signal(&input->player->mainloop.wait);
    }
}

void
heimdall_player_destructor_AddJoinableInput(heimdall_player_t *player,
                                       struct heimdall_player_input *input)
{
    if (heimdall_list_HasInput(&player->mainloop.stopping_inputs, input))
        heimdall_list_remove(&input->node);

    assert(!input->started);
    heimdall_player_destructor_AddInput(player, input);
}

static bool heimdall_player_destructor_IsEmpty(heimdall_player_t *player)
{
    return heimdall_list_is_empty(&player->mainloop.stop_inputs)
        && heimdall_list_is_empty(&player->mainloop.stopping_inputs)
        && heimdall_list_is_empty(&player->mainloop.joinable_inputs);
}

void
heimdall_player_SignalAtoBLoop(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input || !input->abloop_state[0].set || !input->abloop_state[1].set)
        return;

    heimdall_cond_signal(&player->mainloop.wait);
}

static heimdall_tick_t
heimdall_player_GetAtoBLoopDeadline(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input || input->pause_date != HEIMDALL_TICK_INVALID || !input->abloop_state[0].set || !input->abloop_state[1].set)
        return HEIMDALL_TICK_MIN;

    heimdall_tick_t now = heimdall_tick_now();
    heimdall_tick_t ts;
    bool seeking = false;
    if (heimdall_player_GetTimerPoint(player, &seeking, now, &ts, NULL) != 0)
        return HEIMDALL_TICK_MIN;
    if (seeking)
        return HEIMDALL_TICK_MIN;

    heimdall_tick_t b_time = input->abloop_state[1].time;
    if (b_time == HEIMDALL_TICK_INVALID)
    {
        if (input->length == HEIMDALL_TICK_INVALID)
            return HEIMDALL_TICK_MIN;
        b_time = input->abloop_state[1].pos * input->length;
    }
    return now + (heimdall_tick_t) ((b_time - ts) / input->rate);
}

static void *
heimdall_player_mainloop_Thread(void *data)
{
    heimdall_player_t *player = data;

    heimdall_thread_set_name("heimdall-player-end");

    heimdall_mutex_lock(&player->lock);

    /* Terminate this thread when the player is deleting (heimdall_player_Delete()
     * was called) and when all input_thread_t all stopped and released. */
    while (!player->deleting
        || !heimdall_player_destructor_IsEmpty(player))
    {
        /* Wait for an input to stop or close. No while loop here since we want
         * to leave this code path when the player is deleting. */
        heimdall_tick_t deadline = heimdall_player_GetAtoBLoopDeadline(player);
        bool timeout = false;
        if (heimdall_list_is_empty(&player->mainloop.stop_inputs)
         && heimdall_list_is_empty(&player->mainloop.joinable_inputs))
        {
            if (deadline == HEIMDALL_TICK_MIN)
                heimdall_cond_wait(&player->mainloop.wait, &player->lock);
            else
                timeout = heimdall_cond_timedwait(&player->mainloop.wait,
                                             &player->lock, deadline) != 0;
        }

        if (timeout && player->input != NULL)
            heimdall_player_input_HandleAtoBLoop(player->input, true);

        struct heimdall_player_input *input;
        heimdall_list_foreach(input, &player->mainloop.stop_inputs, node)
        {
            input_Stop(input->thread);
            heimdall_player_input_HandleState(input, HEIMDALL_PLAYER_STATE_STOPPING,
                                         HEIMDALL_TICK_INVALID);
            heimdall_player_destructor_AddStoppingInput(player, input);
        }

        bool keep_sout = true;
        const bool inputs_changed =
            !heimdall_list_is_empty(&player->mainloop.joinable_inputs);
        heimdall_list_foreach(input, &player->mainloop.joinable_inputs, node)
        {
            heimdall_player_UpdateMLStates(player, input);

            keep_sout = var_GetBool(input->thread, "sout-keep");

            heimdall_player_input_HandleState(input, HEIMDALL_PLAYER_STATE_STOPPED,
                                         HEIMDALL_TICK_INVALID);

            heimdall_list_remove(&input->node);
            heimdall_player_input_Delete(input);
        }

        if (inputs_changed)
        {
            const bool started = player->started;
            heimdall_player_Unlock(player);
            if (!started)
                input_resource_StopFreeVout(player->resource);
            if (!keep_sout)
                input_resource_TerminateSout(player->resource);
            heimdall_player_Lock(player);
        }
    }
    heimdall_mutex_unlock(&player->lock);
    return NULL;
}

size_t
heimdall_player_GetProgramCount(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    return input ? input->program_vector.size : 0;
}

const struct heimdall_player_program *
heimdall_player_GetProgramAt(heimdall_player_t *player, size_t index)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return NULL;

    assert(index < input->program_vector.size);
    return input->program_vector.data[index];
}

const struct heimdall_player_program *
heimdall_player_GetProgram(heimdall_player_t *player, int id)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return NULL;

    struct heimdall_player_program *prgm =
        heimdall_player_program_vector_FindById(&input->program_vector, id, NULL);
    return prgm;
}

void
heimdall_player_SelectProgram(heimdall_player_t *player, int id)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;

    input_SetProgramId(input->thread, id);

    const struct heimdall_player_program *prgm =
        heimdall_player_program_vector_FindById(&input->program_vector,
                                           id, NULL);
    if (prgm != NULL)
        heimdall_player_osd_Program(player, prgm->name);
}

static void
heimdall_player_CycleProgram(heimdall_player_t *player, bool next)
{
    size_t count = heimdall_player_GetProgramCount(player);
    if (!count)
        return;
    size_t index = 0;
    bool selected = false;
    for (size_t i = 0; i < count; ++i)
    {
        const struct heimdall_player_program *prgm =
            heimdall_player_GetProgramAt(player, i);
        if (prgm->selected)
        {
            /* Only one program can be selected at a time */
            assert(!selected);
            index = i;
            selected = true;
        }
    }
    assert(selected);
    if (next && index + 1 == count) /* First program */
        index = 0;
    else if (!next && index == 0) /* Last program */
        index = count - 1;
    else /* Next or Previous program */
        index = index + (next ? 1 : -1);

    const struct heimdall_player_program *prgm =
        heimdall_player_GetProgramAt(player, index);
    assert(prgm);
    heimdall_player_SelectProgram(player, prgm->group_id);
}

void
heimdall_player_SelectNextProgram(heimdall_player_t *player)
{
    heimdall_player_CycleProgram(player, true);
}

void
heimdall_player_SelectPrevProgram(heimdall_player_t *player)
{
    heimdall_player_CycleProgram(player, false);
}

size_t
heimdall_player_GetTrackCount(heimdall_player_t *player, enum es_format_category_e cat)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return 0;
    heimdall_player_track_vector *vec = heimdall_player_input_GetTrackVector(input, cat);
    if (!vec)
        return 0;
    return vec->size;
}

const struct heimdall_player_track *
heimdall_player_GetTrackAt(heimdall_player_t *player, enum es_format_category_e cat,
                      size_t index)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return NULL;
    heimdall_player_track_vector *vec = heimdall_player_input_GetTrackVector(input, cat);
    if (!vec)
        return NULL;
    assert(index < vec->size);
    return &vec->data[index]->t;
}

static struct heimdall_player_track_priv *
heimdall_player_GetPrivTrack(heimdall_player_t *player, heimdall_es_id_t *id)

{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return NULL;
    heimdall_player_track_vector *vec =
        heimdall_player_input_GetTrackVector(input, heimdall_es_id_GetCat(id));
    if (!vec)
        return NULL;
    return heimdall_player_track_vector_FindById(vec, id, NULL);
}

const struct heimdall_player_track *
heimdall_player_GetTrack(heimdall_player_t *player, heimdall_es_id_t *id)
{
    struct heimdall_player_track_priv *trackpriv =
        heimdall_player_GetPrivTrack(player, id);
    return trackpriv ? &trackpriv->t : NULL;
}

vout_thread_t *
heimdall_player_GetEsIdVout(heimdall_player_t *player, heimdall_es_id_t *es_id,
                       enum heimdall_vout_order *order)
{
    struct heimdall_player_track_priv *trackpriv =
        heimdall_player_GetPrivTrack(player, es_id);
    if (trackpriv)
    {
        if (order)
            *order = trackpriv->vout_order;
        return trackpriv->vout;
    }
    return NULL;
}

heimdall_es_id_t *
heimdall_player_GetEsIdFromVout(heimdall_player_t *player, vout_thread_t *vout)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return NULL;

    static const enum es_format_category_e cats[] = {
        VIDEO_ES, SPU_ES, AUDIO_ES /* for visualisation filters */
    };
    for (size_t i = 0; i < ARRAY_SIZE(cats); ++i)
    {
        enum es_format_category_e cat = cats[i];
        heimdall_player_track_vector *vec =
            heimdall_player_input_GetTrackVector(input, cat);
        for (size_t j = 0; j < vec->size; ++j)
        {
            struct heimdall_player_track_priv *trackpriv = vec->data[j];
            if (trackpriv->vout == vout)
                return trackpriv->t.es_id;
        }
    }
    return NULL;
}

unsigned
heimdall_player_SelectEsIdList(heimdall_player_t *player,
                          enum es_format_category_e cat,
                          heimdall_es_id_t *const es_id_list[])
{
    static const size_t max_tracks_by_cat[] = {
        [UNKNOWN_ES] = 0,
        [VIDEO_ES] = UINT_MAX,
        [AUDIO_ES] = 1,
        [SPU_ES] = 2,
        [DATA_ES] = 0,
    };

    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return 0;

    const size_t max_tracks = max_tracks_by_cat[cat];

    if (max_tracks == 0)
        return 0;

    /* First, count and hold all the ES Ids.
       Ids will be released in input.c:ControlRelease */
    size_t track_count = 0;
    for (size_t i = 0; es_id_list[i] != NULL && track_count < max_tracks; i++)
        if (heimdall_es_id_GetCat(es_id_list[i]) == cat)
            track_count++;

    /* Copy es_id_list into an allocated list so that it remains in memory until
       selection completes. The list will be freed in input.c:ControlRelease */
    struct heimdall_es_id_t **allocated_ids =
        heimdall_alloc(track_count + 1, sizeof(heimdall_es_id_t *));
    struct heimdall_es_id_t **osd_ids = /* create two copies, one for osd message */
        heimdall_alloc(track_count + 1, sizeof(heimdall_es_id_t *));

    if (allocated_ids == NULL || osd_ids == NULL)
        return 0;

    track_count = 0;
    for (size_t i = 0; es_id_list[i] != NULL && track_count < max_tracks; i++)
    {
        heimdall_es_id_t *es_id = es_id_list[i];
        if (heimdall_es_id_GetCat(es_id_list[i]) == cat)
        {
            heimdall_es_id_Hold(es_id);
            osd_ids[track_count] = es_id;
            allocated_ids[track_count++] = es_id;
        }
    }
    osd_ids[track_count] = NULL;
    allocated_ids[track_count] = NULL;

    /* Attempt to select all the requested tracks */
    int ret = input_ControlPush(input->thread, INPUT_CONTROL_SET_ES_LIST,
        &(input_control_param_t) {
            .list.cat = cat,
            .list.ids = allocated_ids,
        });
    if (ret != HEIMDALL_SUCCESS)
    {
        free(osd_ids);
        return 0;
    }

    /* Display track selection message */
    heimdall_player_osd_Tracks(player, osd_ids, NULL);
    free(osd_ids);

    return track_count;
}

/* Returns an array of selected tracks, putting id in first position (if any).
 * */
static heimdall_es_id_t **
heimdall_player_GetEsIdList(heimdall_player_t *player,
                       const enum es_format_category_e cat,
                       heimdall_es_id_t *id)
{
    const size_t track_count = heimdall_player_GetTrackCount(player, cat);
    if (track_count == 0)
        return NULL;

    size_t selected_track_count = id ? 1 : 0;
    for (size_t i = 0; i < track_count; ++i)
    {
        const struct heimdall_player_track *track =
            heimdall_player_GetTrackAt(player, cat, i);
        if (track->selected && track->es_id != id)
            selected_track_count++;
    }

    heimdall_es_id_t **es_id_list =
        heimdall_alloc(selected_track_count + 1 /* NULL */, sizeof(heimdall_es_id_t*));
    if (!es_id_list)
        return NULL;

    size_t es_id_list_idx = 0;
    /* Assure to select the requested track */
    if (id)
        es_id_list[es_id_list_idx++] = id;

    for (size_t i = 0; i < track_count; ++i)
    {
        const struct heimdall_player_track *track =
            heimdall_player_GetTrackAt(player, cat, i);
        if (track->selected && track->es_id != id)
            es_id_list[es_id_list_idx++] = track->es_id;
    }
    es_id_list[selected_track_count] = NULL;

    return es_id_list;
}

unsigned
heimdall_player_SelectEsId(heimdall_player_t *player, heimdall_es_id_t *id,
                      enum heimdall_player_select_policy policy)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return 0;

    if (policy == HEIMDALL_PLAYER_SELECT_EXCLUSIVE)
    {
        if (input_ControlPushEsHelper(input->thread, INPUT_CONTROL_SET_ES, id)
         == HEIMDALL_SUCCESS)
            heimdall_player_osd_Track(player, id, true);
        return 1;
    }

    /* HEIMDALL_PLAYER_SELECT_SIMULTANEOUS */
    const enum es_format_category_e cat = heimdall_es_id_GetCat(id);
    heimdall_es_id_t **es_id_list = heimdall_player_GetEsIdList(player, cat, id);
    if (!es_id_list)
        return 0;

    unsigned ret = heimdall_player_SelectEsIdList(player, cat, es_id_list);
    free(es_id_list);
    return ret;
}

void
heimdall_player_SelectTracksByStringIds(heimdall_player_t *player,
                                   enum es_format_category_e cat,
                                   const char *str_ids)
{
    heimdall_player_assert_locked(player);
    char **cat_str_ids;

    switch (cat)
    {
        case VIDEO_ES: cat_str_ids = &player->video_string_ids; break;
        case AUDIO_ES: cat_str_ids = &player->audio_string_ids; break;
        case SPU_ES:   cat_str_ids = &player->sub_string_ids; break;
        default: return;
    }

    free(*cat_str_ids);
    *cat_str_ids = str_ids ? strdup(str_ids) : NULL;

    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (input)
        heimdall_player_input_SelectTracksByStringIds(input, cat, str_ids);
}

void
heimdall_player_CycleTrack(heimdall_player_t *player, enum es_format_category_e cat,
                      enum heimdall_vout_order vout_order, bool next)
{
    size_t count = heimdall_player_GetTrackCount(player, cat);
    if (!count)
        return;

    heimdall_es_id_t *keep_id = NULL;

    /* Check how many tracks are already selected */
    size_t selected_count = 0;

    /* Find out the current selected index.
     * If no track selected, select the first or the last track */
    size_t cycle_index = next ? count-1 : count;

    for (size_t i = 0; i < count && selected_count < 2; ++i)
    {
        const struct heimdall_player_track *track =
            heimdall_player_GetTrackAt(player, cat, i);
        assert(track);

        if (track->selected)
        {
            enum heimdall_vout_order order;
            heimdall_player_GetEsIdVout(player, track->es_id, &order);
            if (order == vout_order)
                cycle_index = i;
            else
                keep_id = track->es_id;
            ++selected_count;
        }
    }

    heimdall_es_id_t * cycle_id = NULL;
    /* Look for the next free (unselected) track */
    for (size_t i = 0; i < count; ++i)
    {
        cycle_index = (cycle_index + (next ? 1 : -1) + count) % count;

        /* Unselect if we reach the end of the cycle
         * Unless cycling PRIMARY with a selected SECONDARY then wrap around */
        if (((next && cycle_index == 0) || (!next && cycle_index + 1 == count))
         && ((selected_count == 1 && vout_order == HEIMDALL_VOUT_ORDER_PRIMARY)
          || (selected_count == 2 && vout_order == HEIMDALL_VOUT_ORDER_SECONDARY)))
            break;

        const struct heimdall_player_track *track =
            heimdall_player_GetTrackAt(player, cat, cycle_index);
        if (!track->selected)
        {
            cycle_id = track->es_id;
            break;
        }
    }

    // We want the PRIMARY in first position
    heimdall_es_id_t *esIds[] = { cycle_id, keep_id, NULL };
    if (vout_order == HEIMDALL_VOUT_ORDER_SECONDARY)
    {
        esIds[0] = keep_id;
        esIds[1] = cycle_id;
    }
    if (!esIds[0])
    {
        esIds[0] = esIds[1];
        esIds[1] = NULL;
    }

    heimdall_player_SelectEsIdList(player, cat, esIds);
}

void
heimdall_player_UnselectEsId(heimdall_player_t *player, heimdall_es_id_t *id)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;

    int ret = input_ControlPushEsHelper(input->thread, INPUT_CONTROL_UNSET_ES,
                                        id);
    if (ret != HEIMDALL_SUCCESS)
        return;

    const enum es_format_category_e cat = heimdall_es_id_GetCat(id);
    heimdall_es_id_t **selected_es = heimdall_player_GetEsIdList(player, cat, NULL);
    if (selected_es == NULL)
        return;

    heimdall_player_osd_Tracks(player, selected_es, id);
    free(selected_es);
}

void
heimdall_player_RestartEsId(heimdall_player_t *player, heimdall_es_id_t *id)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (input)
        input_ControlPushEsHelper(input->thread, INPUT_CONTROL_RESTART_ES, id);
}

void
heimdall_player_SelectCategoryLanguage(heimdall_player_t *player,
                                  enum es_format_category_e cat,
                                  const char *lang)
{
    heimdall_player_assert_locked(player);
    switch (cat)
    {
        case AUDIO_ES:
            var_SetString(player, "audio-language", lang);
            break;
        case SPU_ES:
            var_SetString(player, "sub-language", lang);
            break;
        default:
            heimdall_assert_unreachable();
    }
}

char *
heimdall_player_GetCategoryLanguage(heimdall_player_t *player,
                               enum es_format_category_e cat)
{
    heimdall_player_assert_locked(player);
    switch (cat)
    {
        case AUDIO_ES:
            return var_GetString(player, "audio-language");
        case SPU_ES:
            return var_GetString(player, "sub-language");
        default:
            heimdall_assert_unreachable();
    }
}

void
heimdall_player_SetTeletextEnabled(heimdall_player_t *player, bool enabled)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input || !input->teletext_source)
        return;
    if (enabled)
        heimdall_player_SelectEsId(player, input->teletext_source->t.es_id,
                              HEIMDALL_PLAYER_SELECT_EXCLUSIVE);
    else
        heimdall_player_UnselectEsId(player, input->teletext_source->t.es_id);
}

void
heimdall_player_SelectTeletextPage(heimdall_player_t *player, unsigned page)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input || !input->teletext_source)
        return;
    input_ControlPush(input->thread, INPUT_CONTROL_SET_VBI_PAGE,
        &(input_control_param_t) {
            .vbi_page.id = input->teletext_source->t.es_id,
            .vbi_page.page = page,
    });
}

void
heimdall_player_SetTeletextTransparency(heimdall_player_t *player, bool enabled)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input || !input->teletext_source)
        return;

    input_ControlPush(input->thread, INPUT_CONTROL_SET_VBI_TRANSPARENCY,
        &(input_control_param_t) {
            .vbi_transparency.id = input->teletext_source->t.es_id,
            .vbi_transparency.enabled = enabled,
    });
}

bool
heimdall_player_HasTeletextMenu(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    return input && input->teletext_source;
}

bool
heimdall_player_IsTeletextEnabled(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (input && input->teletext_enabled)
    {
        assert(input->teletext_source);
        return true;
    }
    return false;
}

unsigned
heimdall_player_GetTeletextPage(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    return heimdall_player_IsTeletextEnabled(player) ? input->teletext_page : 0;
}

bool
heimdall_player_IsTeletextTransparent(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    return heimdall_player_IsTeletextEnabled(player) && input->teletext_transparent;
}

struct heimdall_player_title_list *
heimdall_player_GetTitleList(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    return input ? input->titles : NULL;
}

ssize_t
heimdall_player_GetSelectedTitleIdx(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return -1;
    return input->title_selected;
}

static ssize_t
heimdall_player_GetTitleIdx(heimdall_player_t *player,
                       const struct heimdall_player_title *title)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (input && input->titles)
        for (size_t i = 0; i < input->titles->count; ++i)
            if (&input->titles->array[i] == title)
                return i;
    return -1;
}

void
heimdall_player_SelectTitleIdx(heimdall_player_t *player, size_t index)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (input)
        input_ControlPushHelper(input->thread, INPUT_CONTROL_SET_TITLE,
                                &(heimdall_value_t){ .i_int = index });
}

void
heimdall_player_SelectTitle(heimdall_player_t *player,
                       const struct heimdall_player_title *title)
{
    ssize_t idx = heimdall_player_GetTitleIdx(player, title);
    if (idx != -1)
        heimdall_player_SelectTitleIdx(player, idx);
}

void
heimdall_player_SelectChapter(heimdall_player_t *player,
                         const struct heimdall_player_title *title,
                         size_t chapter_idx)
{
    ssize_t idx = heimdall_player_GetTitleIdx(player, title);
    if (idx != -1 && idx == heimdall_player_GetSelectedTitleIdx(player))
        heimdall_player_SelectChapterIdx(player, chapter_idx);
}

void
heimdall_player_SelectNextTitle(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;
    int ret = input_ControlPush(input->thread, INPUT_CONTROL_SET_TITLE_NEXT,
                                NULL);
    if (ret == HEIMDALL_SUCCESS)
        heimdall_player_osd_Message(player, _("Next title"));
}

void
heimdall_player_SelectPrevTitle(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;
    int ret = input_ControlPush(input->thread, INPUT_CONTROL_SET_TITLE_PREV,
                                NULL);
    if (ret == HEIMDALL_SUCCESS)
        heimdall_player_osd_Message(player, _("Previous title"));
}

ssize_t
heimdall_player_GetSelectedChapterIdx(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return -1;
    return input->chapter_selected;
}

static heimdall_tick_t
heimdall_player_ChapterTime(struct heimdall_player_input *input, size_t index)
{
    if (!input->titles)
        return -1;
    const struct heimdall_player_title *title =
        &input->titles->array[input->title_selected];
    if (index >= title->chapter_count)
        return -1;
    return title->chapters[index].time;
}

/* move the timer right away so the seek bar does not wait for new data */
static void
heimdall_player_UpdateChapterSeekState(struct heimdall_player_input *input, size_t index)
{
    heimdall_tick_t time = heimdall_player_ChapterTime(input, index);
    if (time < 0)
        return;
    if (time < HEIMDALL_TICK_0)
        time = HEIMDALL_TICK_0;
    heimdall_player_UpdateTimerSeekState(input->player, time, -1);
}

void
heimdall_player_SelectChapterIdx(heimdall_player_t *player, size_t index)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;
    heimdall_player_UpdateChapterSeekState(input, index);
    int ret = input_ControlPushHelper(input->thread, INPUT_CONTROL_SET_SEEKPOINT,
                                      &(heimdall_value_t){ .i_int = index });
    if (ret == HEIMDALL_SUCCESS)
        heimdall_player_osd_Message(player, _("Chapter %ld"), index);
}

void
heimdall_player_SelectNextChapter(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;
    heimdall_player_UpdateChapterSeekState(input, input->chapter_selected + 1);
    int ret = input_ControlPush(input->thread, INPUT_CONTROL_SET_SEEKPOINT_NEXT,
                                NULL);
    if (ret == HEIMDALL_SUCCESS)
        heimdall_player_osd_Message(player, _("Next chapter"));
}

void
heimdall_player_SelectPrevChapter(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;
    size_t index = input->chapter_selected;
    heimdall_tick_t time = heimdall_player_ChapterTime(input, index);
    /* same rule as the input thread */
    if (time >= 0 && heimdall_player_input_GetTime(input, true, heimdall_tick_now())
                     >= time + HEIMDALL_TICK_FROM_SEC(3))
        heimdall_player_UpdateChapterSeekState(input, index);
    else if (index > 0)
        heimdall_player_UpdateChapterSeekState(input, index - 1);
    int ret = input_ControlPush(input->thread, INPUT_CONTROL_SET_SEEKPOINT_PREV,
                                NULL);
    if (ret == HEIMDALL_SUCCESS)
        heimdall_player_osd_Message(player, _("Previous chapter"));
}

void
heimdall_player_Lock(heimdall_player_t *player)
{
    /* Metadata, Vout and aout locks should not be held, cf.
     * heimdall_player_metadata_cbs, heimdall_player_vout_cbs and heimdall_player_aout_cbs
     * documentation */
    assert(!heimdall_mutex_held(&player->metadata_listeners_lock));
    assert(!heimdall_mutex_held(&player->vout_listeners_lock));
    assert(!heimdall_mutex_held(&player->aout_listeners_lock));
    /* The timer lock should not be held (possible lock-order-inversion), cf.
     * heimdall_player_timer_cbs.on_update documentation */
    assert(!heimdall_mutex_held(&player->timer.lock));

    heimdall_mutex_lock(&player->lock);
}

void
heimdall_player_Unlock(heimdall_player_t *player)
{
    heimdall_mutex_unlock(&player->lock);
}

void
heimdall_player_CondWait(heimdall_player_t *player, heimdall_cond_t *cond)
{
    heimdall_player_assert_locked(player);
    heimdall_cond_wait(cond, &player->lock);
}

heimdall_player_listener_id *
heimdall_player_AddListener(heimdall_player_t *player,
                       const struct heimdall_player_cbs *cbs, void *cbs_data)
{
    assert(cbs);
    heimdall_player_assert_locked(player);

    heimdall_player_listener_id *listener = malloc(sizeof(*listener));
    if (!listener)
        return NULL;

    listener->cbs = cbs;
    listener->cbs_data = cbs_data;

    heimdall_list_append(&listener->node, &player->listeners);

    return listener;
}

void
heimdall_player_RemoveListener(heimdall_player_t *player,
                          heimdall_player_listener_id *id)
{
    assert(id);
    heimdall_player_assert_locked(player);

    heimdall_list_remove(&id->node);
    free(id);
}

static void
heimdall_player_InvalidateNextMedia(heimdall_player_t *player)
{
    heimdall_player_assert_locked(player);
    if (player->next_media)
    {
        input_item_Release(player->next_media);
        player->next_media = NULL;
    }
}

int
heimdall_player_SetCurrentMedia(heimdall_player_t *player, input_item_t *media)
{
    heimdall_player_assert_locked(player);

    heimdall_player_CancelWaitError(player);

    heimdall_player_InvalidateNextMedia(player);

    if (media)
    {
        /* Switch to this new media when the current input is stopped */
        player->next_media = input_item_Hold(media);
        player->releasing_media = false;
    }
    else if (player->media)
    {
        /* The current media will be set to NULL once the current input is
         * stopped */
        player->releasing_media = true;
    }
    else
        return HEIMDALL_SUCCESS;

    if (player->input)
    {
        heimdall_player_destructor_AddInput(player, player->input);
        player->input = NULL;
    }

    assert(media == player->next_media);
    if (!heimdall_player_destructor_IsEmpty(player))
    {
        /* This media will be opened when the input is finally stopped */
        return HEIMDALL_SUCCESS;
    }

    /* We can switch to the next media directly */
    return heimdall_player_OpenNextMedia(player);
}

void
heimdall_player_SetNextMedia(heimdall_player_t *player, input_item_t *media)
{
    heimdall_player_assert_locked(player);

    /* Order is important, hold the new media before releasing the old one */
    input_item_t *next_media = media != NULL ? input_item_Hold(media) : NULL;

    if (player->next_media != NULL)
        input_item_Release(player->next_media);

    player->next_media = next_media;
}

input_item_t *
heimdall_player_GetNextMedia(heimdall_player_t *player)
{
    heimdall_player_assert_locked(player);

    return player->next_media;
}

input_item_t *
heimdall_player_GetCurrentMedia(heimdall_player_t *player)
{
    heimdall_player_assert_locked(player);

    return player->media;
}

int
heimdall_player_AddAssociatedMedia(heimdall_player_t *player,
                              enum es_format_category_e cat, const char *uri,
                              bool select, bool notify, bool check_ext)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input || !uri)
        return HEIMDALL_EGENERIC;

    enum slave_type type;
    switch (cat)
    {
        case SPU_ES:
            type = SLAVE_TYPE_SPU;
            break;
        default:
            type = SLAVE_TYPE_GENERIC;
            break;
    }

    if (check_ext && type == SLAVE_TYPE_SPU && !subtitles_Filter(uri))
        return HEIMDALL_EGENERIC;

    input_item_slave_t *slave =
        input_item_slave_New(uri, type, SLAVE_PRIORITY_USER);
    if (!slave)
        return HEIMDALL_ENOMEM;
    slave->b_forced = select;

    heimdall_value_t val = { .p_address = slave };
    int ret = input_ControlPushHelper(input->thread, INPUT_CONTROL_ADD_SLAVE,
                                      &val);
    if (ret != HEIMDALL_SUCCESS)
        return ret;

    if (notify)
    {
        switch( type )
        {
            case SLAVE_TYPE_GENERIC:
                heimdall_player_osd_Message(player, "%s",
                                       heimdall_gettext("Slave added"));
                break;
            case SLAVE_TYPE_SPU:
                heimdall_player_osd_Message(player, "%s",
                                       heimdall_gettext("Subtitle slave added"));
                break;
        }
    }
    return HEIMDALL_SUCCESS;
}

void
heimdall_player_SetAssociatedSubsFPS(heimdall_player_t *player, float fps)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    var_SetFloat(player, "sub-fps", fps);
    if (input)
        input_ControlPushHelper(input->thread, INPUT_CONTROL_SET_SUBS_FPS,
                                &(heimdall_value_t) { .f_float = fps });
    heimdall_player_SendEvent(player, on_associated_subs_fps_changed, fps);
}

float
heimdall_player_GetAssociatedSubsFPS(heimdall_player_t *player)
{
    heimdall_player_assert_locked(player);
    return var_GetFloat(player, "sub-fps");
}

int
heimdall_player_Start(heimdall_player_t *player)
{
    heimdall_player_assert_locked(player);

    heimdall_player_CancelWaitError(player);

    if (player->started)
        return HEIMDALL_SUCCESS;

    if (!heimdall_player_destructor_IsEmpty(player))
    {
        if (player->next_media)
        {
            player->started = true;
            return HEIMDALL_SUCCESS;
        }
        else if (unlikely(player->media != NULL))
        {
            /* The current media is being stopped while the user requested to
             * play it again. Tell the thread to play the same media when
             * ready. */
            player->started = true;
            player->next_media = input_item_Hold(player->media);
            player->releasing_media = false;
            return HEIMDALL_SUCCESS;
        }
        else
            return HEIMDALL_EGENERIC;
    }

    if (!player->media)
    {
        if (player->next_media != NULL)
        {
            /* The user called only SetNextMedia() and not SetCurrentMedia(),
             * so open the next media directly from here. */
            int ret = heimdall_player_OpenNextMedia(player);
            if (ret != HEIMDALL_SUCCESS)
                return ret;
        }
        else
            return HEIMDALL_EGENERIC;
    }

    if (!player->input)
    {
        /* Possible if the player was stopped by the user */
        player->input = heimdall_player_input_New(player, player->media);

        if (!player->input)
            return HEIMDALL_ENOMEM;
    }
    assert(!player->input->started);

    if (player->start_paused)
    {
        var_Create(player->input->thread, "start-paused", HEIMDALL_VAR_BOOL);
        var_SetBool(player->input->thread, "start-paused", true);
    }

    int ret = heimdall_player_input_Start(player->input);
    if (ret == HEIMDALL_SUCCESS)
    {
        player->started = true;
        heimdall_player_osd_Icon(player, OSD_PLAY_ICON);
    }
    return ret;
}

int
heimdall_player_Stop(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    heimdall_player_CancelWaitError(player);

    heimdall_player_InvalidateNextMedia(player);

    if (!input || !player->started)
        return HEIMDALL_EGENERIC;
    player->started = false;

    heimdall_player_destructor_AddInput(player, input);
    player->input = NULL;
    return HEIMDALL_SUCCESS;
}

void
heimdall_player_SetStartPaused(heimdall_player_t *player, bool start_paused)
{
    heimdall_player_assert_locked(player);
    player->start_paused = start_paused;
}

void
heimdall_player_SetPlayAndPause(heimdall_player_t *player, bool play_and_pause)
{
    heimdall_player_assert_locked(player);
    player->play_and_pause = play_and_pause;
}

void
heimdall_player_SetRepeatCount(heimdall_player_t *player, unsigned repeat_count)
{
    heimdall_player_assert_locked(player);
    player->repeat = repeat_count;
}

static void
heimdall_player_SetPause(heimdall_player_t *player, bool pause)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input || !input->started)
        return;

    heimdall_value_t val = { .i_int = pause ? PAUSE_S : PLAYING_S };
    int ret = input_ControlPushHelper(input->thread, INPUT_CONTROL_SET_STATE,
                                      &val);

    if (ret == HEIMDALL_SUCCESS)
        heimdall_player_osd_Icon(player, pause ? OSD_PAUSE_ICON : OSD_PLAY_ICON);
}

void
heimdall_player_Pause(heimdall_player_t *player)
{
    heimdall_player_SetPause(player, true);
}

void
heimdall_player_Resume(heimdall_player_t *player)
{
    heimdall_player_SetPause(player, false);
}

void
heimdall_player_NextVideoFrame(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;
    input_ControlPushHelper(input->thread,
                            INPUT_CONTROL_SET_FRAME_NEXT, NULL);
}

void
heimdall_player_PreviousVideoFrame(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;
    input_ControlPushHelper(input->thread,
                            INPUT_CONTROL_SET_FRAME_PREVIOUS, NULL);
}

enum heimdall_player_state
heimdall_player_GetState(heimdall_player_t *player)
{
    heimdall_player_assert_locked(player);
    return player->global_state;
}

enum heimdall_player_error
heimdall_player_GetError(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    return input ? input->error : HEIMDALL_PLAYER_ERROR_NONE;
}

int
heimdall_player_GetCapabilities(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    return input ? input->capabilities : 0;
}

float
heimdall_player_GetRate(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (input)
        return input->rate;
    else
        return var_GetFloat(player, "rate");
}

void
heimdall_player_ChangeRate(heimdall_player_t *player, float rate)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (rate == 0.0)
        return;

    /* Save rate across inputs */
    var_SetFloat(player, "rate", rate);

    /* The event is sent from the thread processing the control */
    if (input
     && input_ControlPushHelper(input->thread, INPUT_CONTROL_SET_RATE,
                                &(heimdall_value_t) { .f_float = rate }) == HEIMDALL_SUCCESS)
        heimdall_player_osd_Message(player, ("Speed: %.2fx"), rate);
    else /* Send the event anyway since it's a global state */
        heimdall_player_SendEvent(player, on_rate_changed, rate);

}

static void
heimdall_player_ChangeRateOffset(heimdall_player_t *player, bool increment)
{
    static const float rates[] = {
        1.0f/64, 1.0f/32, 1.0f/16, 1.0f/8, 1.0f/4, 1.0f/3, 1.0f/2, 2.0f/3,
        1.0f/1,
        3.0f/2, 2.0f/1, 3.0f/1, 4.0f/1, 8.0f/1, 16.0f/1, 32.0f/1, 64.0f/1,
    };
    float rate = heimdall_player_GetRate(player) * (increment ? 1.1f : 0.9f);

    /* find closest rate (if any) in the desired direction */
    for (size_t i = 0; i < ARRAY_SIZE(rates); ++i)
    {
        if ((increment && rates[i] > rate) ||
            (!increment && rates[i] >= rate && i))
        {
            rate = increment ? rates[i] : rates[i-1];
            break;
        }
    }

    heimdall_player_ChangeRate(player, rate);
}

void
heimdall_player_IncrementRate(heimdall_player_t *player)
{
    heimdall_player_ChangeRateOffset(player, true);
}

void
heimdall_player_DecrementRate(heimdall_player_t *player)
{
    heimdall_player_ChangeRateOffset(player, false);
}

heimdall_tick_t
heimdall_player_GetLength(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    return input ? input->length : HEIMDALL_TICK_INVALID;
}

heimdall_tick_t
heimdall_player_GetTime(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return HEIMDALL_TICK_INVALID;

    return heimdall_player_input_GetTime(input, false, heimdall_tick_now());
}

double
heimdall_player_GetPosition(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    return input ? heimdall_player_input_GetPos(input, false, heimdall_tick_now()) : -1.f;
}

void
heimdall_player_DisplayPosition(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;

    heimdall_tick_t now = heimdall_tick_now();
    heimdall_player_osd_Position(player, input,
                            heimdall_player_input_GetTime(input, false, now),
                            heimdall_player_input_GetPos(input, false, now));
}

void
heimdall_player_SeekByPos(heimdall_player_t *player, double position,
                     enum heimdall_player_seek_speed speed,
                     enum heimdall_player_whence whence)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (input != NULL)
        heimdall_player_input_SeekByPos(input, position, speed, whence);
}

void
heimdall_player_SeekByTime(heimdall_player_t *player, heimdall_tick_t time,
                      enum heimdall_player_seek_speed speed,
                      enum heimdall_player_whence whence)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (input != NULL)
        heimdall_player_input_SeekByTime(input, time, speed, whence);
}

void
heimdall_player_SetRenderer(heimdall_player_t *player, heimdall_renderer_item_t *renderer)
{
    heimdall_player_assert_locked(player);

    if (player->renderer)
        heimdall_renderer_item_release(player->renderer);
    player->renderer = renderer ? heimdall_renderer_item_hold(renderer) : NULL;

    heimdall_player_foreach_inputs(input)
    {
        heimdall_value_t val = {
            .p_address = renderer ? heimdall_renderer_item_hold(renderer) : NULL
        };
        input_ControlPushHelper(input->thread, INPUT_CONTROL_SET_RENDERER,
                                &val);
    }
    heimdall_player_SendEvent(player, on_renderer_changed, player->renderer);
}

heimdall_renderer_item_t *
heimdall_player_GetRenderer(heimdall_player_t *player)
{
    heimdall_player_assert_locked(player);
    return player->renderer;
}

int
heimdall_player_SetAtoBLoop(heimdall_player_t *player, enum heimdall_player_abloop abloop)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input )
        return HEIMDALL_EINVAL;

    heimdall_tick_t time = heimdall_player_GetTime(player);
    float pos = heimdall_player_GetPosition(player);
    int ret = HEIMDALL_SUCCESS;
    switch (abloop)
    {
        case HEIMDALL_PLAYER_ABLOOP_A:
            if (input->abloop_state[1].set)
                return HEIMDALL_EGENERIC;
            input->abloop_state[0].time = time;
            input->abloop_state[0].pos = pos;
            input->abloop_state[0].set = true;
            break;
        case HEIMDALL_PLAYER_ABLOOP_B:
            if (!heimdall_player_CanSeek(player))
            {
                input->abloop_state[0].set = false;
                return HEIMDALL_EGENERIC;
            }

            if (!input->abloop_state[0].set)
                return HEIMDALL_EGENERIC;
            input->abloop_state[1].time = time;
            input->abloop_state[1].pos = pos;
            input->abloop_state[1].set = true;
            if (input->abloop_state[0].time != HEIMDALL_TICK_INVALID
             && time != HEIMDALL_TICK_INVALID)
            {
                if (time > input->abloop_state[0].time)
                {
                    heimdall_player_SetTime(player, input->abloop_state[0].time);
                    break;
                }
            }
            else if (pos > input->abloop_state[0].pos)
            {
                heimdall_player_SetPosition(player, input->abloop_state[0].pos);
                break;
            }

            /* Error: A time is superior to B time. */
            abloop = HEIMDALL_PLAYER_ABLOOP_NONE;
            ret = HEIMDALL_EGENERIC;
            /* fall-through */
        case HEIMDALL_PLAYER_ABLOOP_NONE:
            input->abloop_state[0].set = input->abloop_state[1].set = false;
            time = HEIMDALL_TICK_INVALID;
            pos = 0.f;
            break;
        default:
            heimdall_assert_unreachable();
    }
    heimdall_player_SendEvent(player, on_atobloop_changed, abloop, time, pos);
    return ret;
}

int
heimdall_player_SetAtoBLoopTime(heimdall_player_t *player, heimdall_tick_t a_time, heimdall_tick_t b_time)
{
    if (a_time < 0 || b_time < 0)
        return HEIMDALL_EINVAL;

    if (a_time == HEIMDALL_TICK_INVALID || b_time == HEIMDALL_TICK_INVALID)
        return HEIMDALL_EINVAL;

    if (b_time <= a_time)
        return HEIMDALL_EINVAL;

    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return HEIMDALL_EINVAL;

    input->abloop_state[0].time = a_time;
    input->abloop_state[0].pos = 0;
    input->abloop_state[0].set = true;
    input->abloop_state[1].time = b_time;
    input->abloop_state[1].pos = 0;
    input->abloop_state[1].set = true;

    heimdall_tick_t current = heimdall_player_input_GetTime(input, false, heimdall_tick_now());
    if (current == HEIMDALL_TICK_INVALID || current < a_time || current > b_time)
        heimdall_player_SetTime(player, a_time);

    heimdall_player_SendEvent(player, on_atobloop_changed, HEIMDALL_PLAYER_ABLOOP_A, a_time, 0);
    heimdall_player_SendEvent(player, on_atobloop_changed, HEIMDALL_PLAYER_ABLOOP_B, b_time, 0);
    return HEIMDALL_SUCCESS;
}

int
heimdall_player_SetAtoBLoopPosition(heimdall_player_t *player, double a_pos, double b_pos)
{
    if (a_pos < 0 || a_pos > 1.0 || b_pos < 0 || b_pos > 1.0)
        return HEIMDALL_EINVAL;

    if (b_pos <= a_pos)
        return HEIMDALL_EINVAL;

    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return HEIMDALL_EINVAL;

    input->abloop_state[0].time = HEIMDALL_TICK_INVALID;
    input->abloop_state[0].pos = a_pos;
    input->abloop_state[0].set = true;
    input->abloop_state[1].time = HEIMDALL_TICK_INVALID;
    input->abloop_state[1].pos = b_pos;
    input->abloop_state[1].set = true;

    double current = heimdall_player_input_GetPos(input, false, heimdall_tick_now());
    if (current < a_pos || current > b_pos)
        heimdall_player_SetPosition(player, a_pos);

    heimdall_player_SendEvent(player, on_atobloop_changed, HEIMDALL_PLAYER_ABLOOP_A, HEIMDALL_TICK_INVALID, a_pos);
    heimdall_player_SendEvent(player, on_atobloop_changed, HEIMDALL_PLAYER_ABLOOP_B, HEIMDALL_TICK_INVALID, b_pos);
    return HEIMDALL_SUCCESS;
}

int
heimdall_player_ResetAtoBLoop(heimdall_player_t *player) {
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input || !heimdall_player_CanSeek(player))
        return HEIMDALL_EGENERIC;

    input->abloop_state[0].time = HEIMDALL_TICK_INVALID;
    input->abloop_state[0].pos = 0;
    input->abloop_state[0].set = false;
    input->abloop_state[1].time = HEIMDALL_TICK_INVALID;
    input->abloop_state[1].pos = 0;
    input->abloop_state[1].set = false;

    heimdall_player_SendEvent(player, on_atobloop_changed, HEIMDALL_PLAYER_ABLOOP_NONE, HEIMDALL_TICK_INVALID, 0);
    return HEIMDALL_SUCCESS;
}

enum heimdall_player_abloop
heimdall_player_GetAtoBLoop(heimdall_player_t *player, heimdall_tick_t *a_time, double *a_pos,
                       heimdall_tick_t *b_time, double *b_pos)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input || !heimdall_player_CanSeek(player) || !input->abloop_state[0].set)
        return HEIMDALL_PLAYER_ABLOOP_NONE;

    if (a_time)
        *a_time = input->abloop_state[0].time;
    if (a_pos)
        *a_pos = input->abloop_state[0].pos;
    if (!input->abloop_state[1].set)
        return HEIMDALL_PLAYER_ABLOOP_A;

    if (b_time)
        *b_time = input->abloop_state[1].time;
    if (b_pos)
        *b_pos = input->abloop_state[1].pos;
    return HEIMDALL_PLAYER_ABLOOP_B;
}

void
heimdall_player_Navigate(heimdall_player_t *player, enum heimdall_player_nav nav)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (!input)
        return;

    enum input_control_e control;
    switch (nav)
    {
        case HEIMDALL_PLAYER_NAV_ACTIVATE:
            control = INPUT_CONTROL_NAV_ACTIVATE;
            break;
        case HEIMDALL_PLAYER_NAV_UP:
            control = INPUT_CONTROL_NAV_UP;
            break;
        case HEIMDALL_PLAYER_NAV_DOWN:
            control = INPUT_CONTROL_NAV_DOWN;
            break;
        case HEIMDALL_PLAYER_NAV_LEFT:
            control = INPUT_CONTROL_NAV_LEFT;
            break;
        case HEIMDALL_PLAYER_NAV_RIGHT:
            control = INPUT_CONTROL_NAV_RIGHT;
            break;
        case HEIMDALL_PLAYER_NAV_POPUP:
            control = INPUT_CONTROL_NAV_POPUP;
            break;
        case HEIMDALL_PLAYER_NAV_MENU:
            control = INPUT_CONTROL_NAV_MENU;
            break;
        default:
            heimdall_assert_unreachable();
    }
    input_ControlPushHelper(input->thread, control, NULL);
}

void
heimdall_player_UpdateViewpoint(heimdall_player_t *player,
                           const heimdall_viewpoint_t *viewpoint,
                           enum heimdall_player_whence whence)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (input != NULL)
        heimdall_player_input_UpdateViewpoint(input, viewpoint, whence);
}

bool
heimdall_player_IsRecording(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    return input ? input->recording : false;
}

void
heimdall_player_SetRecordingEnabled(heimdall_player_t *player, bool enable,
                               const char *dir_path_)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return;
    char *dir_path;
    if (dir_path_ != NULL)
    {
        dir_path = strdup(dir_path_);
        if (dir_path == NULL)
            return;
    }
    else
        dir_path = NULL;

    const input_control_param_t param = { .record_state = { enable, dir_path } };
    int ret = input_ControlPush(input->thread,
                                INPUT_CONTROL_SET_RECORD_STATE, &param);

    if (ret == HEIMDALL_SUCCESS)
        heimdall_player_osd_Message(player, enable ?
                               _("Recording") : _("Recording done"));
}

int
heimdall_player_SetCategoryDelay(heimdall_player_t *player, enum es_format_category_e cat,
                            heimdall_tick_t delay, enum heimdall_player_whence whence)
{
    bool absolute = whence == HEIMDALL_PLAYER_WHENCE_ABSOLUTE;
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return HEIMDALL_EGENERIC;

    if (cat != AUDIO_ES && cat != SPU_ES && cat != VIDEO_ES)
        return HEIMDALL_EGENERIC;
    heimdall_tick_t *cat_delay = &input->cat_delays[cat];

    if (absolute)
        *cat_delay = delay;
    else
    {
        *cat_delay += delay;
        delay = *cat_delay;
    }

    int ret = input_SetEsCatDelay(input->thread, cat, delay);
    if (ret == HEIMDALL_SUCCESS)
    {
        heimdall_player_osd_Message(player, _("%s delay: %i ms"),
                               es_format_category_to_string(cat),
                               (int)MS_FROM_HEIMDALL_TICK(delay));
        heimdall_player_SendEvent(player, on_category_delay_changed, cat, delay);
    }
    return HEIMDALL_SUCCESS;
}

heimdall_tick_t
heimdall_player_GetCategoryDelay(heimdall_player_t *player, enum es_format_category_e cat)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return 0;

    if (cat != AUDIO_ES && cat != SPU_ES)
        return 0;

    return input->cat_delays[cat];
}

int
heimdall_player_SetEsIdDelay(heimdall_player_t *player, heimdall_es_id_t *es_id,
                        heimdall_tick_t delay, enum heimdall_player_whence whence)
{
    bool absolute = whence == HEIMDALL_PLAYER_WHENCE_ABSOLUTE;
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return HEIMDALL_EGENERIC;

    struct heimdall_player_track_priv *trackpriv =
        heimdall_player_input_FindTrackById(input, es_id, NULL);
    if (trackpriv == NULL ||
        (trackpriv->t.fmt.i_cat != AUDIO_ES && trackpriv->t.fmt.i_cat != SPU_ES))
        return HEIMDALL_EGENERIC;

    if (absolute)
        trackpriv->delay = delay;
    else
    {
        if (trackpriv->delay == HEIMDALL_TICK_MAX)
            trackpriv->delay = 0;
        trackpriv->delay += delay;
        delay = trackpriv->delay;
    }

    const input_control_param_t param = { .es_delay = { es_id, delay } };
    int ret = input_ControlPush(input->thread, INPUT_CONTROL_SET_ES_DELAY,
                                &param);
    if (ret == HEIMDALL_SUCCESS)
    {
        if (delay != HEIMDALL_TICK_MAX)
            heimdall_player_osd_Message(player, _("%s delay: %i ms"),
                                   trackpriv->t.name,
                                   (int)MS_FROM_HEIMDALL_TICK(delay));
        heimdall_player_SendEvent(player, on_track_delay_changed, es_id, delay);
    }

    return HEIMDALL_SUCCESS;
}

heimdall_tick_t
heimdall_player_GetEsIdDelay(heimdall_player_t *player, heimdall_es_id_t *es_id)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    if (!input)
        return 0;

    struct heimdall_player_track_priv *trackpriv =
        heimdall_player_input_FindTrackById(input, es_id, NULL);
    return trackpriv ? trackpriv->delay : HEIMDALL_TICK_MAX;
}

static struct {
    const char var[sizeof("video")];
    const char sout_var[sizeof("sout-video")];
} cat2vars[] = {
    [VIDEO_ES] = { "video", "sout-video" },
    [AUDIO_ES] = { "audio", "sout-audio" },
    [SPU_ES] = { "spu", "sout-spu" },
};

void
heimdall_player_SetTrackCategoryEnabled(heimdall_player_t *player,
                                   enum es_format_category_e cat, bool enabled)
{
    assert(cat >= UNKNOWN_ES && cat <= DATA_ES);
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    var_SetBool(player, cat2vars[cat].var, enabled);
    var_SetBool(player, cat2vars[cat].sout_var, enabled);

    if (input)
    {
        var_SetBool(input->thread, cat2vars[cat].var, enabled);
        var_SetBool(input->thread, cat2vars[cat].sout_var, enabled);

        if (!enabled)
            heimdall_player_UnselectTrackCategory(player, cat);
    }
}

bool
heimdall_player_IsTrackCategoryEnabled(heimdall_player_t *player,
                                  enum es_format_category_e cat)
{
    assert(cat >= UNKNOWN_ES && cat <= DATA_ES);
    return var_GetBool(player, cat2vars[cat].var);
}

void
heimdall_player_SetSubtitleTextScale(heimdall_player_t *player, unsigned scale)
{
    assert(scale >= 10 && scale <= 500);
    var_SetInteger(player, "sub-text-scale", scale);
}

unsigned
heimdall_player_GetSubtitleTextScale(heimdall_player_t *player)
{
    return var_GetInteger(player, "sub-text-scale");
}

int
heimdall_player_GetSignal(heimdall_player_t *player, float *quality, float *strength)
{
    assert(quality && strength);
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    if (input && input->signal_quality >= 0 && input->signal_strength >= 0)
    {
        *quality = input->signal_quality;
        *strength = input->signal_strength;
        return HEIMDALL_SUCCESS;
    }
    return HEIMDALL_EGENERIC;
}

const struct input_stats_t *
heimdall_player_GetStatistics(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);

    return input ? &input->stats : NULL;
}

void
heimdall_player_SetPauseOnCork(heimdall_player_t *player, bool enabled)
{
    heimdall_player_assert_locked(player);
    player->pause_on_cork = enabled;
}

static int
heimdall_player_CorkCallback(heimdall_object_t *this, const char *var,
                        heimdall_value_t oldval, heimdall_value_t newval, void *data)
{
    heimdall_player_t *player = data;

    if (oldval.i_int == newval.i_int )
        return HEIMDALL_SUCCESS;

    heimdall_player_Lock(player);

    if (player->pause_on_cork)
    {
        if (newval.i_int)
        {
            player->corked = player->global_state == HEIMDALL_PLAYER_STATE_PLAYING
                          || player->global_state == HEIMDALL_PLAYER_STATE_STARTED;
            if (player->corked)
                heimdall_player_Pause(player);
        }
        else
        {
            if (player->corked)
            {
                heimdall_player_Resume(player);
                player->corked = false;
            }
        }
    }
    else
        heimdall_player_SendEvent(player, on_cork_changed, newval.i_int);

    heimdall_player_Unlock(player);

    (void) this; (void) var;
    return HEIMDALL_SUCCESS;
}

heimdall_object_t *
heimdall_player_GetV4l2Object(heimdall_player_t *player)
{
    struct heimdall_player_input *input = heimdall_player_get_input_locked(player);
    return input && var_Type(input->thread, "controls") != 0 ?
           HEIMDALL_OBJECT(input->thread) : NULL;
}

static void
heimdall_player_InitLocks(heimdall_player_t *player, enum heimdall_player_lock_type lock_type)
{
    if (lock_type == HEIMDALL_PLAYER_LOCK_REENTRANT)
        heimdall_mutex_init_recursive(&player->lock);
    else
        heimdall_mutex_init(&player->lock);

    heimdall_mutex_init(&player->metadata_listeners_lock);
    heimdall_mutex_init(&player->vout_listeners_lock);
    heimdall_mutex_init(&player->aout_listeners_lock);
    heimdall_cond_init(&player->start_delay_cond);
    heimdall_cond_init(&player->mainloop.wait);
}

void
heimdall_player_Delete(heimdall_player_t *player)
{
    heimdall_mutex_lock(&player->lock);

    if (player->input)
    {
        heimdall_player_destructor_AddInput(player, player->input);
        player->input = NULL;
    }

    player->deleting = true;
    heimdall_cond_signal(&player->mainloop.wait);

    assert(heimdall_list_is_empty(&player->listeners));
    assert(heimdall_list_is_empty(&player->metadata_listeners));
    assert(heimdall_list_is_empty(&player->vout_listeners));
    assert(heimdall_list_is_empty(&player->aout_listeners));

    heimdall_mutex_unlock(&player->lock);

    heimdall_join(player->mainloop.thread, NULL);

    if (player->media)
        input_item_Release(player->media);
    if (player->next_media)
        input_item_Release(player->next_media);

    free(player->video_string_ids);
    free(player->audio_string_ids);
    free(player->sub_string_ids);

    heimdall_player_DestroyTimer(player);

    heimdall_player_aout_Deinit(player);
    var_DelCallback(player, "corks", heimdall_player_CorkCallback, player);

    input_resource_Release(player->resource);
    if (player->renderer)
        heimdall_renderer_item_release(player->renderer);

    heimdall_http_cookie_jar_t *cookies = var_GetAddress(player, "http-cookies");
    if (cookies != NULL)
    {
        var_Destroy(player, "http-cookies");
        heimdall_http_cookies_destroy(cookies);
    }

    assert(!heimdall_mutex_held(&player->lock));

    heimdall_object_delete(player);
}

heimdall_player_t *
heimdall_player_New(heimdall_object_t *parent, enum heimdall_player_lock_type lock_type)
{
    audio_output_t *aout = NULL;
    heimdall_player_t *player = heimdall_custom_create(parent, sizeof(*player), "player");
    if (!player)
        return NULL;

    heimdall_list_init(&player->listeners);
    heimdall_list_init(&player->metadata_listeners);
    heimdall_list_init(&player->vout_listeners);
    heimdall_list_init(&player->aout_listeners);
    heimdall_list_init(&player->mainloop.stop_inputs);
    heimdall_list_init(&player->mainloop.stopping_inputs);
    heimdall_list_init(&player->mainloop.joinable_inputs);
    player->start_paused = false;
    player->pause_on_cork = false;
    player->corked = false;
    player->renderer = NULL;
    player->media = NULL;
    player->input = NULL;
    player->global_state = HEIMDALL_PLAYER_STATE_STOPPED;
    player->started = false;

    player->last_eos = HEIMDALL_TICK_INVALID;
    player->eos_burst_count = 0;

    player->releasing_media = false;
    player->next_media = NULL;

    player->video_string_ids = player->audio_string_ids =
    player->sub_string_ids = NULL;

    player->play_and_pause = false;
    player->repeat = 0;

#define VAR_CREATE(var, flag) do { \
    if (var_Create(player, var, flag) != HEIMDALL_SUCCESS) \
        goto error; \
} while(0)

    /* player variables */
    VAR_CREATE("rate", HEIMDALL_VAR_FLOAT | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("sub-fps", HEIMDALL_VAR_FLOAT | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("sub-text-scale", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("demux-filter", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);

    /* aout variables */
    VAR_CREATE("audio-filter", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("mute", HEIMDALL_VAR_BOOL);
    VAR_CREATE("corks", HEIMDALL_VAR_INTEGER);

    /* es_out variables */
    VAR_CREATE("sout", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("video", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("sout-video", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("audio", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("sout-audio", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("spu", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("sout-spu", HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("audio-language", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);
    VAR_CREATE("sub-language", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);

    /* TODO: Override these variables since the player handle media ended
     * action itself. */
    VAR_CREATE("start-paused", HEIMDALL_VAR_BOOL);

    /* Initialize the shared HTTP cookie jar */
    heimdall_value_t cookies;
    cookies.p_address = heimdall_http_cookies_new();
    if (likely(cookies.p_address != NULL))
    {
        VAR_CREATE("http-cookies", HEIMDALL_VAR_ADDRESS);
        var_SetChecked(player, "http-cookies", HEIMDALL_VAR_ADDRESS, cookies);
    }
#undef VAR_CREATE

    player->resource = input_resource_New(HEIMDALL_OBJECT(player));

    if (!player->resource)
        goto error;

    /* Ensure the player has a valid aout */
    aout = heimdall_player_aout_Init(player);
    var_AddCallback(player, "corks", heimdall_player_CorkCallback, player);

    player->deleting = false;
    heimdall_player_InitLocks(player, lock_type);
    heimdall_player_InitTimer(player);

    if (heimdall_clone(&player->mainloop.thread, heimdall_player_mainloop_Thread,
                  player) != 0)
    {
        heimdall_player_DestroyTimer(player);
        goto error;
    }

    return player;

error:
    if (aout)
        heimdall_player_aout_Deinit(player);
    var_DelCallback(player, "corks", heimdall_player_CorkCallback, player);
    if (player->resource)
        input_resource_Release(player->resource);

    heimdall_object_delete(player);
    return NULL;
}

heimdall_object_t *
heimdall_player_GetObject(heimdall_player_t *player)
{
    return HEIMDALL_OBJECT(player);
}
