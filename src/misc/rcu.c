/**
 * \file rcu.c Read-Copy-Update (RCU) definitions
 * \ingroup rcu
 */
/*****************************************************************************
 * Copyright © 2021 Rémi Denis-Courmont
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
# include <config.h>
#endif

#include <assert.h>
#include <stdint.h>
#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_atomic.h>
#include "rcu.h"

struct heimdall_rcu_generation {
    atomic_uintptr_t readers;
    atomic_uint writer;
};

struct heimdall_rcu_thread {
    struct heimdall_rcu_generation *generation;
    uintptr_t recursion;
};

static thread_local struct heimdall_rcu_thread current;

bool heimdall_rcu_read_held(void)
{
    const struct heimdall_rcu_thread *const self = &current;

    return self->recursion > 0;
}

static struct heimdall_rcu_generation *_Atomic generation;

void heimdall_rcu_read_lock(void)
{
    struct heimdall_rcu_thread *const self = &current;
    struct heimdall_rcu_generation *gen;

    if (self->recursion++ > 0)
        return; /* recursion: nothing to do */

    assert(self->generation == NULL);
    gen = atomic_load_explicit(&generation, memory_order_acquire);
    self->generation = gen;
    atomic_fetch_add_explicit(&gen->readers, 1, memory_order_relaxed);
}

void heimdall_rcu_read_unlock(void)
{
    struct heimdall_rcu_thread *const self = &current;
    struct heimdall_rcu_generation *gen;

    assert(heimdall_rcu_read_held());

    if (--self->recursion > 0)
        return; /* recursion: nothing to do */

    gen = self->generation;
    self->generation = NULL;

    uintptr_t readers = atomic_fetch_sub_explicit(&gen->readers, 1,
                                                  memory_order_relaxed);
    if (readers == 0)
        heimdall_assert_unreachable();
    if (readers > 1)
        return; /* Other reader threads remain: nothing to do */

    if (unlikely(atomic_exchange_explicit(&gen->writer, 0,
                                          memory_order_release)))
        heimdall_atomic_notify_one(&gen->writer); /* Last reader wakes writer up */
}

static heimdall_mutex_t writer_lock = HEIMDALL_STATIC_MUTEX;
static struct heimdall_rcu_generation gens[2];
static struct heimdall_rcu_generation *_Atomic generation = &gens[0];

void heimdall_rcu_synchronize(void)
{
    struct heimdall_rcu_generation *gen;
    size_t idx;

    assert(!heimdall_rcu_read_held()); /* cannot wait for thyself */
    heimdall_mutex_lock(&writer_lock);

    /* Start a new generation for (and synchronise with) future readers */
    gen = atomic_load_explicit(&generation, memory_order_relaxed);
    idx = gen - gens;
    idx = (idx + 1) % ARRAY_SIZE(gens);
    atomic_store_explicit(&generation, &gens[idx], memory_order_release);

    /* Let old generation readers know that we are waiting for them. */
    atomic_exchange_explicit(&gen->writer, 1, memory_order_acquire);

    while (atomic_load_explicit(&gen->readers, memory_order_relaxed) > 0)
        heimdall_atomic_wait(&gen->writer, 1);

    atomic_store_explicit(&gen->writer, 0, memory_order_relaxed);
    heimdall_mutex_unlock(&writer_lock);
}
