/*****************************************************************************
 * player.c: Player interface
 *****************************************************************************
 * Copyright © 2018 VLC authors and VideoLAN
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

#include "player.h"

void
heimdall_player_ResetTimer(heimdall_player_t *player)
{
    heimdall_mutex_lock(&player->timer.lock);

    player->timer.input_live = false;
    player->timer.input_length = HEIMDALL_TICK_INVALID;
    player->timer.input_normal_time = HEIMDALL_TICK_0;
    player->timer.last_ts = HEIMDALL_TICK_INVALID;
    player->timer.start_offset = 0;
    player->timer.input_position = 0;
    player->timer.smpte_source.smpte.last_framenum = ULONG_MAX;
    player->timer.seek_ts = HEIMDALL_TICK_INVALID;
    player->timer.seek_position = -1;
    player->timer.update_state = UPDATE_STATE_RESUMED;
    player->timer.pause_date = HEIMDALL_TICK_INVALID;
    player->timer.last_resume_date = HEIMDALL_TICK_INVALID;
    player->timer.stopping = false;

    heimdall_mutex_unlock(&player->timer.lock);
}

static void
heimdall_player_SendTimerSeek(heimdall_player_t *player,
                         struct heimdall_player_timer_source *source,
                         const struct heimdall_player_timer_point *point,
                         bool is_smpte)
{
    (void) player;
    heimdall_player_timer_id *timer;

    heimdall_list_foreach(timer, &source->listeners, node)
    {
        if (is_smpte)
        {
            if (timer->smpte_cbs->on_seek != NULL)
                timer->smpte_cbs->on_seek(point, timer->data);
        }
        else
        {
            if (timer->cbs->on_seek != NULL)
                timer->cbs->on_seek(point, timer->data);
        }
    }
}

static void
heimdall_player_SendTimerPause(heimdall_player_t *player,
                          struct heimdall_player_timer_source *source,
                          heimdall_tick_t system_date, bool is_smpte)
{
    (void) player;

    heimdall_player_timer_id *timer;
    heimdall_list_foreach(timer, &source->listeners, node)
    {
        if (is_smpte)
        {
            if (timer->smpte_cbs->on_paused != NULL)
                timer->smpte_cbs->on_paused(system_date, timer->data);
        }
        else
        {
            timer->last_update_date = HEIMDALL_TICK_INVALID;
            if (timer->cbs->on_paused != NULL)
                timer->cbs->on_paused(system_date, timer->data);
        }
    }
}

static void
heimdall_player_SendTimerSourceUpdates(heimdall_player_t *player,
                                  struct heimdall_player_timer_source *source,
                                  bool force_update,
                                  const struct heimdall_player_timer_point *point)
{
    (void) player;
    heimdall_player_timer_id *timer;

    heimdall_list_foreach(timer, &source->listeners, node)
    {
        /* Respect refresh delay of the timer */
        if (force_update || timer->period == HEIMDALL_TICK_INVALID
         || timer->last_update_date == HEIMDALL_TICK_INVALID
         || point->system_date == HEIMDALL_TICK_MAX /* always update when paused */
         || point->system_date - timer->last_update_date >= timer->period)
        {
            timer->cbs->on_update(point, timer->data);
            timer->last_update_date = point->system_date == HEIMDALL_TICK_MAX ?
                                      HEIMDALL_TICK_INVALID : point->system_date;
        }
    }
}

static void
heimdall_player_SendSmpteTimerSourceUpdates(heimdall_player_t *player,
                                       struct heimdall_player_timer_source *source,
                                       const struct heimdall_player_timer_point *point)
{
    (void) player;
    heimdall_player_timer_id *timer;

    struct heimdall_player_timer_smpte_timecode tc;
    unsigned long framenum;
    unsigned frame_rate;
    unsigned frame_rate_base;

    if (source->smpte.df > 0)
    {
        /* Use the exact SMPTE framerate that can be different from the input
         * source (at demuxer/decoder level) */
        assert(source->smpte.df_fps == 30 || source->smpte.df_fps == 60);
        frame_rate = source->smpte.df_fps * 1000;
        frame_rate_base = 1001;

        /* Convert the ts to a frame number */
        framenum = round(point->ts * frame_rate
                         / (double) frame_rate_base / HEIMDALL_TICK_FROM_SEC(1));

        /* Drop 2 or 4 frames every minutes except every 10 minutes in order to
         * make one hour of timecode match one hour on the clock. */
        ldiv_t res;
        res = ldiv(framenum, source->smpte.frames_per_10mins);

        framenum += (9 * source->smpte.df * res.quot)
                  + (source->smpte.df * ((res.rem - source->smpte.df)
                     / (source->smpte.frames_per_10mins / 10)));

        tc.drop_frame = true;

        /* Use 30 or 60 framerates for the next frames/seconds/minutes/hours
         * calculaton */
        frame_rate = source->smpte.df_fps;
        frame_rate_base = 1;
    }
    else
    {
        frame_rate = source->smpte.frame_rate;
        frame_rate_base = source->smpte.frame_rate_base;

        /* Convert the ts to a frame number */
        framenum = round(point->ts * frame_rate
                         / (double) frame_rate_base / HEIMDALL_TICK_FROM_SEC(1));

        tc.drop_frame = false;
    }
    if (framenum == source->smpte.last_framenum)
        return;

    source->smpte.last_framenum = framenum;

    tc.frames = framenum % (frame_rate / frame_rate_base);
    tc.seconds = (framenum * frame_rate_base / frame_rate) % 60;
    tc.minutes = (framenum * frame_rate_base / frame_rate / 60) % 60;
    tc.hours = framenum * frame_rate_base / frame_rate / 3600;

    tc.frame_resolution = source->smpte.frame_resolution;

    heimdall_list_foreach(timer, &source->listeners, node)
        timer->smpte_cbs->on_update(&tc, timer->data);
}

static void
heimdall_player_UpdateSmpteTimerFPS(heimdall_player_t *player,
                               struct heimdall_player_timer_source *source,
                               unsigned frame_rate, unsigned frame_rate_base)
{
    (void) player;
    source->smpte.frame_rate = frame_rate;
    source->smpte.frame_rate_base = frame_rate_base;

    /* Calculate everything that will be needed to create smpte timecodes */
    source->smpte.frame_resolution = 0;

    unsigned max_frames = frame_rate / frame_rate_base;

    if (max_frames == 29 && (100 * frame_rate / frame_rate_base) == 2997)
    {
        /* SMPTE Timecode: 29.97 fps DF */
        source->smpte.df = 2;
        source->smpte.df_fps = 30;
        source->smpte.frames_per_10mins = 17982; /* 29.97 * 60 * 10 */
    }
    else if (max_frames == 59 && (100 * frame_rate / frame_rate_base) == 5994)
    {
        /* SMPTE Timecode: 59.94 fps DF */
        source->smpte.df = 4;
        source->smpte.df_fps = 60;
        source->smpte.frames_per_10mins = 35964; /* 59.94 * 60 * 10 */
    }
    else
        source->smpte.df = 0;

    while (max_frames != 0)
    {
        max_frames /= 10;
        source->smpte.frame_resolution++;
    }
}

void
heimdall_player_UpdateTimerEvent(heimdall_player_t *player, heimdall_es_id_t *es_source,
                            enum heimdall_player_timer_event event,
                            heimdall_tick_t system_date)
{
    heimdall_mutex_lock(&player->timer.lock);

    /* Discontinuity is signalled by all output clocks and the input.
     * discard the event if it was already signalled or not on the good
     * es_source. */

    switch (event)
    {
        case HEIMDALL_PLAYER_TIMER_EVENT_DISCONTINUITY:
            assert(system_date == HEIMDALL_TICK_INVALID);
            for (size_t i = 0; i < HEIMDALL_PLAYER_TIMER_TYPE_COUNT; ++i)
            {
                struct heimdall_player_timer_source *source = &player->timer.sources[i];
                if (source->es != es_source)
                    continue;

                /* There can be several discontinuities on the same source
                 * for one seek request, hence the need of the
                 * 'timer.seeking' variable to notify only once the end of
                 * the seek request. */
                if (source->seeking)
                {
                    source->seeking = false;
                    heimdall_player_SendTimerSeek(player, source, NULL,
                                             i == HEIMDALL_PLAYER_TIMER_TYPE_SMPTE);
                }
                source->point.system_date = HEIMDALL_TICK_INVALID;
            }
            break;

        case HEIMDALL_PLAYER_TIMER_EVENT_PAUSED:
            assert(system_date != HEIMDALL_TICK_INVALID);

            /* An ES PAUSED event can be delivered after its pause was
             * resumed. It is still reported: per-ES events are delivered
             * in order, but it should not touch player timer state. */
            if (es_source == NULL
             || player->timer.last_resume_date == HEIMDALL_TICK_INVALID
             || system_date >= player->timer.last_resume_date)
            {
                player->timer.update_state = UPDATE_STATE_PAUSED;
                player->timer.pause_date = system_date;
            }

            for (size_t i = 0; i < HEIMDALL_PLAYER_TIMER_TYPE_COUNT; ++i)
            {
                struct heimdall_player_timer_source *source = &player->timer.sources[i];
                if (source->es != es_source)
                    continue;

                /* The input and the output signal the same pause with the
                 * same date */
                if (source->reported_pause_date == system_date)
                    continue;
                source->reported_pause_date = system_date;
                heimdall_player_SendTimerPause(player, source, system_date,
                                          i == HEIMDALL_PLAYER_TIMER_TYPE_SMPTE);
            }

            break;

        case HEIMDALL_PLAYER_TIMER_EVENT_PLAYING:
            assert(!player->timer.stopping);
            if (system_date != HEIMDALL_TICK_INVALID)
                player->timer.last_resume_date = system_date;
            player->timer.update_state = UPDATE_STATE_RESUMING;
            break;

        case HEIMDALL_PLAYER_TIMER_EVENT_STOPPING:
            player->timer.stopping = true;
            for (size_t i = 0; i < HEIMDALL_PLAYER_TIMER_TYPE_COUNT; ++i)
            {
                struct heimdall_player_timer_source *source = &player->timer.sources[i];
                heimdall_player_SendTimerPause(player, source, system_date,
                                          i == HEIMDALL_PLAYER_TIMER_TYPE_SMPTE);
            }
            break;

        default:
            heimdall_assert_unreachable();
    }

    heimdall_mutex_unlock(&player->timer.lock);
}

void
heimdall_player_UpdateTimerSeekState(heimdall_player_t *player, heimdall_tick_t time,
                                double position)
{
    heimdall_mutex_lock(&player->timer.lock);
    struct heimdall_player_timer_source *source = &player->timer.best_source;

    if (time == HEIMDALL_TICK_INVALID)
    {
        assert(position >= 0);
        if (source->point.length != HEIMDALL_TICK_INVALID)
            player->timer.seek_ts = position * source->point.length;
        else
            player->timer.seek_ts = HEIMDALL_TICK_INVALID;
    }
    else
        player->timer.seek_ts = time;

    if (position < 0)
    {
        assert(time != HEIMDALL_TICK_INVALID);
        if (source->point.length != HEIMDALL_TICK_INVALID && !source->point.live)
            player->timer.seek_position = time / (double) source->point.length;
    }
    else
        player->timer.seek_position = position;

    const struct heimdall_player_timer_point point =
    {
        .position = player->timer.seek_position,
        .rate = source->point.rate,
        .ts = player->timer.seek_ts,
        .length = source->point.length,
        .live = source->point.live,
        .system_date = HEIMDALL_TICK_MAX,
    };

    source->seeking = true;
    heimdall_player_SendTimerSeek(player, source, &point, false);

    source = &player->timer.smpte_source;
    source->seeking = true;
    heimdall_player_SendTimerSeek(player, source, &point, true);

    heimdall_mutex_unlock(&player->timer.lock);
}

static void
heimdall_player_UpdateTimerSource(heimdall_player_t *player,
                             struct heimdall_player_timer_source *source,
                             double rate, heimdall_tick_t ts, heimdall_tick_t system_date)
{
    assert(ts >= HEIMDALL_TICK_0);
    assert(player->timer.input_normal_time >= HEIMDALL_TICK_0);

    source->point.rate = rate;
    source->point.ts = ts - player->timer.input_normal_time - player->timer.start_offset + HEIMDALL_TICK_0;
    source->point.length = player->timer.input_length;
    source->point.live = player->timer.input_live;

    /* Put an invalid date for the first point in order to disable
     * interpolation (behave as paused), indeed, we should wait for one more
     * point before starting interpolation (ideally, it should be more) */
    if (source->point.system_date == HEIMDALL_TICK_INVALID)
        source->point.system_date = HEIMDALL_TICK_MAX;
    else
        source->point.system_date = system_date;

    if (source->point.length != HEIMDALL_TICK_INVALID && !source->point.live)
        source->point.position = (ts - player->timer.input_normal_time - player->timer.start_offset)
                               / (double) source->point.length;
    else
        source->point.position = player->timer.input_position;
}

static void
heimdall_player_UpdateTimerBestSource(heimdall_player_t *player, heimdall_es_id_t *es_source,
                                 bool es_source_is_master,
                                 const struct heimdall_player_timer_point *point,
                                 heimdall_tick_t system_date,
                                 bool force_update)
{
    /* Best source priority:
     * 1/ es_source != NULL + master (from the master ES track)
     * 2/ es_source != NULL (from the first ES track updated)
     * 3/ es_source == NULL (from the input)
     */
    struct heimdall_player_timer_source *source = &player->timer.best_source;
    if (!source->es || es_source_is_master)
        source->es = es_source;

    /* Notify the best source */
    if (source->es == es_source || force_update)
    {
        if (source->point.rate != point->rate)
        {
            player->timer.last_ts = HEIMDALL_TICK_INVALID;
            force_update = true;
        }

        /* When paused (HEIMDALL_TICK_MAX), the same ts can be send more than one
         * time from the video source, only send it if different in that case.
         */
        if (point->ts != player->timer.last_ts
          || (source->point.system_date != system_date && system_date != HEIMDALL_TICK_MAX))
        {
            heimdall_player_UpdateTimerSource(player, source, point->rate, point->ts,
                                         system_date);
            player->timer.last_ts = point->ts;

            /* It is possible to receive valid points while seeking. These
             * points could be updated when the input thread didn't yet process
             * the seek request. */
            if (!source->seeking)
            {
                /* Reset seek time/position now that we receive a valid point
                 * and seek was processed */
                player->timer.seek_ts = HEIMDALL_TICK_INVALID;
                player->timer.seek_position = -1;
            }

            if (!heimdall_list_is_empty(&source->listeners))
                heimdall_player_SendTimerSourceUpdates(player, source, force_update,
                                                  &source->point);

            if (player->timer.update_state == UPDATE_STATE_RESUMING)
            {
                player->timer.update_state = UPDATE_STATE_RESUMED;
                player->timer.pause_date = HEIMDALL_TICK_INVALID;
            }
        }
    }
}

static void
heimdall_player_UpdateTimerSmpteSource(heimdall_player_t *player, heimdall_es_id_t *es_source,
                                  const struct heimdall_player_timer_point *point,
                                  heimdall_tick_t system_date,
                                  unsigned frame_rate, unsigned frame_rate_base)
{
    struct heimdall_player_timer_source *source = &player->timer.smpte_source;
    /* SMPTE source: only the video source */
    if (!source->es && es_source && heimdall_es_id_GetCat(es_source) == VIDEO_ES)
        source->es = es_source;

    /* Notify the SMPTE source, also notify when the video output was rendered
     * while the clock was paused */
    if (source->es == es_source && source->es)
    {
        if (frame_rate != 0 && (frame_rate != source->smpte.frame_rate
         || frame_rate_base != source->smpte.frame_rate_base))
        {
            assert(frame_rate_base != 0);
            heimdall_player_UpdateSmpteTimerFPS(player, source, frame_rate,
                                           frame_rate_base);
        }

        if (source->smpte.frame_rate != 0)
        {
            heimdall_player_UpdateTimerSource(player, source, point->rate, point->ts,
                                         system_date);

            if (!heimdall_list_is_empty(&source->listeners))
                heimdall_player_SendSmpteTimerSourceUpdates(player, source,
                                                       &source->point);
        }
    }
}

void
heimdall_player_UpdateTimer(heimdall_player_t *player, heimdall_es_id_t *es_source,
                       bool es_source_is_master,
                       const struct heimdall_player_timer_point *point,
                       heimdall_tick_t normal_time,
                       unsigned frame_rate, unsigned frame_rate_base,
                       heimdall_tick_t start_offset)
{
    assert(point);
    /* A null source can't be the master */
    assert(es_source == NULL ? !es_source_is_master : true);

    heimdall_mutex_lock(&player->timer.lock);

    bool force_update = false;
    if (!es_source) /* input source */
    {
        /* Only valid for input sources */
        if (normal_time != HEIMDALL_TICK_INVALID
         && player->timer.input_normal_time != normal_time)
        {
            player->timer.input_normal_time = normal_time;
            player->timer.last_ts = HEIMDALL_TICK_INVALID;
            force_update = true;
        }
        if (player->timer.input_length != point->length
         && point->length >= HEIMDALL_TICK_0)
        {
            player->timer.input_length = point->length;
            player->timer.last_ts = HEIMDALL_TICK_INVALID;
            force_update = true;
        }
        if (player->timer.input_live != point->live)
        {
            player->timer.input_live = point->live;
            force_update = true;
        }
        if (start_offset > 0)
            player->timer.start_offset = start_offset;
        /* Will likely be overridden by non input source */
        player->timer.input_position = point->position;

        if (point->ts == HEIMDALL_TICK_INVALID
         || point->system_date == HEIMDALL_TICK_INVALID)
        {
            /* ts can only be invalid from the input source */
            heimdall_mutex_unlock(&player->timer.lock);
            return;
        }
    }

    assert(point->ts != HEIMDALL_TICK_INVALID);

    heimdall_tick_t system_date = point->system_date;
    if (player->timer.update_state == UPDATE_STATE_PAUSED)
    {
        if (es_source != NULL && point->system_date == HEIMDALL_TICK_MAX)
        {
            /* Update was forced, probably a next/prev frame */
            force_update = true;
        }
        system_date = HEIMDALL_TICK_MAX;
    }

    if (!player->timer.stopping)
        heimdall_player_UpdateTimerBestSource(player, es_source,
                                         es_source_is_master, point, system_date,
                                         force_update);

    heimdall_player_UpdateTimerSmpteSource(player, es_source, point, system_date,
                                      frame_rate, frame_rate_base);

    heimdall_mutex_unlock(&player->timer.lock);
}

void
heimdall_player_RemoveTimerSource(heimdall_player_t *player, heimdall_es_id_t *es_source)
{
    heimdall_mutex_lock(&player->timer.lock);
    struct heimdall_player_timer_source *bestsource = &player->timer.best_source;
    struct heimdall_player_timer_source *smptesource = &player->timer.smpte_source;

    /* Unlikely case where the source ES is deleted while seeking */
    if (bestsource->es == es_source && bestsource->seeking)
    {
        bestsource->seeking = false;
        heimdall_player_SendTimerSeek(player, bestsource, NULL, false);
    }
    if (smptesource->es == es_source && smptesource->seeking)
    {
        smptesource->seeking = false;
        heimdall_player_SendTimerSeek(player, smptesource, NULL, true);
    }

    for (size_t i = 0; i < HEIMDALL_PLAYER_TIMER_TYPE_COUNT; ++i)
    {
        struct heimdall_player_timer_source *source = &player->timer.sources[i];
        if (source->es == es_source)
        {
            /* Discontinuity should have been already signaled */
            assert(source->point.system_date == HEIMDALL_TICK_INVALID);
            source->es = NULL;
        }
    }
    heimdall_mutex_unlock(&player->timer.lock);
}

int
heimdall_player_GetTimerPoint(heimdall_player_t *player, bool *seeking,
                         heimdall_tick_t system_now,
                         heimdall_tick_t *out_ts, double *out_pos)
{
    int ret = HEIMDALL_EGENERIC;
    heimdall_mutex_lock(&player->timer.lock);
    bool timer_seeking = player->timer.seek_ts != HEIMDALL_TICK_INVALID
                      || player->timer.seek_position >= 0.0f;
    if (*seeking && timer_seeking)
    {
        if (out_ts != NULL)
        {
            if (player->timer.seek_ts == HEIMDALL_TICK_INVALID)
                goto end;
            *out_ts = player->timer.seek_ts;
        }
        if (out_pos != NULL)
        {
            if (player->timer.seek_position < 0)
                goto end;
            *out_pos = player->timer.seek_position;
        }
        ret = HEIMDALL_SUCCESS;
        goto end;
    }

    if (player->timer.best_source.point.system_date == HEIMDALL_TICK_INVALID)
        goto end;

    if (system_now != HEIMDALL_TICK_INVALID) /* interpolate */
    {
        /* If paused, force interpolation to the paused date */
        if (player->timer.pause_date != HEIMDALL_TICK_INVALID)
            system_now = player->timer.pause_date;
        ret = heimdall_player_timer_point_Interpolate(&player->timer.best_source.point,
                                                 system_now, out_ts, out_pos);
    }
    else
    {
        const struct heimdall_player_timer_point *point =
            &player->timer.best_source.point;
        if (out_ts)
            *out_ts = point->ts;
        if (out_pos)
            *out_pos = point->position;
        ret = HEIMDALL_SUCCESS;
    }

    if (ret == HEIMDALL_SUCCESS)
        *seeking = timer_seeking;
end:
    heimdall_mutex_unlock(&player->timer.lock);
    return ret;
}

heimdall_player_timer_id *
heimdall_player_AddTimer(heimdall_player_t *player, heimdall_tick_t min_period,
                    const struct heimdall_player_timer_cbs *cbs, void *data)
{
    assert(min_period >= HEIMDALL_TICK_0 || min_period == HEIMDALL_TICK_INVALID);
    assert(cbs && cbs->on_update);

    struct heimdall_player_timer_id *timer = malloc(sizeof(*timer));
    if (!timer)
        return NULL;
    timer->period = min_period;
    timer->last_update_date = HEIMDALL_TICK_INVALID;
    timer->cbs = cbs;
    timer->data = data;

    heimdall_mutex_lock(&player->timer.lock);
    heimdall_list_append(&timer->node, &player->timer.best_source.listeners);
    heimdall_mutex_unlock(&player->timer.lock);

    return timer;
}

heimdall_player_timer_id *
heimdall_player_AddSmpteTimer(heimdall_player_t *player,
                         const struct heimdall_player_timer_smpte_cbs *cbs,
                         void *data)
{
    assert(cbs && cbs->on_update);

    struct heimdall_player_timer_id *timer = malloc(sizeof(*timer));
    if (!timer)
        return NULL;
    timer->period = HEIMDALL_TICK_INVALID;
    timer->last_update_date = HEIMDALL_TICK_INVALID;
    timer->smpte_cbs = cbs;
    timer->data = data;

    heimdall_mutex_lock(&player->timer.lock);
    heimdall_list_append(&timer->node, &player->timer.smpte_source.listeners);
    heimdall_mutex_unlock(&player->timer.lock);

    return timer;
}

void
heimdall_player_RemoveTimer(heimdall_player_t *player, heimdall_player_timer_id *timer)
{
    assert(timer);

    heimdall_mutex_lock(&player->timer.lock);
    heimdall_list_remove(&timer->node);
    heimdall_mutex_unlock(&player->timer.lock);

    free(timer);
}

int
heimdall_player_timer_point_Interpolate(const struct heimdall_player_timer_point *point,
                                   heimdall_tick_t system_now,
                                   heimdall_tick_t *out_ts, double *out_pos)
{
    assert(point);
    assert(system_now > 0);
    assert(out_ts || out_pos);

    /* A system_date == HEIMDALL_TICK_MAX means the clock was paused when it updated
     * this point, so there is nothing to interpolate */
    const heimdall_tick_t drift = point->system_date == HEIMDALL_TICK_MAX ? 0
                           : (system_now - point->system_date) * point->rate;
    heimdall_tick_t ts = point->ts;
    double pos = point->position;

    if (ts != HEIMDALL_TICK_INVALID)
    {
        ts += drift;
        if (unlikely(ts < HEIMDALL_TICK_0))
            return HEIMDALL_EGENERIC;
    }
    if (point->length != HEIMDALL_TICK_INVALID && !point->live)
    {
        pos += drift / (double) point->length;
        if (unlikely(pos < 0.f))
            return HEIMDALL_EGENERIC;
        if (pos > 1.f)
            pos = 1.f;
        if (ts > point->length)
            ts = point->length;
    }

    if (out_ts)
        *out_ts = ts;
    if (out_pos)
        *out_pos = pos;

    return HEIMDALL_SUCCESS;
}

heimdall_tick_t
heimdall_player_timer_point_GetNextIntervalDate(const struct heimdall_player_timer_point *point,
                                           heimdall_tick_t system_now,
                                           heimdall_tick_t interpolated_ts,
                                           heimdall_tick_t next_interval)
{
    assert(point);
    assert(system_now > 0);
    assert(next_interval != HEIMDALL_TICK_INVALID);

    const unsigned ts_interval = interpolated_ts / next_interval;
    const heimdall_tick_t ts_next_interval =
        ts_interval * next_interval + next_interval;

    return (ts_next_interval - interpolated_ts) / point->rate + system_now;
}

void
heimdall_player_InitTimer(heimdall_player_t *player)
{
    heimdall_mutex_init(&player->timer.lock);

    for (size_t i = 0; i < HEIMDALL_PLAYER_TIMER_TYPE_COUNT; ++i)
    {
        heimdall_list_init(&player->timer.sources[i].listeners);
        player->timer.sources[i].point.system_date = HEIMDALL_TICK_INVALID;
        player->timer.sources[i].es = NULL;
        player->timer.sources[i].seeking = false;
        player->timer.sources[i].reported_pause_date = HEIMDALL_TICK_INVALID;
    }
    heimdall_player_ResetTimer(player);
}

void
heimdall_player_DestroyTimer(heimdall_player_t *player)
{
    for (size_t i = 0; i < HEIMDALL_PLAYER_TIMER_TYPE_COUNT; ++i)
        assert(heimdall_list_is_empty(&player->timer.sources[i].listeners));
}
