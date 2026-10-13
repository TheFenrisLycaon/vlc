/*****************************************************************************
 * timer.c: simple threaded timer
 *****************************************************************************
 * Copyright (C) 2009-2012 Rémi Denis-Courmont
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

#include <stdatomic.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>

/*
 * POSIX timers are essentially unusable from a library: there provide no safe
 * way to ensure that a timer has no pending/ongoing iteration. Furthermore,
 * they typically require one thread per timer plus one thread per iteration,
 * which is inefficient and overkill (unless you need multiple iteration
 * of the same timer concurrently).
 * Thus, this is a generic manual implementation of timers using a thread.
 */

struct heimdall_timer
{
    heimdall_thread_t thread;
    heimdall_cond_t   reschedule;
    heimdall_mutex_t  lock;
    void       (*func) (void *);
    void        *data;
    heimdall_tick_t   value, interval;
    bool         live;
    atomic_uint  overruns;
};

static void *heimdall_timer_thread (void *data)
{
    heimdall_thread_set_name("heimdall-timer");

    struct heimdall_timer *timer = data;

    heimdall_mutex_lock (&timer->lock);

    while (timer->live)
    {
        if (timer->value == 0)
        {
            assert(timer->interval == 0);
            heimdall_cond_wait (&timer->reschedule, &timer->lock);
            continue;
        }

        if (timer->interval != 0)
        {
            heimdall_tick_t now = heimdall_tick_now();

            if (now > timer->value)
            {   /* Update overrun counter */
                unsigned misses = (now - timer->value) / timer->interval;

                timer->value += misses * timer->interval;
                assert(timer->value <= now);
                atomic_fetch_add_explicit(&timer->overruns, misses,
                                          memory_order_relaxed);
            }
        }

        heimdall_tick_t value = timer->value;

        if (heimdall_cond_timedwait(&timer->reschedule, &timer->lock, value) == 0)
            continue;

        if (likely(timer->value <= value))
        {
            timer->value += timer->interval; /* rearm */

            if (timer->interval == 0)
                timer->value = 0; /* disarm */
        }

        heimdall_mutex_unlock (&timer->lock);
        timer->func (timer->data);
        heimdall_mutex_lock (&timer->lock);
    }

    heimdall_mutex_unlock (&timer->lock);
    return NULL;
}

int heimdall_timer_create (heimdall_timer_t *id, void (*func) (void *), void *data)
{
    struct heimdall_timer *timer = malloc (sizeof (*timer));

    if (unlikely(timer == NULL))
        return ENOMEM;
    heimdall_mutex_init (&timer->lock);
    heimdall_cond_init (&timer->reschedule);
    assert (func);
    timer->func = func;
    timer->data = data;
    timer->value = 0;
    timer->interval = 0;
    timer->live = true;
    atomic_init(&timer->overruns, 0);

    if (heimdall_clone (&timer->thread, heimdall_timer_thread, timer))
    {
        free (timer);
        return ENOMEM;
    }

    *id = timer;
    return 0;
}

void heimdall_timer_destroy (heimdall_timer_t timer)
{
    heimdall_mutex_lock(&timer->lock);
    timer->live = false;
    heimdall_cond_signal(&timer->reschedule);
    heimdall_mutex_unlock(&timer->lock);

    heimdall_join (timer->thread, NULL);
    free (timer);
}

void heimdall_timer_schedule (heimdall_timer_t timer, bool absolute,
                         heimdall_tick_t value, heimdall_tick_t interval)
{
    if (value == HEIMDALL_TIMER_DISARM)
        interval = 0;
    else
    if (!absolute)
        value += heimdall_tick_now();

    heimdall_mutex_lock (&timer->lock);
    timer->value = value;
    timer->interval = interval;
    heimdall_cond_signal (&timer->reschedule);
    heimdall_mutex_unlock (&timer->lock);
}

unsigned heimdall_timer_getoverrun (heimdall_timer_t timer)
{
    return atomic_exchange_explicit (&timer->overruns, 0,
                                     memory_order_relaxed);
}
