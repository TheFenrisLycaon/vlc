/*****************************************************************************
 * player_input.c: Player input implementation
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

#include <heimdall_common.h>
#include <heimdall_memstream.h>
#include <heimdall_dialog.h>
#include "player.h"

struct heimdall_player_track_priv *
heimdall_player_input_FindTrackById(struct heimdall_player_input *input, heimdall_es_id_t *id,
                               size_t *idx)
{
    heimdall_player_track_vector *vec =
        heimdall_player_input_GetTrackVector(input, heimdall_es_id_GetCat(id));
    return vec ? heimdall_player_track_vector_FindById(vec, id, idx) : NULL;
}

heimdall_tick_t
heimdall_player_input_GetTime(struct heimdall_player_input *input, bool seeking,
                         heimdall_tick_t system_now)
{
    heimdall_player_t *player = input->player;
    heimdall_tick_t ts;

    if (input == player->input
     && heimdall_player_GetTimerPoint(player, &seeking, system_now, &ts, NULL) == 0)
        return ts;
    return input->time;
}

double
heimdall_player_input_GetPos(struct heimdall_player_input *input, bool seeking,
                        heimdall_tick_t system_now)
{
    heimdall_player_t *player = input->player;
    double pos;

    if (input == player->input
     && heimdall_player_GetTimerPoint(player, &seeking, system_now, NULL, &pos) == 0)
        return pos;
    return input->position;
}

bool
heimdall_player_input_HandleAtoBLoop(struct heimdall_player_input *input, bool forced)
{
    if (!input->abloop_state[0].set || !input->abloop_state[1].set
     || (input->capabilities & HEIMDALL_PLAYER_CAP_SEEK) == 0)
        return false;

    heimdall_player_t *player = input->player;

    if (player->input != input)
        return false;

    heimdall_tick_t now = heimdall_tick_now();
    if (input->abloop_state[0].time != HEIMDALL_TICK_INVALID
     && input->abloop_state[1].time != HEIMDALL_TICK_INVALID)
    {
        heimdall_tick_t time = heimdall_player_input_GetTime(input, false, now);
        if (forced
         || (time != HEIMDALL_TICK_INVALID && time >= input->abloop_state[1].time))
            heimdall_player_SetTime(player, input->abloop_state[0].time);
        return true;
    }

    double pos = heimdall_player_input_GetPos(input, false, now);
    if (forced || pos >= input->abloop_state[1].pos)
    {
        heimdall_player_SetPosition(player, input->abloop_state[0].pos);
        return true;
    }

    return false;
}

int
heimdall_player_input_Start(struct heimdall_player_input *input)
{
    int ret = input_Start(input->thread);
    if (ret != HEIMDALL_SUCCESS)
        return ret;
    input->started = true;
    return ret;
}

static inline void
heimdall_player_assert_seek_params(enum heimdall_player_seek_speed speed,
                              enum heimdall_player_whence whence)
{
    assert(speed == HEIMDALL_PLAYER_SEEK_PRECISE
        || speed == HEIMDALL_PLAYER_SEEK_FAST);
    assert(whence == HEIMDALL_PLAYER_WHENCE_ABSOLUTE
        || whence == HEIMDALL_PLAYER_WHENCE_RELATIVE);
    (void) speed; (void) whence;
}

void
heimdall_player_input_SeekByPos(struct heimdall_player_input *input, double position,
                           enum heimdall_player_seek_speed speed,
                           enum heimdall_player_whence whence)
{
    heimdall_player_t *player = input->player;
    heimdall_player_assert_seek_params(speed, whence);

    /* Assert before we modify position internally, this only check that user
     * positions are correct. */
    if (whence != HEIMDALL_PLAYER_WHENCE_ABSOLUTE)
    {
        assert(position >= -1 && position <=1);
        position += heimdall_player_input_GetPos(input, true, heimdall_tick_now());
    }
    else
    {
        assert(position >= 0 && position <=1);
    }

    if (position < 0)
        position = 0;
    else if (position > 1)
        position = 1;

    heimdall_player_UpdateTimerSeekState(player, HEIMDALL_TICK_INVALID, position);

    int ret = input_ControlPush(input->thread, INPUT_CONTROL_SET_POSITION,
        &(input_control_param_t) {
            .pos.f_val = position,
            .pos.b_fast_seek = speed == HEIMDALL_PLAYER_SEEK_FAST,
    });

    if (ret == HEIMDALL_SUCCESS)
        heimdall_player_osd_Position(player, input, HEIMDALL_TICK_INVALID, position);
}

void
heimdall_player_input_SeekByTime(struct heimdall_player_input *input, heimdall_tick_t time,
                            enum heimdall_player_seek_speed speed,
                            enum heimdall_player_whence whence)
{
    heimdall_player_t *player = input->player;
    heimdall_player_assert_seek_params(speed, whence);

    if (whence != HEIMDALL_PLAYER_WHENCE_ABSOLUTE)
        time += heimdall_player_input_GetTime(input, true, heimdall_tick_now());

    if (time < HEIMDALL_TICK_0)
        time = HEIMDALL_TICK_0;

    heimdall_player_UpdateTimerSeekState(player, time, -1);

    int ret = input_ControlPush(input->thread, INPUT_CONTROL_SET_TIME,
        &(input_control_param_t) {
            .time.i_val = time,
            .time.b_fast_seek = speed == HEIMDALL_PLAYER_SEEK_FAST,
    });

    if (ret == HEIMDALL_SUCCESS)
        heimdall_player_osd_Position(player, input, time, -1);
}

void
heimdall_player_input_UpdateViewpoint(struct heimdall_player_input *input,
                                 const heimdall_viewpoint_t *viewpoint,
                                 enum heimdall_player_whence whence)
{
    input_control_param_t param = { .viewpoint = *viewpoint };
    if (whence == HEIMDALL_PLAYER_WHENCE_ABSOLUTE)
        input_ControlPush(input->thread, INPUT_CONTROL_SET_VIEWPOINT,
                          &param);
    else
        input_ControlPush(input->thread, INPUT_CONTROL_UPDATE_VIEWPOINT,
                          &param);
}

static bool
heimdall_player_WaitRetryDelay(heimdall_player_t *player)
{
#define RETRY_TIMEOUT_BASE HEIMDALL_TICK_FROM_MS(100)
#define RETRY_TIMEOUT_MAX HEIMDALL_TICK_FROM_MS(3200)
#define MAX_EOS_BURST 4
    /* Temporize only after a few successive EOS */
    if (player->eos_burst_count > MAX_EOS_BURST)
    {
        unsigned pow = player->eos_burst_count - MAX_EOS_BURST;
        /* Delay the next opening to avoid busy loops */
        heimdall_tick_t delay = RETRY_TIMEOUT_BASE;
        for (unsigned i = 1; i < pow && delay < RETRY_TIMEOUT_MAX; ++i)
            delay *= 2; /* Wait 100, 200, 400, 800, 1600 and finally 3200ms */
        delay += heimdall_tick_now();

        while (player->eos_burst_count != 0
            && heimdall_cond_timedwait(&player->start_delay_cond, &player->lock,
                                  delay) == 0);
        if (player->eos_burst_count == 0)
            return false; /* canceled */
    }
    return true;
}

void
heimdall_player_input_HandleState(struct heimdall_player_input *input,
                             enum heimdall_player_state state, heimdall_tick_t state_date)
{
    heimdall_player_t *player = input->player;

    /* The STOPPING state can be set earlier by the player. In that case,
     * ignore all future events except the STOPPED one */
    if (input->state == HEIMDALL_PLAYER_STATE_STOPPING
     && state != HEIMDALL_PLAYER_STATE_STOPPED)
        return;

    enum heimdall_player_state last_state = input->state;
    input->state = state;

    /* Override the global state if the player is still playing and has a next
     * media to play */
    bool send_event = player->global_state != state;
    switch (input->state)
    {
        case HEIMDALL_PLAYER_STATE_STOPPED:
            assert(!input->started);
            assert(input != player->input);

            if (input->titles)
            {
                heimdall_player_title_list_Release(input->titles);
                input->titles = NULL;
                heimdall_player_SendEvent(player, on_titles_changed, NULL);
            }

            heimdall_player_ResetTimer(player);

            if (player->last_eos != HEIMDALL_TICK_INVALID)
            {
                heimdall_tick_t diff = heimdall_tick_now() - player->last_eos;
                if (diff < HEIMDALL_PLAYER_EOS_BURST_THRESHOLD)
                    ++player->eos_burst_count;
                else
                    player->eos_burst_count = 0;
            }

            if (player->started)
                heimdall_player_WaitRetryDelay(player);

            /* Assign the current date after the wait */
            player->last_eos = heimdall_tick_now();

            if (!player->deleting)
            {
                heimdall_player_OpenNextMedia(player);
                /* It is possible to open several medias in a stopped state */
                if (player->input && player->started)
                    heimdall_player_input_Start(player->input);
            }
            if (!player->input)
                player->started = false;

            send_event = !player->started && last_state != HEIMDALL_PLAYER_STATE_STOPPED;
            break;
        case HEIMDALL_PLAYER_STATE_STOPPING:
        {
            enum heimdall_player_media_stopping_reason reason;
            /* The order matters. input->error is only set while the
               input->started flag is set, so an error or EOS that follows a
               user stop request is ignored and the stop is reported as USER,
               whereas an error that occurred before a user stop request takes
               precedence and is reported as ERROR. */
            if (input->error != HEIMDALL_PLAYER_ERROR_NONE)
                reason = HEIMDALL_PLAYER_MEDIA_STOPPING_ERROR;
            else if (!input->started)
                reason = HEIMDALL_PLAYER_MEDIA_STOPPING_USER;
            else
                reason = HEIMDALL_PLAYER_MEDIA_STOPPING_EOS;
            input->started = false;

            /* Note: no need to hold the media here, as it is already protected
               by the player lock. User can hold the input_item, if they want
               to use it beyond the callback scope. */
            input_item_t *media = input_GetItem(input->thread);
            heimdall_player_SendEvent(player, on_stopping_current_media,
                                 media, reason);

            heimdall_player_UpdateTimerEvent(player, NULL,
                                        HEIMDALL_PLAYER_TIMER_EVENT_DISCONTINUITY,
                                        HEIMDALL_TICK_INVALID);

            heimdall_player_UpdateTimerEvent(player, NULL,
                                        HEIMDALL_PLAYER_TIMER_EVENT_STOPPING,
                                        HEIMDALL_TICK_INVALID);

            if (input == player->input)
                player->input = NULL;

            if (player->started && !player->next_media)
                player->started = false;
            send_event = !player->started;
            break;
        }
        case HEIMDALL_PLAYER_STATE_PLAYING:
            input->pause_date = HEIMDALL_TICK_INVALID;
            heimdall_player_SignalAtoBLoop(player);
            heimdall_player_UpdateTimerEvent(player, NULL,
                                        HEIMDALL_PLAYER_TIMER_EVENT_PLAYING,
                                        state_date);
            /* fall through */
        case HEIMDALL_PLAYER_STATE_STARTED:
            if (player->started &&
                player->global_state == HEIMDALL_PLAYER_STATE_PLAYING)
                send_event = false;
            break;

        case HEIMDALL_PLAYER_STATE_PAUSED:
            /* Can pause from PLAYING or STARTED (deferred pause during buffering) */
            assert(player->global_state == HEIMDALL_PLAYER_STATE_PLAYING
                   || player->global_state == HEIMDALL_PLAYER_STATE_STARTED);
            assert(state_date != HEIMDALL_TICK_INVALID);
            input->pause_date = state_date;

            heimdall_player_SignalAtoBLoop(player);
            heimdall_player_UpdateTimerEvent(player, NULL,
                                        HEIMDALL_PLAYER_TIMER_EVENT_PAUSED,
                                        input->pause_date);
            break;
        default:
            heimdall_assert_unreachable();
    }

    if (send_event)
    {
        assert(player->global_state != input->state);
        player->global_state = input->state;
        heimdall_player_SendEvent(player, on_state_changed, player->global_state);
    }
}

static void
heimdall_player_input_HandleStateEvent(struct heimdall_player_input *input,
                                  input_state_e state, heimdall_tick_t state_date)
{
    switch (state)
    {
        case OPENING_S:
            heimdall_player_input_HandleState(input, HEIMDALL_PLAYER_STATE_STARTED,
                                         HEIMDALL_TICK_INVALID);
            break;
        case PLAYING_S:
            input->playing = true;
            heimdall_player_input_HandleState(input, HEIMDALL_PLAYER_STATE_PLAYING,
                                         state_date);
            break;
        case PAUSE_S:
            heimdall_player_input_HandleState(input, HEIMDALL_PLAYER_STATE_PAUSED,
                                         state_date);
            break;
        case END_S:
            input->playing = false;
            heimdall_player_input_HandleState(input, HEIMDALL_PLAYER_STATE_STOPPING,
                                         HEIMDALL_TICK_INVALID);
            heimdall_player_destructor_AddStoppingInput(input->player, input);
            break;
        case ERROR_S:
            /* Don't send errors if the input is stopped by the user */
            if (input->started)
            {
                /* Contrary to the input_thread_t, an error is not a state */
                input->error = HEIMDALL_PLAYER_ERROR_GENERIC;
                heimdall_player_SendEvent(input->player, on_error_changed, input->error);
            }
            /* input->playing monitor the OPENING_S -> PLAYING_S transition.
             * If input->playing is false, we have an error at the opening of
             * the input thread and we won't reach END_S. */
            if (!input->playing)
            {
                heimdall_player_input_HandleState(input, HEIMDALL_PLAYER_STATE_STOPPING,
                                             HEIMDALL_TICK_INVALID);
                heimdall_player_destructor_AddStoppingInput(input->player, input);
            }
            break;
        default:
            heimdall_assert_unreachable();
    }
}

static void
heimdall_player_input_HandleProgramEvent(struct heimdall_player_input *input,
                                    const struct heimdall_input_event_program *ev)
{
    heimdall_player_t *player = input->player;
    struct heimdall_player_program *prgm;
    heimdall_player_program_vector *vec = &input->program_vector;

    switch (ev->action)
    {
        case HEIMDALL_INPUT_PROGRAM_ADDED:
            prgm = heimdall_player_program_New(ev->id, ev->title);
            if (!prgm)
                break;

            if (!heimdall_vector_push(vec, prgm))
            {
                heimdall_player_program_Delete(prgm);
                break;
            }
            heimdall_player_SendEvent(player, on_program_list_changed,
                                 HEIMDALL_PLAYER_LIST_ADDED, prgm);
            break;
        case HEIMDALL_INPUT_PROGRAM_DELETED:
        {
            size_t idx;
            prgm = heimdall_player_program_vector_FindById(vec, ev->id, &idx);
            if (prgm)
            {
                heimdall_player_SendEvent(player, on_program_list_changed,
                                     HEIMDALL_PLAYER_LIST_REMOVED, prgm);
                heimdall_vector_remove(vec, idx);
                heimdall_player_program_Delete(prgm);
            }
            break;
        }
        case HEIMDALL_INPUT_PROGRAM_UPDATED:
        case HEIMDALL_INPUT_PROGRAM_SCRAMBLED:
            prgm = heimdall_player_program_vector_FindById(vec, ev->id, NULL);
            if (!prgm)
                break;
            if (ev->action == HEIMDALL_INPUT_PROGRAM_UPDATED)
            {
                if (heimdall_player_program_Update(prgm, ev->id, ev->title) != 0)
                    break;
            }
            else
                prgm->scrambled = ev->scrambled;
            heimdall_player_SendEvent(player, on_program_list_changed,
                                 HEIMDALL_PLAYER_LIST_UPDATED, prgm);
            break;
        case HEIMDALL_INPUT_PROGRAM_SELECTED:
        {
            int unselected_id = -1, selected_id = -1;
            heimdall_vector_foreach(prgm, vec)
            {
                if (prgm->group_id == ev->id)
                {
                    if (!prgm->selected)
                    {
                        assert(selected_id == -1);
                        prgm->selected = true;
                        selected_id = prgm->group_id;
                    }
                }
                else
                {
                    if (prgm->selected)
                    {
                        assert(unselected_id == -1);
                        prgm->selected = false;
                        unselected_id = prgm->group_id;
                    }
                }
            }
            if (unselected_id != -1 || selected_id != -1)
                heimdall_player_SendEvent(player, on_program_selection_changed,
                                     unselected_id, selected_id);
            break;
        }
        default:
            heimdall_assert_unreachable();
    }
}

static const struct heimdall_player_track_priv *
heimdall_player_FindTeletextSource(const struct heimdall_player_input *input,
                              const struct heimdall_player_track_priv *exclude,
                              bool selected)
{
    const struct heimdall_player_track_priv *t;
    heimdall_vector_foreach(t, &input->spu_track_vector)
    {
        if (t->t.fmt.i_codec == HEIMDALL_CODEC_TELETEXT &&
           t != exclude &&
           t->t.selected == selected)
            return t;
    }
    return NULL;
}

static unsigned
heimdall_player_input_TeletextUserPage(const struct heimdall_player_track_priv *t)
{
    const uint8_t mag = t->t.fmt.subs.teletext.i_magazine;
    const uint8_t page = t->t.fmt.subs.teletext.i_page;
    return (mag % 10) * 100 +
           (page & 0x0F) + ((page >> 4) & 0x0F) * 10;
}

static void
heimdall_player_input_HandleTeletextMenu(struct heimdall_player_input *input,
                                    const struct heimdall_input_event_es *ev,
                                    const struct heimdall_player_track_priv *trackpriv)
{
    heimdall_player_t *player = input->player;
    if (ev->fmt->i_cat != SPU_ES ||
        ev->fmt->i_codec != HEIMDALL_CODEC_TELETEXT)
        return;
    switch (ev->action)
    {
        case HEIMDALL_INPUT_ES_ADDED:
        {
            if (!input->teletext_source)
            {
                input->teletext_source = trackpriv;
                heimdall_player_SendEvent(player, on_teletext_menu_changed, true);
            }
            break;
        }
        case HEIMDALL_INPUT_ES_DELETED:
        {
            if (input->teletext_source == trackpriv)
            {
                input->teletext_source =
                        heimdall_player_FindTeletextSource(input, trackpriv, true);
                if (!input->teletext_source)
                    input->teletext_source =
                            heimdall_player_FindTeletextSource(input, trackpriv, false);
                if (!input->teletext_source) /* no more teletext ES */
                {
                    if (input->teletext_enabled)
                    {
                        input->teletext_enabled = false;
                        heimdall_player_SendEvent(player, on_teletext_enabled_changed, false);
                    }
                    heimdall_player_SendEvent(player, on_teletext_menu_changed, false);
                }
                else /* another teletext ES was reselected */
                {
                    if (input->teletext_source->t.selected != input->teletext_enabled)
                    {
                        input->teletext_enabled = input->teletext_source->t.selected;
                        heimdall_player_SendEvent(player, on_teletext_enabled_changed,
                                             input->teletext_source->t.selected);
                    }
                    input->teletext_page =
                            heimdall_player_input_TeletextUserPage(input->teletext_source);
                    heimdall_player_SendEvent(player, on_teletext_page_changed,
                                         input->teletext_page);
                }
            }
            break;
        }
        case HEIMDALL_INPUT_ES_UPDATED:
            break;
        case HEIMDALL_INPUT_ES_SELECTED:
        {
            if (!input->teletext_enabled) /* we stick with the first selected */
            {
                input->teletext_source = trackpriv;
                input->teletext_enabled = true;
                input->teletext_page = heimdall_player_input_TeletextUserPage(trackpriv);
                heimdall_player_SendEvent(player, on_teletext_enabled_changed, true);
                heimdall_player_SendEvent(player, on_teletext_page_changed,
                                     input->teletext_page);
            }
            break;
        }
        case HEIMDALL_INPUT_ES_UNSELECTED:
            if (input->teletext_source == trackpriv)
            {
                /* If there's another selected teletext, it needs to become source */
                const struct heimdall_player_track_priv *other =
                        heimdall_player_FindTeletextSource(input, trackpriv, true);
                if (other)
                {
                    input->teletext_source = other;
                    if (!input->teletext_enabled)
                    {
                        input->teletext_enabled = true;
                        heimdall_player_SendEvent(player, on_teletext_enabled_changed, true);
                    }
                    input->teletext_page = heimdall_player_input_TeletextUserPage(other);
                    heimdall_player_SendEvent(player, on_teletext_page_changed,
                                         input->teletext_page);
                }
                else
                {
                    input->teletext_enabled = false;
                    heimdall_player_SendEvent(player, on_teletext_enabled_changed, false);
                }
            }
            break;
        default:
            heimdall_assert_unreachable();
    }
}

static void
heimdall_player_input_HandleEsEvent(struct heimdall_player_input *input,
                               const struct heimdall_input_event_es *ev)
{
    assert(ev->id && ev->title && ev->fmt);

    heimdall_player_track_vector *vec =
        heimdall_player_input_GetTrackVector(input, ev->fmt->i_cat);
    if (!vec)
        return; /* UNKNOWN_ES or DATA_ES not handled */

    heimdall_player_t *player = input->player;
    struct heimdall_player_track_priv *trackpriv;
    switch (ev->action)
    {
        case HEIMDALL_INPUT_ES_ADDED:
            trackpriv = heimdall_player_track_priv_New(ev->id, ev->title, ev->fmt);
            if (!trackpriv)
                break;

            if (ev->fmt->i_cat == VIDEO_ES)
                input->ml.has_video_tracks = true;
            else if (ev->fmt->i_cat == AUDIO_ES)
                input->ml.has_audio_tracks = true;
            if (!heimdall_vector_push(vec, trackpriv))
            {
                heimdall_player_track_priv_Delete(trackpriv);
                break;
            }
            heimdall_player_SendEvent(player, on_track_list_changed,
                                 HEIMDALL_PLAYER_LIST_ADDED, &trackpriv->t);
            heimdall_player_input_HandleTeletextMenu(input, ev, trackpriv);
            break;
        case HEIMDALL_INPUT_ES_DELETED:
        {
            size_t idx;
            trackpriv = heimdall_player_track_vector_FindById(vec, ev->id, &idx);
            if (trackpriv)
            {
                heimdall_player_input_HandleTeletextMenu(input, ev, trackpriv);
                heimdall_player_SendEvent(player, on_track_list_changed,
                                     HEIMDALL_PLAYER_LIST_REMOVED, &trackpriv->t);
                heimdall_vector_remove(vec, idx);
                heimdall_player_track_priv_Delete(trackpriv);
            }
            break;
        }
        case HEIMDALL_INPUT_ES_UPDATED:
            trackpriv = heimdall_player_track_vector_FindById(vec, ev->id, NULL);
            if (!trackpriv)
                break;
            if (heimdall_player_track_priv_Update(trackpriv, ev->title, ev->fmt) != 0)
                break;
            heimdall_player_SendEvent(player, on_track_list_changed,
                                 HEIMDALL_PLAYER_LIST_UPDATED, &trackpriv->t);
            heimdall_player_input_HandleTeletextMenu(input, ev, trackpriv);
            break;
        case HEIMDALL_INPUT_ES_SELECTED:
            trackpriv = heimdall_player_track_vector_FindById(vec, ev->id, NULL);
            if (trackpriv)
            {
                trackpriv->t.selected = true;
                trackpriv->selected_by_user = ev->forced;
                trackpriv->vout_order = ev->vout_order;
                heimdall_player_SendEvent(player, on_track_selection_changed,
                                     NULL, trackpriv->t.es_id);
                heimdall_player_input_HandleTeletextMenu(input, ev, trackpriv);
            }
            break;
        case HEIMDALL_INPUT_ES_UNSELECTED:
            trackpriv = heimdall_player_track_vector_FindById(vec, ev->id, NULL);
            if (trackpriv)
            {
                heimdall_player_RemoveTimerSource(player, ev->id);
                trackpriv->t.selected = false;
                trackpriv->selected_by_user = false;
                heimdall_player_SendEvent(player, on_track_selection_changed,
                                     trackpriv->t.es_id, NULL);
                heimdall_player_input_HandleTeletextMenu(input, ev, trackpriv);
            }
            break;
        default:
            heimdall_assert_unreachable();
    }
}

static void
heimdall_player_input_HandleTitleEvent(struct heimdall_player_input *input,
                                  const struct heimdall_input_event_title *ev)
{
    heimdall_player_t *player = input->player;
    switch (ev->action)
    {
        case HEIMDALL_INPUT_TITLE_NEW_LIST:
        {
            input_thread_private_t *input_th = input_priv(input->thread);
            const int title_offset = input_th->i_title_offset;
            const int chapter_offset = input_th->i_seekpoint_offset;

            if (input->titles)
                heimdall_player_title_list_Release(input->titles);
            input->title_selected = input->chapter_selected = 0;
            input->titles =
                heimdall_player_title_list_Create(ev->list.array, ev->list.count,
                                             title_offset, chapter_offset);
            heimdall_player_SendEvent(player, on_titles_changed, input->titles);
            if (input->titles)
            {
                heimdall_player_SendEvent(player, on_title_selection_changed,
                                     &input->titles->array[0], 0);
                if (input->ml.restore == HEIMDALL_RESTOREPOINT_TITLE &&
                    (size_t)input->ml.states.current_title < ev->list.count)
                {
                    heimdall_player_SelectTitleIdx(player, input->ml.states.current_title);
                }
                input->ml.restore = HEIMDALL_RESTOREPOINT_POSITION;
            }
            else input->ml.restore = HEIMDALL_RESTOREPOINT_NONE;
            break;
        }
        case HEIMDALL_INPUT_TITLE_SELECTED:
            if (!input->titles)
                return; /* a previous HEIMDALL_INPUT_TITLE_NEW_LIST failed */
            assert(ev->selected_idx < input->titles->count);
            input->title_selected = ev->selected_idx;
            heimdall_player_SendEvent(player, on_title_selection_changed,
                                 &input->titles->array[input->title_selected],
                                 input->title_selected);
            if (input->ml.restore == HEIMDALL_RESTOREPOINT_POSITION &&
                input->ml.states.current_title >= 0 &&
                (size_t)input->ml.states.current_title == ev->selected_idx &&
                input->ml.pos > .0f)
            {
                input_SetPosition(input->thread, input->ml.pos, false);
            }
            /* Reset the wanted title to avoid forcing it or the position
             * again during the next title change
             */
            input->ml.restore = HEIMDALL_RESTOREPOINT_NONE;
            break;
        default:
            heimdall_assert_unreachable();
    }
}

static void
heimdall_player_input_HandleChapterEvent(struct heimdall_player_input *input,
                                    const struct heimdall_input_event_chapter *ev)
{
    heimdall_player_t *player = input->player;
    if (!input->titles || ev->title < 0 || ev->seekpoint < 0)
        return; /* a previous HEIMDALL_INPUT_TITLE_NEW_LIST failed */

    assert((size_t)ev->title < input->titles->count);
    const struct heimdall_player_title *title = &input->titles->array[ev->title];
    if (!title->chapter_count)
        return;

    assert(ev->seekpoint < (int)title->chapter_count);
    input->title_selected = ev->title;
    input->chapter_selected = ev->seekpoint;

    const struct heimdall_player_chapter *chapter = &title->chapters[ev->seekpoint];
    heimdall_player_SendEvent(player, on_chapter_selection_changed, title, ev->title,
                         chapter, ev->seekpoint);
}

static void
heimdall_player_input_HandleVoutEvent(struct heimdall_player_input *input,
                                 const struct heimdall_input_event_vout *ev)
{
    assert(ev->vout);
    assert(ev->id);

    heimdall_player_t *player = input->player;

    struct heimdall_player_track_priv *trackpriv =
        heimdall_player_input_FindTrackById(input, ev->id, NULL);
    if (!trackpriv)
        return;

    const bool is_video_es = trackpriv->t.fmt.i_cat == VIDEO_ES;

    switch (ev->action)
    {
        case HEIMDALL_INPUT_EVENT_VOUT_STARTED:
            trackpriv->vout = ev->vout;
            heimdall_player_SendEvent(player, on_vout_changed,
                                 HEIMDALL_PLAYER_VOUT_STARTED, ev->vout,
                                 ev->order, ev->id);

            if (is_video_es)
            {
                /* Register vout callbacks after the vout list event */
                heimdall_player_vout_AddCallbacks(player, ev->vout);
            }
            break;
        case HEIMDALL_INPUT_EVENT_VOUT_STOPPED:
            if (is_video_es)
            {
                /* Un-register vout callbacks before the vout list event */
                heimdall_player_vout_DelCallbacks(player, ev->vout);
            }

            trackpriv->vout = NULL;
            heimdall_player_SendEvent(player, on_vout_changed,
                                 HEIMDALL_PLAYER_VOUT_STOPPED, ev->vout,
                                 HEIMDALL_VOUT_ORDER_NONE, ev->id);
            break;
        default:
            heimdall_assert_unreachable();
    }
}

static void
heimdall_player_input_NavigationFallback(struct heimdall_player_input *input, int nav_type)
{
    heimdall_player_t *player = input->player;

    /* Handle Up/Down/Left/Right if the demux can't navigate */
    heimdall_viewpoint_t vp = { 0 };
    bool viewpoint_updated = true;
    float yaw = 0.f, pitch = 0.f, roll = 0.f;

    int vol_direction = 0;
    int seek_direction = 0;
    switch (nav_type)
    {
        case INPUT_CONTROL_NAV_UP:
            vol_direction = 1;
            pitch = -1.f;
            break;
        case INPUT_CONTROL_NAV_DOWN:
            vol_direction = -1;
            pitch = 1.f;
            break;
        case INPUT_CONTROL_NAV_LEFT:
            seek_direction = -1;
            yaw = -1.f;
            break;
        case INPUT_CONTROL_NAV_RIGHT:
            seek_direction = 1;
            yaw = 1.f;
            break;
        case INPUT_CONTROL_NAV_ACTIVATE:
        case INPUT_CONTROL_NAV_POPUP:
        case INPUT_CONTROL_NAV_MENU:
            viewpoint_updated = false;
            return;
        default:
            heimdall_assert_unreachable();
    }

    /* Try to change the viewpoint if possible */
    bool viewpoint_ch = false;
    size_t vout_count;
    vout_thread_t **vouts = heimdall_player_vout_HoldAll(input->player, &vout_count);
    for (size_t i = 0; i < vout_count; ++i)
    {
        if (!viewpoint_ch && var_GetBool(vouts[i], "viewpoint-changeable"))
            viewpoint_ch = true;
        vout_Release(vouts[i]);
    }
    free(vouts);

    if (viewpoint_ch && viewpoint_updated)
    {
        heimdall_viewpoint_from_euler(&vp, yaw, pitch, roll);
        heimdall_player_input_UpdateViewpoint(input, &vp, HEIMDALL_PLAYER_WHENCE_RELATIVE);
    }
    else if (seek_direction != 0)
    {
        /* Seek or change volume if the input doesn't have navigation or viewpoint */
        heimdall_tick_t it = heimdall_tick_from_sec(seek_direction
                      * var_InheritInteger(player, "short-jump-size"));
        heimdall_player_input_SeekByTime(input, it, HEIMDALL_PLAYER_SEEK_PRECISE,
                                    HEIMDALL_PLAYER_WHENCE_RELATIVE);
    }
    else
    {
        assert(vol_direction != 0);
        if (input == player->input)
            heimdall_player_aout_IncrementVolume(player, vol_direction, NULL);
    }
}

static void
heimdall_player_input_MouseFallback(struct heimdall_player_input *input)
{
    heimdall_player_t *player = input->player;
    heimdall_player_TogglePause(player);
}

static void
heimdall_player_DisplayFrameError(heimdall_player_t *player,
                             const char *title, const char *title_error,
                             int status)
{
    static const char erange[] = {N_("can't seek back")};
    static const char ebusy[] = {N_("no video found")};
    static const char enotusp[] = {N_("can't pause/seek/pace")};
    static const char einval[] = {N_("invalid state")};

    switch (status)
    {
        case 0:
            heimdall_player_osd_Message(player, title);
            break;
        case -EAGAIN:
            break;
        case -ERANGE:
            heimdall_dialog_display_error(player, title_error, erange);
            break;
        case -EBUSY:
            heimdall_dialog_display_error(player, title_error, ebusy);
            break;
        case -ENOTSUP:
            heimdall_dialog_display_error(player, title_error, enotusp);
            break;
        default:
        case -EINVAL:
            heimdall_dialog_display_error(player, title_error, einval);
            break;
    }
}

static void
heimdall_player_input_FrameNextStatus(struct heimdall_player_input *input, int status)
{
    heimdall_player_t *player = input->player;

    unsigned count;
    heimdall_player_SendEventCount(player, on_next_frame_status, count, status);

    /* Don't display errors if status is handled by the player user */
    if (count != 0)
        return;

    heimdall_player_DisplayFrameError(player, _("Next frame"),
                                 _("Next frame error"), status);
}

static void
heimdall_player_input_FramePreviousStatus(struct heimdall_player_input *input, int status)
{
    heimdall_player_t *player = input->player;

    unsigned count;
    heimdall_player_SendEventCount(player, on_prev_frame_status, count, status);

    /* Don't display errors if status is handled by the player user */
    if (count != 0)
        return;

    heimdall_player_DisplayFrameError(player, _("Previous frame"),
                                 _("Previous frame error"), status);
}

static bool
input_thread_Events(input_thread_t *input_thread,
                    const struct heimdall_input_event *event, void *user_data)
{
    struct heimdall_player_input *input = user_data;
    heimdall_player_t *player = input->player;
    input_thread_private_t *priv = input_priv(input_thread);

    assert(input_thread == input->thread);

    /* No player lock for this event */
    if (event->type == INPUT_EVENT_OUTPUT_CLOCK)
    {
        if (event->output_clock.system_ts != HEIMDALL_TICK_INVALID)
        {
            const struct heimdall_player_timer_point point = {
                .position = 0,
                .rate = event->output_clock.rate,
                .ts = event->output_clock.ts,
                .length = HEIMDALL_TICK_INVALID,
                .system_date = event->output_clock.system_ts,
            };
            heimdall_player_UpdateTimer(player, event->output_clock.id,
                                   event->output_clock.master, &point,
                                   HEIMDALL_TICK_INVALID,
                                   event->output_clock.frame_rate,
                                   event->output_clock.frame_rate_base, 0);
        }
        else
        {
            heimdall_player_UpdateTimerEvent(player, event->output_clock.id,
                                        HEIMDALL_PLAYER_TIMER_EVENT_DISCONTINUITY,
                                        HEIMDALL_TICK_INVALID);
        }
        return true;
    }

    bool handled = true;

    heimdall_mutex_lock(&player->lock);

    switch (event->type)
    {
        case INPUT_EVENT_STATE:
            heimdall_player_input_HandleStateEvent(input, event->state.value,
                                              event->state.date);
            break;
        case INPUT_EVENT_EOF:
            handled = heimdall_player_input_HandleAtoBLoop(input, true);
            if (handled)
                break;
            if (player->play_and_pause)
            {
                heimdall_player_Pause(player);
                handled = true;
            }
            else if (input->repeat > 0)
            {
                input->repeat--;
                handled =
                    input_ControlPush(input->thread,
                                      INPUT_CONTROL_RESET_POSITION, NULL) == 0;
            }
            else
                handled = false;
            break;
        case INPUT_EVENT_RATE:
            input->rate = event->rate;
            heimdall_player_SignalAtoBLoop(player);
            heimdall_player_SendEvent(player, on_rate_changed, input->rate);
            break;
        case INPUT_EVENT_CAPABILITIES:
        {
            int old_caps = input->capabilities;
            input->capabilities = event->capabilities;
            heimdall_player_SendEvent(player, on_capabilities_changed,
                                 old_caps, input->capabilities);
            break;
        }
        case INPUT_EVENT_TIMES:
        {
            bool changed = false;
            heimdall_tick_t system_date = HEIMDALL_TICK_INVALID;
            heimdall_tick_t duration = input_GetItemDuration(input->thread, event->times.length);

            if (event->times.time != HEIMDALL_TICK_INVALID
             && (input->time != event->times.time
              || input->position != event->times.position))
            {
                input->time = event->times.time;
                input->position = event->times.position;
                system_date = heimdall_tick_now();
                changed = true;
                heimdall_player_SendEvent(player, on_position_changed,
                                     input->time, input->position);

                heimdall_player_input_HandleAtoBLoop(input, false);
            }
            if (input->length != duration)
            {
                input->length = duration;
                input_item_SetDuration(input_GetItem(input->thread), duration);
                heimdall_player_SendEvent(player, on_length_changed, input->length);
                changed = true;
            }

            if (input->live != event->times.live)
            {
                input->live = event->times.live;
                changed = true;
            }

            if (input->normal_time != event->times.normal_time)
            {
                input->normal_time = event->times.normal_time;
                changed = true;
            }

            if (changed)
            {
                const struct heimdall_player_timer_point point = {
                    .position = input->position,
                    .rate = input->rate,
                    .ts = input->time,
                    .length = input->length,
                    .live = input->live,
                    .system_date = system_date,
                };
                heimdall_player_UpdateTimer(player, NULL, false, &point,
                                       input->normal_time, 0, 0, priv->i_start);
                heimdall_player_SignalAtoBLoop(player);
            }
            break;
        }
        case INPUT_EVENT_PROGRAM:
            heimdall_player_input_HandleProgramEvent(input, &event->program);
            break;
        case INPUT_EVENT_ES:
            heimdall_player_input_HandleEsEvent(input, &event->es);
            break;
        case INPUT_EVENT_TITLE:
            heimdall_player_input_HandleTitleEvent(input, &event->title);
            break;
        case INPUT_EVENT_CHAPTER:
            heimdall_player_input_HandleChapterEvent(input, &event->chapter);
            break;
        case INPUT_EVENT_RECORD:
            input->recording = event->record;
            heimdall_player_SendEvent(player, on_recording_changed, input->recording);
            break;
        case INPUT_EVENT_STATISTICS:
            input->stats = *event->stats;
            heimdall_player_SendEvent(player, on_statistics_changed, &input->stats);
            break;
        case INPUT_EVENT_SIGNAL:
            input->signal_quality = event->signal.quality;
            input->signal_strength = event->signal.strength;
            heimdall_player_SendEvent(player, on_signal_changed,
                                 input->signal_quality, input->signal_strength);
            break;
        case INPUT_EVENT_CACHE:
            if (event->cache == 0.0f)
                heimdall_player_UpdateTimerEvent(player, NULL,
                                            HEIMDALL_PLAYER_TIMER_EVENT_DISCONTINUITY,
                                            HEIMDALL_TICK_INVALID);
            input->cache = event->cache;
            heimdall_player_SendEvent(player, on_buffering_changed, event->cache);
            break;
        case INPUT_EVENT_VOUT:
            heimdall_player_input_HandleVoutEvent(input, &event->vout);
            break;
        case INPUT_EVENT_OUTPUT_STATE:
            if (event->output_state.action == HEIMDALL_INPUT_EVENT_OUTPUT_STATE_PAUSED)
                heimdall_player_UpdateTimerEvent(player, event->output_state.id,
                                            HEIMDALL_PLAYER_TIMER_EVENT_PAUSED,
                                            event->output_state.paused_date);
            break;
        case INPUT_EVENT_ITEM_META:
        case INPUT_EVENT_ITEM_INFO:
            heimdall_player_SendEvent(player, on_media_meta_changed,
                                 input_GetItem(input->thread));
            break;
        case INPUT_EVENT_ITEM_EPG:
            heimdall_player_SendEvent(player, on_media_epg_changed,
                                 input_GetItem(input->thread));
            break;
        case INPUT_EVENT_SUBITEMS:
            heimdall_player_SendEvent(player, on_media_subitems_changed,
                                 input_GetItem(input->thread), event->subitems);
            input_item_node_Delete(event->subitems);
            break;
        case INPUT_EVENT_DEAD:
            heimdall_player_destructor_AddJoinableInput(player, input);
            break;
        case INPUT_EVENT_VBI_PAGE:
            input->teletext_page = event->vbi_page < 999 ? event->vbi_page : 100;
            heimdall_player_SendEvent(player, on_teletext_page_changed,
                                 input->teletext_page);
            break;
        case INPUT_EVENT_VBI_TRANSPARENCY:
            input->teletext_transparent = event->vbi_transparent;
            heimdall_player_SendEvent(player, on_teletext_transparency_changed,
                                 input->teletext_transparent);
            break;
        case INPUT_EVENT_ATTACHMENTS:
            heimdall_player_SendEvent(player, on_media_attachments_added,
                                 input_GetItem(input->thread),
                                 event->attachments.array,
                                 event->attachments.count);
            break;
        case INPUT_EVENT_NAV_FAILED:
            heimdall_player_input_NavigationFallback(input, event->nav_type);
            break;
        case INPUT_EVENT_MOUSE_LEFT:
            heimdall_player_input_MouseFallback(input);
            break;
        case INPUT_EVENT_FRAME_NEXT_STATUS:
            heimdall_player_input_FrameNextStatus(input,
                                             event->frame_next_status);
            break;
        case INPUT_EVENT_FRAME_PREVIOUS_STATUS:
            heimdall_player_input_FramePreviousStatus(input,
                                                 event->frame_previous_status);
            break;
        default:
            handled = false;
            break;
    }

    heimdall_mutex_unlock(&player->lock);
    return handled;
}

void
heimdall_player_input_SelectTracksByStringIds(struct heimdall_player_input *input,
                                         enum es_format_category_e cat,
                                         const char *str_ids)
{
    input_SetEsCatIds(input->thread, cat, str_ids);
}

char *
heimdall_player_input_GetSelectedTrackStringIds(struct heimdall_player_input *input,
                                           enum es_format_category_e cat)
{
    heimdall_player_track_vector *vec = heimdall_player_input_GetTrackVector(input, cat);
    assert(vec);
    bool first_track = true;
    struct heimdall_memstream ms;

    struct heimdall_player_track_priv* t;
    heimdall_vector_foreach(t, vec)
    {
        if (t->selected_by_user && heimdall_es_id_IsStrIdStable(t->t.es_id))
        {
            if (first_track)
            {
                int ret = heimdall_memstream_open(&ms);
                if (ret != 0)
                    return NULL;
            }
            const char *str_id = heimdall_es_id_GetStrId(t->t.es_id);
            assert(str_id);

            if (!first_track)
                heimdall_memstream_putc(&ms, ',');
            heimdall_memstream_puts(&ms, str_id);
            first_track = false;
        }
    }
    return !first_track && heimdall_memstream_close(&ms) == 0 ? ms.ptr : NULL;
}

struct heimdall_player_input *
heimdall_player_input_New(heimdall_player_t *player, input_item_t *item)
{
    struct heimdall_player_input *input = malloc(sizeof(*input));
    if (!input)
        return NULL;

    input->player = player;
    input->started = false;
    input->playing = false;

    input->state = HEIMDALL_PLAYER_STATE_STOPPED;
    input->error = HEIMDALL_PLAYER_ERROR_NONE;
    input->rate = 1.f;
    input->capabilities = 0;
    input->length = input->time = HEIMDALL_TICK_INVALID;
    input->live = false;
    input->normal_time = HEIMDALL_TICK_0;
    input->pause_date = HEIMDALL_TICK_INVALID;
    input->position = 0.f;

    input->recording = false;

    input->cache = 0.f;
    input->signal_quality = input->signal_strength = -1.f;

    memset(&input->stats, 0, sizeof(input->stats));

    heimdall_vector_init(&input->program_vector);
    heimdall_vector_init(&input->video_track_vector);
    heimdall_vector_init(&input->audio_track_vector);
    heimdall_vector_init(&input->spu_track_vector);
    input->teletext_source = NULL;

    input->titles = NULL;
    input->title_selected = input->chapter_selected = 0;

    input->teletext_enabled = input->teletext_transparent = false;
    input->teletext_page = 0;

    input->abloop_state[0].set = input->abloop_state[1].set = false;

    memset(&input->ml.states, 0, sizeof(input->ml.states));
    input->ml.states.aspect_ratio = input->ml.states.crop =
        input->ml.states.deinterlace = input->ml.states.video_filter = NULL;
    input->ml.states.current_title = -1;
    input->ml.states.current_video_track =
        input->ml.states.current_audio_track =
        input->ml.states.current_subtitle_track = NULL;
    input->ml.restore = HEIMDALL_RESTOREPOINT_NONE;
    input->ml.restore_states = false;
    input->ml.delay_restore = false;
    input->ml.pos = -1.f;
    input->ml.has_audio_tracks = input->ml.has_video_tracks = false;

    static const struct heimdall_input_thread_callbacks cbs = {
        .on_event = input_thread_Events,
    };

    const struct heimdall_input_thread_cfg cfg = {
        .type = INPUT_TYPE_PLAYBACK,
        .hw_dec = INPUT_CFG_HW_DEC_DEFAULT,
        .resource = player->resource,
        .renderer = player->renderer,
        .cbs = &cbs,
        .cbs_data = input,
    };

    input->repeat = player->repeat;

    input->thread = input_Create(player, item, &cfg);
    if (!input->thread)
    {
        free(input);
        return NULL;
    }
    heimdall_player_input_RestoreMlStates(input, false);

    if (player->video_string_ids)
        heimdall_player_input_SelectTracksByStringIds(input, VIDEO_ES,
                                                 player->video_string_ids);

    if (player->audio_string_ids)
        heimdall_player_input_SelectTracksByStringIds(input, AUDIO_ES,
                                                 player->audio_string_ids);

    if (player->sub_string_ids)
        heimdall_player_input_SelectTracksByStringIds(input, SPU_ES,
                                                 player->sub_string_ids);

    /* Initial sub/audio delay */
    const heimdall_tick_t cat_delays[DATA_ES] = {
        [AUDIO_ES] =
            HEIMDALL_TICK_FROM_MS(var_InheritInteger(player, "audio-desync")),
        [SPU_ES] =
            heimdall_tick_from_samples(var_InheritInteger(player, "sub-delay"), 10),
    };

    for (enum es_format_category_e i = UNKNOWN_ES; i < DATA_ES; ++i)
    {
        input->cat_delays[i] = cat_delays[i];
        if (cat_delays[i] != 0)
        {
            int ret = input_SetEsCatDelay(input->thread, i, cat_delays[i]);
            if (ret == HEIMDALL_SUCCESS)
                heimdall_player_SendEvent(player, on_category_delay_changed, i,
                                     cat_delays[i]);
        }
    }
    return input;
}

void
heimdall_player_input_Delete(struct heimdall_player_input *input)
{
    assert(input->titles == NULL);
    assert(input->program_vector.size == 0);
    assert(input->video_track_vector.size == 0);
    assert(input->audio_track_vector.size == 0);
    assert(input->spu_track_vector.size == 0);
    assert(input->teletext_source == NULL);

    heimdall_vector_destroy(&input->program_vector);
    heimdall_vector_destroy(&input->video_track_vector);
    heimdall_vector_destroy(&input->audio_track_vector);
    heimdall_vector_destroy(&input->spu_track_vector);

    input_Close(input->thread);
    free(input);
}
