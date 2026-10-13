/*****************************************************************************
 * heimdall_atomic.h:
 *****************************************************************************
 * Copyright (C) 2010 Rémi Denis-Courmont
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

#ifndef HEIMDALL_ATOMIC_H
# define HEIMDALL_ATOMIC_H

/**
 * \file
 * Atomic operations do not require locking, but they are not very powerful.
 */

# include <assert.h>
#ifndef __cplusplus
# include <stdatomic.h>
#else
# include <atomic>
using std::atomic_uintptr_t;
using std::memory_order_relaxed;
using std::memory_order_acq_rel;
#endif
# include <heimdall_common.h>
# include <heimdall_tick.h>

#define HEIMDALL_STATIC_RC { \
    .refs = (uintptr_t) 1 \
}

typedef struct heimdall_atomic_rc_t {
    atomic_uintptr_t refs;
} heimdall_atomic_rc_t;

/** Init the RC to 1 */
static inline void heimdall_atomic_rc_init(heimdall_atomic_rc_t *rc)
{
#ifndef __cplusplus
    atomic_init(&rc->refs, (uintptr_t)1);
#else
    rc->refs = (uintptr_t)1;
#endif
}

/** Increment the RC */
static inline void heimdall_atomic_rc_inc(heimdall_atomic_rc_t *rc)
{
    uintptr_t prev = atomic_fetch_add_explicit(&rc->refs, (uintptr_t)1,
                                               memory_order_relaxed);
    heimdall_assert(prev);
    HEIMDALL_UNUSED(prev);
}

/** Decrement the RC and return true if it reaches 0 */
static inline bool heimdall_atomic_rc_dec(heimdall_atomic_rc_t *rc)
{
    uintptr_t prev = atomic_fetch_sub_explicit(&rc->refs, (uintptr_t)1,
                                               memory_order_acq_rel);
    heimdall_assert(prev);
    return prev == 1;
}

/** Returns the current reference count.
 *  This is not safe to use for logic and must only be used for debugging or
 *  assertion purposes */
static inline uintptr_t heimdall_atomic_rc_get(const heimdall_atomic_rc_t* rc)
{
    return atomic_load_explicit(&rc->refs, memory_order_relaxed);
}

/**
 * Waits on an address.
 *
 * Puts the calling thread to sleep if a specific unsigned 32-bits value is
 * stored at a specified address. The thread will sleep until it is woken up by
 * a call to heimdall_atomic_notify_one() or heimdall_atomic_notify_all() in another
 * thread, or spuriously.
 *
 * If the value does not match, do nothing and return immediately.
 *
 * \param addr address to check for
 * \param val value to match at the address
 */
HEIMDALL_API void heimdall_atomic_wait(void *addr, unsigned val);

/**
 * Waits on an address with a time-out.
 *
 * This function operates as heimdall_atomic_wait() but provides an additional
 * time-out. If the deadline is reached, the thread resumes and the function
 * returns.
 *
 * \param addr address to check for
 * \param val value to match at the address
 * \param deadline deadline to wait until
 *
 * \retval 0 the function was woken up before the time-out
 * \retval ETIMEDOUT the deadline was reached
 */
HEIMDALL_API
int heimdall_atomic_timedwait(void *addr, unsigned val, heimdall_tick_t deadline);

/**
 * Wakes up one thread on an address.
 *
 * Wakes up (at least) one of the thread sleeping on the specified address.
 * The address must be equal to the first parameter given by at least one
 * thread sleeping within the heimdall_atomic_wait() or heimdall_atomic_timedwait()
 * functions. If no threads are found, this function does nothing.
 *
 * \param addr address identifying which threads may be woken up
 */
HEIMDALL_API void heimdall_atomic_notify_one(void *addr);

/**
 * Wakes up all thread on an address.
 *
 * Wakes up all threads sleeping on the specified address (if any).
 * Any thread sleeping within a call to heimdall_atomic_wait() or
 * heimdall_atomic_timedwait() with the specified address as first call parameter
 * will be woken up.
 *
 * \param addr address identifying which threads to wake up
 */
HEIMDALL_API void heimdall_atomic_notify_all(void *addr);

#endif
