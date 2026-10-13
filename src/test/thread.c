/*****************************************************************************
 * thread.c: Test for thread API
 *****************************************************************************
 * Copyright (C) 2020 Marvin Scholz
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
#include <errno.h>
#undef NDEBUG
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>

static int thread_data_magic = 0x1234;
static int thread_return_magic = 0x4321;

// Join thread and verify return
#define TEST_THREAD_JOIN(th, eret)  \
    do {                            \
        void *r;                    \
        heimdall_join(th, &r);           \
        assert(r == eret);          \
    } while (0)

// Cancels thread, joins and verifies return
#define TEST_THREAD_CANCEL_JOIN(th)                 \
    do {                                            \
        heimdall_cancel(th);                             \
        TEST_THREAD_JOIN(th, HEIMDALL_THREAD_CANCELED);  \
    } while (0)

// Join thread and verify magic return value
#define TEST_THREAD_JOIN_MAGIC(th) \
    TEST_THREAD_JOIN(th, &thread_return_magic)

// Spawn thread
#define TEST_THREAD_CLONE(th, f, data) \
    assert(heimdall_clone(th, f, data) == 0)

// Spawn thread with magic data
#define TEST_THREAD_CLONE_MAGIC(th, f) \
    TEST_THREAD_CLONE(th, f, &thread_data_magic)


// Structure for the condition tests
struct cond_data
{
    int state; // Count of threads starting, or -1 when threads may exit
    heimdall_mutex_t mutex;
    heimdall_cond_t  cond_started;  // Signalled when a thread started
    heimdall_cond_t  cond_end;      // Waited on by thread before terminating
};

static void cond_data_init(struct cond_data *data, int count)
{
    assert(count > 0);

    data->state = count;
    heimdall_mutex_init(&data->mutex);
    heimdall_cond_init(&data->cond_started);
    heimdall_cond_init(&data->cond_end);
}


/*
 * Test simple thread creation
 */
static void *thread_func_noop(void *data)
{
    assert(data == &thread_data_magic);
    return &thread_return_magic;
}

static void test__thread_create(void)
{
    heimdall_thread_t th;
    TEST_THREAD_CLONE_MAGIC(&th, thread_func_noop);
    TEST_THREAD_JOIN_MAGIC(th);
}


/*
 * Test simple thread cancellation
 */
static void *thread_func_loop(void *data)
{
    assert(data == &thread_data_magic);
    while(1) {
        sleep(10);
    }
    return &thread_return_magic;
}

static void test__thread_cancelation(void)
{
    heimdall_thread_t th;
    TEST_THREAD_CLONE_MAGIC(&th, thread_func_loop);
    TEST_THREAD_CANCEL_JOIN(th);
}


/*
 * Test thread cancellation with cleanup
 */
static void thread_func_cleanup(void *data)
{
    int *cleanup_state = data;
    assert(*cleanup_state == 0);
    *cleanup_state = 1;
}

static void *thread_func_loop_cleanup(void *data)
{
    int *cleanup_state = data;
    assert(*cleanup_state == 0);

    heimdall_cleanup_push(thread_func_cleanup, data);
    while(1) {
        heimdall_testcancel();
    }
    heimdall_cleanup_pop();

    return &thread_return_magic;
}

static void test__thread_cancelation_with_cleanup(void)
{
    heimdall_thread_t th;
    int th_cleanup_state = 0;

    TEST_THREAD_CLONE(&th, thread_func_loop_cleanup, &th_cleanup_state);
    TEST_THREAD_CANCEL_JOIN(th);
    assert(th_cleanup_state == 1);
}


/*
 * - Test broadcasting a condition, waking several threads
 * - Test waiting on condition
 */
static void *thread_func_cond(void *ptr)
{
    struct cond_data *data = ptr;

    // Decrement state and signal start of thread
    heimdall_mutex_lock(&data->mutex);
    data->state -= 1;
    heimdall_cond_signal(&data->cond_started);
    heimdall_mutex_unlock(&data->mutex);

    // Wait until termination is allowed
    heimdall_mutex_lock(&data->mutex);
    while (data->state != -1)
        heimdall_cond_wait(&data->cond_end, &data->mutex);
    heimdall_mutex_unlock(&data->mutex);

    return &thread_return_magic;
}

static void test__cond_broadcast(void)
{
    struct cond_data data;
    heimdall_thread_t threads[20];

    cond_data_init(&data, ARRAY_SIZE(threads));

    // Spawn all threads
    for (size_t i = 0; i < ARRAY_SIZE(threads); ++i) {
        TEST_THREAD_CLONE(&threads[i], thread_func_cond, &data);
    }

    // Wait for all threads to start
    heimdall_mutex_lock(&data.mutex);
    while(data.state > 0) {
        heimdall_cond_wait(&data.cond_started, &data.mutex);
    }
    heimdall_mutex_unlock(&data.mutex);

    // Broadcast threads-may-end condition to all threads
    heimdall_mutex_lock(&data.mutex);
    data.state = -1;
    heimdall_cond_broadcast(&data.cond_end);
    heimdall_mutex_unlock(&data.mutex);

    // Wait for all threads to exit
    for (size_t i = 0; i < ARRAY_SIZE(threads); ++i) {
        TEST_THREAD_JOIN_MAGIC(threads[i]);
    }
}

static void test__cond_wait(void)
{
    struct cond_data data;
    cond_data_init(&data, 1);

    heimdall_thread_t th;
    TEST_THREAD_CLONE(&th, thread_func_cond, &data);

    // Wait for thread to start
    heimdall_mutex_lock(&data.mutex);
    while(data.state != 0) {
        heimdall_cond_wait(&data.cond_started, &data.mutex);
    }
    heimdall_mutex_unlock(&data.mutex);

    // Signal threads-may-end condition to thread
    heimdall_mutex_lock(&data.mutex);
    data.state = -1;
    heimdall_cond_signal(&data.cond_end);
    heimdall_mutex_unlock(&data.mutex);

    // Wait for thread to exit
    TEST_THREAD_JOIN_MAGIC(th);
}


/*
 * Test waiting on condition and let it timeout
 */
static void *thread_func_cond_timeout(void *ptr)
{
    struct cond_data *data = ptr;

    // Decrement state and signal start of thread
    heimdall_mutex_lock(&data->mutex);
    data->state -= 1;
    heimdall_cond_signal(&data->cond_started);
    heimdall_mutex_unlock(&data->mutex);

    heimdall_mutex_lock(&data->mutex);

    // Wait until termination is allowed (which is never signalled)
    heimdall_tick_t now = heimdall_tick_now();
    heimdall_tick_t deadline = now + HEIMDALL_TICK_FROM_MS(25);
    int ret = heimdall_cond_timedwait(&data->cond_end, &data->mutex, deadline);
    assert(ret == ETIMEDOUT);

    heimdall_tick_t elapsed = heimdall_tick_now() - now;
    assert(elapsed >= HEIMDALL_TICK_FROM_MS(25));

    heimdall_mutex_unlock(&data->mutex);

    return &thread_return_magic;
}

static void test__cond_wait_timeout(void)
{
    struct cond_data data;
    cond_data_init(&data, 1);

    heimdall_thread_t th;
    TEST_THREAD_CLONE(&th, thread_func_cond_timeout, &data);

    // Wait for thread to start
    heimdall_mutex_lock(&data.mutex);
    while(data.state != 0) {
        heimdall_cond_wait(&data.cond_started, &data.mutex);
    }
    heimdall_mutex_unlock(&data.mutex);

    // Never signal that the thread may end, on purpose
    // as it should timeout.

    // Wait for thread exit
    TEST_THREAD_JOIN_MAGIC(th);
}

/*
 * Test latch synchronisation
 */
static void *thread_latch(void *data)
{
    heimdall_latch_t *l = data;

    assert(!heimdall_latch_is_ready(l));
    heimdall_latch_count_down(l, 1);
    heimdall_latch_wait(l);
    assert(heimdall_latch_is_ready(l));
    return NULL;
}

static void test__latch(void)
{
    heimdall_latch_t l;
    heimdall_thread_t th;

    heimdall_latch_init(&l, 0);
    assert(heimdall_latch_is_ready(&l));
    heimdall_latch_wait(&l);
    assert(heimdall_latch_is_ready(&l));

    heimdall_latch_init(&l, 5);
    assert(!heimdall_latch_is_ready(&l));
    heimdall_latch_count_down(&l, 3);
    assert(!heimdall_latch_is_ready(&l));
    heimdall_latch_count_down_and_wait(&l, 2);
    assert(heimdall_latch_is_ready(&l));
    heimdall_latch_wait(&l);
    assert(heimdall_latch_is_ready(&l));

    heimdall_latch_init(&l, 2);
    TEST_THREAD_CLONE(&th, thread_latch, &l);
    heimdall_latch_count_down_and_wait(&l, 1);
    TEST_THREAD_JOIN(th, NULL);
}

/*
 * Test cancelling heimdall_tick_sleep
 */
static void *thread_func_tick_sleep(void *data)
{
    assert(data == &thread_data_magic);
    (heimdall_tick_sleep)(HEIMDALL_TICK_FROM_SEC(10));
    return &thread_return_magic;
}

static void test__heimdall_tick_sleep_cancelation(void)
{
    heimdall_thread_t th;
    TEST_THREAD_CLONE_MAGIC(&th, thread_func_tick_sleep);
    TEST_THREAD_CANCEL_JOIN(th);
}


/*
 * Test sleeping for delay with heimdall_tick_sleep
 */
static void test__heimdall_tick_sleep(void)
{
    heimdall_tick_t now = heimdall_tick_now();

    (heimdall_tick_sleep)(HEIMDALL_TICK_FROM_MS(25));

    heimdall_tick_t elapsed = heimdall_tick_now() - now;
    assert(elapsed >= HEIMDALL_TICK_FROM_MS(25));
}


/*
 * Main function executing all tests above
 */
int main(void)
{
    alarm(4);

    test__thread_create();
    test__thread_cancelation();
    test__thread_cancelation_with_cleanup();
    test__cond_wait();
    test__cond_wait_timeout();
    test__cond_broadcast();
    test__latch();
    test__heimdall_tick_sleep_cancelation();
    test__heimdall_tick_sleep();

    return 0;
}
