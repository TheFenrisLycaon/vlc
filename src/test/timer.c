/*****************************************************************************
 * timer.c: Test for timer API
 *****************************************************************************
 * Copyright (C) 2009 Rémi Denis-Courmont
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

#include <stdio.h>
#include <stdlib.h>
#undef NDEBUG
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#undef heimdall_tick_sleep

const char heimdall_module_name[] = "test_timer";

struct timer_data
{
    heimdall_timer_t timer;
    heimdall_mutex_t lock;
    heimdall_cond_t  wait;
    unsigned count;
};

static void callback (void *ptr)
{
    struct timer_data *data = ptr;

    heimdall_mutex_lock (&data->lock);
    data->count += 1 + heimdall_timer_getoverrun (data->timer);
    heimdall_cond_signal (&data->wait);
    heimdall_mutex_unlock (&data->lock);
}


int main (void)
{
    struct timer_data data;
    heimdall_tick_t ts;
    int val;

    heimdall_mutex_init (&data.lock);
    heimdall_cond_init (&data.wait);
    data.count = 0;

    val = heimdall_timer_create (&data.timer, callback, &data);
    assert (val == 0);
    heimdall_timer_destroy (data.timer);
    assert (data.count == 0);

    val = heimdall_timer_create (&data.timer, callback, &data);
    assert (val == 0);
    heimdall_timer_schedule (data.timer, false, CLOCK_FREQ << 20, CLOCK_FREQ);
    heimdall_timer_destroy (data.timer);
    assert (data.count == 0);

    val = heimdall_timer_create (&data.timer, callback, &data);
    assert (val == 0);

    /* Relative timer */
    ts = heimdall_tick_now ();
    heimdall_timer_schedule (data.timer, false, 1, HEIMDALL_TICK_FROM_MS(10));

    heimdall_mutex_lock (&data.lock);
    while (data.count <= 10)
        heimdall_cond_wait(&data.wait, &data.lock);

    ts = heimdall_tick_now () - ts;
    printf ("%u iterations in %"PRId64" us\n", data.count, ts);
    data.count = 0;
    heimdall_mutex_unlock (&data.lock);
    assert(ts >= HEIMDALL_TICK_FROM_MS(100));

    heimdall_timer_disarm (data.timer);

    /* Absolute timer */
    ts = heimdall_tick_now ();

    heimdall_timer_schedule (data.timer, true, ts + HEIMDALL_TICK_FROM_MS(100),
                        HEIMDALL_TICK_FROM_MS(10));

    heimdall_mutex_lock (&data.lock);
    while (data.count <= 10)
        heimdall_cond_wait(&data.wait, &data.lock);

    ts = heimdall_tick_now () - ts;
    printf ("%u iterations in %"PRId64" us\n", data.count, ts);
    heimdall_mutex_unlock (&data.lock);
    assert(ts >= HEIMDALL_TICK_FROM_MS(200));

    heimdall_timer_destroy (data.timer);
    return 0;
}
