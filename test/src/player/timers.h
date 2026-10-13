// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * common.h: common helpers for the player test
 *****************************************************************************
 * Copyright (C) 2018-2025 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *****************************************************************************/

#include <heimdall_common.h>
#include <heimdall_player.h>

struct report_timer
{
    enum
    {
        REPORT_TIMER_POINT,
        REPORT_TIMER_TC,
        REPORT_TIMER_PAUSED,
        REPORT_TIMER_SEEK,
    } type;
    union
    {
        struct heimdall_player_timer_point point;
        struct heimdall_player_timer_smpte_timecode tc;
        heimdall_tick_t paused_date;
        struct {
            struct heimdall_player_timer_point point;
            bool finished;
        } seek;
    };
};
typedef struct HEIMDALL_VECTOR(struct report_timer) vec_report_timer;

struct timer_state
{
    heimdall_player_timer_id *id;
    heimdall_tick_t delay;
    vec_report_timer vec;
    size_t last_report_idx;
    heimdall_mutex_t lock;
    heimdall_cond_t wait;
};

static inline void
timers_on_update(const struct heimdall_player_timer_point *point, void *data)
{
    struct timer_state *timer = data;
    struct report_timer report =
    {
        .type = REPORT_TIMER_POINT,
        .point = *point,
    };
    heimdall_mutex_lock(&timer->lock);
    bool success = heimdall_vector_push(&timer->vec, report);
    heimdall_cond_signal(&timer->wait);
    heimdall_mutex_unlock(&timer->lock);
    assert(success);
}

static inline void
timers_on_paused(heimdall_tick_t system_date, void *data)
{
    struct timer_state *timer = data;
    struct report_timer report =
    {
        .type = REPORT_TIMER_PAUSED,
        .paused_date = system_date,
    };
    heimdall_mutex_lock(&timer->lock);
    bool success = heimdall_vector_push(&timer->vec, report);
    heimdall_cond_signal(&timer->wait);
    heimdall_mutex_unlock(&timer->lock);
    assert(success);
}

static inline void
timers_on_seek(const struct heimdall_player_timer_point *point, void *data)
{
    struct timer_state *timer = data;
    struct report_timer report =
    {
        .type = REPORT_TIMER_SEEK,
    };
    report.seek.finished = point == NULL;
    if (!report.seek.finished)
        report.seek.point = *point;
    heimdall_mutex_lock(&timer->lock);
    bool success = heimdall_vector_push(&timer->vec, report);
    heimdall_cond_signal(&timer->wait);
    heimdall_mutex_unlock(&timer->lock);
    assert(success);
}

static inline void
timers_smpte_on_update(const struct heimdall_player_timer_smpte_timecode *tc,
                       void *data)
{
    struct timer_state *timer = data;
    struct report_timer report =
    {
        .type = REPORT_TIMER_TC,
        .tc = *tc,
    };
    heimdall_mutex_lock(&timer->lock);
    bool success = heimdall_vector_push(&timer->vec, report);
    heimdall_cond_signal(&timer->wait);
    heimdall_mutex_unlock(&timer->lock);
    assert(success);
}

static inline void
player_add_timer(heimdall_player_t *player, struct timer_state *timer, bool smpte,
                 heimdall_tick_t delay)
{
    static const struct heimdall_player_timer_cbs cbs =
    {
        .on_update = timers_on_update,
        .on_paused = timers_on_paused,
        .on_seek = timers_on_seek,
    };

    static const struct heimdall_player_timer_smpte_cbs smpte_cbs =
    {
        .on_update = timers_smpte_on_update,
        .on_paused = timers_on_paused,
        .on_seek = timers_on_seek,
    };

    heimdall_vector_init(&timer->vec);
    heimdall_mutex_init(&timer->lock);
    heimdall_cond_init(&timer->wait);
    timer->last_report_idx = 0;
    timer->delay = delay;
    if (smpte)
        timer->id = heimdall_player_AddSmpteTimer(player, &smpte_cbs, timer);
    else
        timer->id = heimdall_player_AddTimer(player, timer->delay, &cbs, timer);
    assert(timer->id);
}

static inline void
player_remove_timer(heimdall_player_t *player, struct timer_state *timer)
{
    heimdall_vector_clear(&timer->vec);
    heimdall_player_RemoveTimer(player, timer->id);
}

static inline void
player_lock_timer(heimdall_player_t *player, struct timer_state *timer)
{
    heimdall_player_Unlock(player);
    heimdall_mutex_lock(&timer->lock);
}

static inline void
player_unlock_timer(heimdall_player_t *player, struct timer_state *timer)
{
    heimdall_mutex_unlock(&timer->lock);
    heimdall_player_Lock(player);
}

static inline struct report_timer *
timer_state_wait_next_report(struct timer_state *timer)
{
    while (timer->vec.size < timer->last_report_idx + 1)
        heimdall_cond_wait(&timer->wait, &timer->lock);
    struct report_timer *r = &timer->vec.data[timer->last_report_idx];
    timer->last_report_idx++;
    return r;
}