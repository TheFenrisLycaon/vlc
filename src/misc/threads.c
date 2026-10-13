/*****************************************************************************
 * threads.c: LibHeimdall generic thread support
 *****************************************************************************
 * Copyright (C) 2009-2016 Rémi Denis-Courmont
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
#include <errno.h>
#include <limits.h>
#include <stdatomic.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_atomic.h>
#include "../libheimdall.h"
#include "threads.h"

/* <stdatomic.h> types cannot be used in the C++ view of <heimdall_threads.h> */
struct heimdall_suuint { union { unsigned int value; }; };

static_assert (sizeof (atomic_uint) <= sizeof (struct heimdall_suuint),
               "Size mismatch");
static_assert (alignof (atomic_uint) <= alignof (struct heimdall_suuint),
               "Alignment mismatch");

/*** Global locks ***/

void heimdall_global_mutex (unsigned n, bool acquire)
{
    static heimdall_mutex_t locks[] = {
        HEIMDALL_STATIC_MUTEX,
        HEIMDALL_STATIC_MUTEX,
        HEIMDALL_STATIC_MUTEX,
        HEIMDALL_STATIC_MUTEX,
#ifdef _WIN32
        HEIMDALL_STATIC_MUTEX, // For MTA holder
#endif
    };
    static_assert (HEIMDALL_MAX_MUTEX == ARRAY_SIZE(locks),
                   "Wrong number of global mutexes");
    assert (n < ARRAY_SIZE(locks));

    heimdall_mutex_t *lock = locks + n;
    if (acquire)
        heimdall_mutex_lock (lock);
    else
        heimdall_mutex_unlock (lock);
}

static void heimdall_mutex_init_common(heimdall_mutex_t *mtx, bool recursive)
{
    atomic_init(&mtx->value, 0);
    atomic_init(&mtx->recursion, recursive ? 1 : 0);
    atomic_init(&mtx->owner, 0);
}

void heimdall_mutex_init(heimdall_mutex_t *mtx)
{
    heimdall_mutex_init_common(mtx, false);
}

void heimdall_mutex_init_recursive(heimdall_mutex_t *mtx)
{
    heimdall_mutex_init_common(mtx, true);
}

bool heimdall_mutex_held(const heimdall_mutex_t *mtx)
{
    /* This comparison is thread-safe:
     * Even though other threads may modify the owner field at any time,
     * they will never make it compare equal to the calling thread.
     */
    return heimdall_thread_id() == atomic_load_explicit(&mtx->owner,
                                                   memory_order_relaxed);
}

void heimdall_mutex_lock(heimdall_mutex_t *mtx)
{
    /* This is the Drepper (non-recursive) mutex algorithm
     * from his "Futexes are tricky" paper. The mutex value can be:
     * - 0: the mutex is free
     * - 1: the mutex is locked and uncontended
     * - 2: the mutex is contended (i.e., unlock needs to wake up a waiter)
     */
    if (heimdall_mutex_trylock(mtx) == 0)
        return;

    int canc = heimdall_savecancel(); /* locking is never a cancellation point */

    while (atomic_exchange_explicit(&mtx->value, 2, memory_order_acquire))
        heimdall_atomic_wait(&mtx->value, 2);

    heimdall_restorecancel(canc);
    atomic_store_explicit(&mtx->owner, heimdall_thread_id(), memory_order_relaxed);
}

int heimdall_mutex_trylock(heimdall_mutex_t *mtx)
{
    /* Check the recursion counter:
     * - 0: mutex is not recursive.
     * - 1: mutex is recursive but free or locked non-recursively.
     * - n > 1: mutex is recursive and locked n time(s).
     */
    unsigned recursion = atomic_load_explicit(&mtx->recursion,
                                              memory_order_relaxed);
    if (unlikely(recursion)) {
        if (heimdall_mutex_held(mtx)) {
            /* This thread already owns the mutex, locks recursively.
             * Other threads shall not have modified the recursion or owner fields.
             */
            atomic_store_explicit(&mtx->recursion, recursion + 1,
                                  memory_order_relaxed);
            return 0;
        }
    } else
        assert(!heimdall_mutex_held(mtx));

    unsigned value = 0;

    if (atomic_compare_exchange_strong_explicit(&mtx->value, &value, 1,
                                                memory_order_acquire,
                                                memory_order_relaxed)) {
        atomic_store_explicit(&mtx->owner, heimdall_thread_id(),
                              memory_order_relaxed);
        return 0;
    }

    return EBUSY;
}

void heimdall_mutex_unlock(heimdall_mutex_t *mtx)
{
    assert(heimdall_mutex_held(mtx));

    unsigned recursion = atomic_load_explicit(&mtx->recursion,
                                              memory_order_relaxed);
    if (unlikely(recursion > 1)) {
        /* Non-last recursive unlocking. */
        atomic_store_explicit(&mtx->recursion, recursion - 1,
                              memory_order_relaxed);
        return;
    }

    atomic_store_explicit(&mtx->owner, 0, memory_order_relaxed);

    switch (atomic_exchange_explicit(&mtx->value, 0, memory_order_release)) {
        case 2:
            heimdall_atomic_notify_one(&mtx->value);
        case 1:
            break;
        default:
            heimdall_assert_unreachable();
    }
}

void heimdall_cond_init(heimdall_cond_t *cond)
{
    cond->head = NULL;
    heimdall_mutex_init(&cond->lock);
}

struct heimdall_cond_waiter {
    struct heimdall_cond_waiter **pprev, *next;
    atomic_uint value;
};

static void heimdall_cond_signal_waiter(struct heimdall_cond_waiter *waiter)
{
    waiter->pprev = &waiter->next;
    waiter->next = NULL;
    atomic_fetch_add_explicit(&waiter->value, 1, memory_order_relaxed);
    heimdall_atomic_notify_one(&waiter->value);
}

void heimdall_cond_signal(heimdall_cond_t *cond)
{
    struct heimdall_cond_waiter *waiter;

    /* Some call sites signal their condition variable without holding the
     * corresponding lock. Thus an extra lock is needed here to ensure the
     * consistency of the linked list and the lifetime of its elements.
     * If all call sites locked cleanly, the inner lock would be unnecessary.
     */
    heimdall_mutex_lock(&cond->lock);
    waiter = cond->head;

    if (waiter != NULL) {
        struct heimdall_cond_waiter *next = waiter->next;
        struct heimdall_cond_waiter **pprev = waiter->pprev;

        *pprev = next;

        if (next != NULL)
            next->pprev = pprev;

        heimdall_cond_signal_waiter(waiter);
    }

    heimdall_mutex_unlock(&cond->lock);
}

void heimdall_cond_broadcast(heimdall_cond_t *cond)
{
    struct heimdall_cond_waiter *waiter;

    heimdall_mutex_lock(&cond->lock);
    waiter = cond->head;
    cond->head = NULL;

    /* Keep the lock here so that waiters don't go out of scope */
    while (waiter != NULL) {
        struct heimdall_cond_waiter *next = waiter->next;

        heimdall_cond_signal_waiter(waiter);
        waiter = next;
    }

    heimdall_mutex_unlock(&cond->lock);
}

static void heimdall_cond_wait_prepare(struct heimdall_cond_waiter *waiter,
                                  heimdall_cond_t *cond, heimdall_mutex_t *mutex)
{
    struct heimdall_cond_waiter *next;

    waiter->pprev = &cond->head;
    atomic_init(&waiter->value, 0);

    heimdall_mutex_lock(&cond->lock);
    next = cond->head;
    cond->head = waiter;
    waiter->next = next;

    if (next != NULL)
        next->pprev = &waiter->next;

    heimdall_mutex_unlock(&cond->lock);
    heimdall_mutex_unlock(mutex);
}

static void heimdall_cond_wait_finish(struct heimdall_cond_waiter *waiter,
                                 heimdall_cond_t *cond, heimdall_mutex_t *mutex)
{
    struct heimdall_cond_waiter *next;

    /* If this waiter is still on the linked list, remove it before it goes
     * out of scope. Otherwise, this is a no-op.
     */
    heimdall_mutex_lock(&cond->lock);
    next = waiter->next;
    *(waiter->pprev) = next;

    if (next != NULL)
        next->pprev = waiter->pprev;

    heimdall_mutex_unlock(&cond->lock);

    /* Lock the caller's mutex as required by condition variable semantics. */
    heimdall_mutex_lock(mutex);
}

void heimdall_cond_wait(heimdall_cond_t *cond, heimdall_mutex_t *mutex)
{
    struct heimdall_cond_waiter waiter;

    // wait on a multiply locked mutex not supported
    assert(atomic_load_explicit(&mutex->recursion, memory_order_relaxed) <= 1);

    heimdall_cond_wait_prepare(&waiter, cond, mutex);
    heimdall_atomic_wait(&waiter.value, 0);
    heimdall_cond_wait_finish(&waiter, cond, mutex);
}

int heimdall_cond_timedwait(heimdall_cond_t *cond, heimdall_mutex_t *mutex,
                       heimdall_tick_t deadline)
{
    struct heimdall_cond_waiter waiter;
    int ret;

    // wait on a multiply locked mutex not supported
    assert(atomic_load_explicit(&mutex->recursion, memory_order_relaxed) <= 1);

    heimdall_cond_wait_prepare(&waiter, cond, mutex);
    ret = heimdall_atomic_timedwait(&waiter.value, 0, deadline);
    heimdall_cond_wait_finish(&waiter, cond, mutex);

    return ret;
}

/*** Generic semaphores ***/

void heimdall_sem_init (heimdall_sem_t *sem, unsigned value)
{
    atomic_init(&sem->value, value);
}

int heimdall_sem_post (heimdall_sem_t *sem)
{
    unsigned exp = atomic_load_explicit(&sem->value, memory_order_relaxed);

    do
    {
        if (unlikely(exp == UINT_MAX))
           return EOVERFLOW;
    } while (!atomic_compare_exchange_weak_explicit(&sem->value, &exp, exp + 1,
                                                    memory_order_release,
                                                    memory_order_relaxed));

    heimdall_atomic_notify_one(&sem->value);
    return 0;
}

void heimdall_sem_wait (heimdall_sem_t *sem)
{
    unsigned exp = 1;

    while (!atomic_compare_exchange_weak_explicit(&sem->value, &exp, exp - 1,
                                                  memory_order_acquire,
                                                  memory_order_relaxed))
    {
        if (likely(exp == 0))
        {
            heimdall_atomic_wait(&sem->value, 0);
            exp = 1;
        }
    }
}

int heimdall_sem_timedwait(heimdall_sem_t *sem, heimdall_tick_t deadline)
{
    unsigned exp = 1;

    while (!atomic_compare_exchange_weak_explicit(&sem->value, &exp, exp - 1,
                                                  memory_order_acquire,
                                                  memory_order_relaxed))
    {
        if (likely(exp == 0))
        {
            int ret = heimdall_atomic_timedwait(&sem->value, 0, deadline);
            if (ret)
                return ret;

            exp = 1;
        }
    }

    return 0;
}

int heimdall_sem_trywait(heimdall_sem_t *sem)
{
    unsigned exp = atomic_load_explicit(&sem->value, memory_order_relaxed);

    do
        if (exp == 0)
            return EAGAIN;
    while (!atomic_compare_exchange_weak_explicit(&sem->value, &exp, exp - 1,
                                                  memory_order_acquire,
                                                  memory_order_relaxed));

    return 0;
}

enum { HEIMDALL_LATCH_READY, HEIMDALL_LATCH_CONTEND, HEIMDALL_LATCH_PENDING };

void heimdall_latch_init(heimdall_latch_t *latch, size_t value)
{
    atomic_init(&latch->ready, value ? HEIMDALL_LATCH_PENDING : HEIMDALL_LATCH_READY);
    atomic_init(&latch->value, value);
}

static bool heimdall_latch_count_down_ready(heimdall_latch_t *latch, size_t n)
{
    size_t value;

    value = atomic_fetch_sub_explicit(&latch->value, n, memory_order_acq_rel);
    assert(value >= n);

    if (value != n)
        return false;

    if (atomic_exchange_explicit(&latch->ready, HEIMDALL_LATCH_READY,
                                 memory_order_release) == HEIMDALL_LATCH_CONTEND)
        heimdall_atomic_notify_all(&latch->ready);

    return true;
}

void heimdall_latch_count_down(heimdall_latch_t *latch, size_t n)
{
    (void) heimdall_latch_count_down_ready(latch, n);
}

void heimdall_latch_count_down_and_wait(heimdall_latch_t *latch, size_t n)
{
    if (!heimdall_latch_count_down_ready(latch, n))
        heimdall_latch_wait(latch);
}

bool heimdall_latch_is_ready(const heimdall_latch_t *latch)
{
    return atomic_load_explicit(&latch->ready,
                                memory_order_acquire) == HEIMDALL_LATCH_READY;
}

void heimdall_latch_wait(heimdall_latch_t *latch)
{
    unsigned int expected = HEIMDALL_LATCH_PENDING;

    if (!atomic_compare_exchange_strong_explicit(&latch->ready, &expected,
                                                 HEIMDALL_LATCH_CONTEND,
                                                 memory_order_acquire,
                                                 memory_order_acquire)) {
        if (expected == HEIMDALL_LATCH_READY)
            return;

        assert(expected == HEIMDALL_LATCH_CONTEND);
    }

    do
        heimdall_atomic_wait(&latch->ready, HEIMDALL_LATCH_CONTEND);
    while (!heimdall_latch_is_ready(latch));
}

void heimdall_queuedmutex_init(heimdall_queuedmutex_t *m)
{
    atomic_init(&m->head, 0);
    atomic_init(&m->tail, 0);
    atomic_init(&m->owner, 0);
}

bool heimdall_queuedmutex_held(heimdall_queuedmutex_t *m)
{
    return (heimdall_thread_id() == atomic_load_explicit(&m->owner, memory_order_relaxed));
}

void heimdall_queuedmutex_lock(heimdall_queuedmutex_t *m)
{
    uint_fast32_t ticket = atomic_fetch_add_explicit(&m->tail, 1,
                                                     memory_order_relaxed);
    uint_fast32_t head;

    while ((head = atomic_load_explicit(&m->head,
                                        memory_order_acquire)) != ticket)
        heimdall_atomic_wait(&m->head, head);

    atomic_store_explicit(&m->owner, heimdall_thread_id(), memory_order_relaxed);
}

void heimdall_queuedmutex_unlock(heimdall_queuedmutex_t *m)
{
    atomic_store_explicit(&m->owner, 0, memory_order_relaxed);
    atomic_fetch_add_explicit(&m->head, 1, memory_order_release);
    heimdall_atomic_notify_all(&m->head);
}

enum { HEIMDALL_ONCE_UNDONE, HEIMDALL_ONCE_DOING, HEIMDALL_ONCE_CONTEND, HEIMDALL_ONCE_DONE };

static_assert (HEIMDALL_ONCE_DONE == 3, "Check heimdall_once in header file");

bool (heimdall_once_begin)(heimdall_once_t *restrict once)
{
    unsigned int value = HEIMDALL_ONCE_UNDONE;

    if (atomic_compare_exchange_strong_explicit(&once->value, &value,
                                                HEIMDALL_ONCE_DOING,
                                                memory_order_acquire,
                                                memory_order_acquire))
        return false;

    assert(value >= HEIMDALL_ONCE_DOING);

    if (unlikely(value == HEIMDALL_ONCE_DOING)
     && atomic_compare_exchange_strong_explicit(&once->value, &value,
                                                HEIMDALL_ONCE_CONTEND,
                                                memory_order_acquire,
                                                memory_order_acquire))
        value = HEIMDALL_ONCE_CONTEND;

    assert(value >= HEIMDALL_ONCE_CONTEND);

    while (unlikely(value != HEIMDALL_ONCE_DONE)) {
        heimdall_atomic_wait(&once->value, HEIMDALL_ONCE_CONTEND);
        value = atomic_load_explicit(&once->value, memory_order_acquire);
    }

    return true;
}

void heimdall_once_complete(heimdall_once_t *restrict once)
{
    switch (atomic_exchange_explicit(&once->value, HEIMDALL_ONCE_DONE,
                                     memory_order_release)) {
        case HEIMDALL_ONCE_DOING: /* No waiters, nothing (else) to do */
            break;
        case HEIMDALL_ONCE_CONTEND: /* Notify waiters */
            heimdall_atomic_notify_all(&once->value);
            break;
        default:
            heimdall_assert_unreachable();
    }
}
