/*****************************************************************************
 * freebsd/thread.c: FreeBSD specifics for atomic wait
 *****************************************************************************
 * Copyright (C) 2020 Rémi Denis-Courmont
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

#include <assert.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <pthread.h>
#include <pthread_np.h>
#include <sys/thr.h>
#include <sys/umtx.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_atomic.h>

unsigned long heimdall_thread_id(void)
{
     static _Thread_local int tid = -1;

     if (unlikely(tid == -1))
         tid = thr_self();

     return tid;
}

void (heimdall_thread_set_name)(const char *name)
{
    pthread_set_name_np(pthread_self(), name);
}

static int heimdall_umtx_wake(void *addr, int nr)
{
    return _umtx_op(addr, UMTX_OP_WAKE_PRIVATE, nr, NULL, NULL);
}

static int heimdall_umtx_wait(void *addr, unsigned val, const struct timespec *ts)
{
    struct _umtx_time to = {
        ._timeout = *ts,
        ._flags = UMTX_ABSTIME,
        ._clockid = CLOCK_MONOTONIC,
    };
    int type;
    /* "the uaddr value [...] must be equal to the size of the structure
     * pointed to by uaddr2, casted to uintptr_t."
     */
    void *uaddr = (void *)sizeof (to);

    return _umtx_op(addr, UMTX_OP_WAIT_UINT_PRIVATE, val, uaddr, &to);
}

void heimdall_atomic_notify_one(void *addr)
{
    heimdall_umtx_wake(addr, 1);
}

void heimdall_atomic_notify_all(void *addr)
{
    heimdall_umtx_wake(addr, INT_MAX);
}

void heimdall_atomic_wait(void *addr, unsigned val)
{
    int ret = heimdall_umtx_wait(addr, val, NULL);

    assert(ret == 0 || ret == EINTR || ret == ERESTART);
}

int heimdall_atomic_timedwait(void *addr, unsigned val, heimdall_tick_t deadline)
{
    struct timespec ts;
    int ret = heimdall_umtx_wait(addr, val, heimdall_tick_to_timespec(&ts, deadline));

    assert(ret == 0 || ret == ETIMEDOUT || ret == EINTR || ret == ERESTART);
    return (ret != ETIMEDOUT) ? 0 : ret;
}
