/*****************************************************************************
 * interrupt.c: Test for interrupt context API
 *****************************************************************************
 * Copyright (C) 2015 Rémi Denis-Courmont
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

#undef NDEBUG
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_interrupt.h>
#include <heimdall_network.h>

const char heimdall_module_name[] = "test_interrupt";

static heimdall_sem_t sem;
static int fds[2];

static void interrupt_callback(void *data)
{
    heimdall_sem_post(data);
}

static void test_context_simple(heimdall_interrupt_t *ctx)
{
    heimdall_interrupt_t *octx;
    char c;

    heimdall_interrupt_set(ctx);
    octx = heimdall_interrupt_set(NULL);
    assert(octx == ctx);
    octx = heimdall_interrupt_set(ctx);
    assert(octx == NULL);
    octx = heimdall_interrupt_set(ctx);
    assert(octx == ctx);

    heimdall_interrupt_register(interrupt_callback, &sem);
    heimdall_interrupt_raise(ctx);
    heimdall_sem_wait(&sem);
    heimdall_interrupt_unregister();

    /* BIG FAT WARNING: This is only meant to test the heimdall_cond_wait_i11e()
     * function. This is NOT a good example of how to use the function in
     * normal code. */
    heimdall_sem_post(&sem);
    assert(heimdall_sem_wait_i11e(&sem) == 0);

    heimdall_interrupt_raise(ctx);
    assert(heimdall_sem_wait_i11e(&sem) == EINTR);

    heimdall_sem_post(&sem);
    heimdall_interrupt_raise(ctx);
    assert(heimdall_sem_wait_i11e(&sem) == EINTR);
    assert(heimdall_sem_wait_i11e(&sem) == 0);

    heimdall_interrupt_raise(ctx);
    heimdall_sem_post(&sem);
    assert(heimdall_sem_wait_i11e(&sem) == EINTR);
    assert(heimdall_sem_wait_i11e(&sem) == 0);

    assert(heimdall_mwait_i11e(1) == 0);
    heimdall_interrupt_raise(ctx);
    assert(heimdall_mwait_i11e(CLOCK_FREQ * 10000000) == EINTR);

    assert(heimdall_poll_i11e(NULL, 0, 1) == 0);
    heimdall_interrupt_raise(ctx);
    assert(heimdall_poll_i11e(NULL, 0, 1000000000) == -1);
    assert(errno == EINTR);

    c = 12;
    assert(heimdall_write_i11e(fds[0], &c, 1) == 1);
    c = 0;
    assert(heimdall_read_i11e(fds[1], &c, 1) == 1 && c == 12);
    heimdall_interrupt_raise(ctx);
    assert(heimdall_read_i11e(fds[1], &c, 1) == -1);
    assert(errno == EINTR);

    c = 42;
    assert(heimdall_sendto_i11e(fds[0], &c, 1, 0, NULL, 0) == 1);
    c = 0;
    assert(heimdall_recvfrom_i11e(fds[1], &c, 1, 0, NULL, 0) == 1 && c == 42);
    heimdall_interrupt_raise(ctx);
    assert(heimdall_recvfrom_i11e(fds[1], &c, 1, 0, NULL, 0) == -1);
    assert(errno == EINTR);

    heimdall_interrupt_raise(ctx);
    assert(heimdall_accept_i11e(fds[1], NULL, NULL, true) < 0);

    octx = heimdall_interrupt_set(NULL);
    assert(octx == ctx);
    octx = heimdall_interrupt_set(NULL);
    assert(octx == NULL);
}

static void *test_thread_simple(void *data)
{
    heimdall_interrupt_t *ctx = data;

    heimdall_interrupt_set(ctx);
    assert(heimdall_sem_wait_i11e(&sem) == EINTR);
    assert(heimdall_sem_wait_i11e(&sem) == 0);
    heimdall_sem_wait(&sem);

    test_context_simple(ctx);
    return NULL;
}

static void *test_thread_cleanup(void *data)
{
    heimdall_interrupt_t *ctx = data;

    test_context_simple(ctx);
    /* Test context clearing on exit */
    heimdall_interrupt_set(ctx);
    return NULL;
}

static void *test_thread_cancel(void *data)
{
    heimdall_interrupt_t *ctx = data;

    int canc = heimdall_savecancel();
    test_context_simple(ctx);
    heimdall_restorecancel(canc);

    /* Test context clearing on cancellation */
    heimdall_interrupt_set(ctx);
    for (;;)
        pause();

    heimdall_assert_unreachable();
}

static void unreachable_callback(void *data)
{
    (void) data;
    abort();
}

int main (void)
{
    heimdall_interrupt_t *ctx;
    heimdall_thread_t th;

    alarm(2);

    ctx = heimdall_interrupt_create();
    assert(ctx != NULL);
    heimdall_interrupt_destroy(ctx);

    heimdall_sem_init(&sem, 0);
    ctx = heimdall_interrupt_create();
    assert(ctx != NULL);

    assert(heimdall_socketpair(PF_LOCAL, SOCK_STREAM, 0, fds, false) == 0);

    test_context_simple(ctx);

    assert(!heimdall_clone(&th, test_thread_simple, ctx));
    heimdall_interrupt_raise(ctx);
    heimdall_sem_post(&sem);
    heimdall_sem_post(&sem);
    heimdall_join(th, NULL);

    assert(!heimdall_clone(&th, test_thread_cleanup, ctx));
    heimdall_join(th, NULL);

    assert(!heimdall_clone(&th, test_thread_cancel, ctx));
    heimdall_cancel(th);
    heimdall_join(th, NULL);

    heimdall_interrupt_destroy(ctx);

    /* Tests without interrupt context */
    heimdall_sem_post(&sem);
    assert(heimdall_sem_wait_i11e(&sem) == 0);
    assert(heimdall_mwait_i11e(1) == 0);
    assert(heimdall_poll_i11e(NULL, 0, 1) == 0);

    heimdall_interrupt_register(unreachable_callback, NULL);
    heimdall_interrupt_unregister();

    void *data[2];
    heimdall_interrupt_forward_start(ctx, data);
    assert(heimdall_interrupt_forward_stop(data) == 0);

    heimdall_close(fds[1]);
    heimdall_close(fds[0]);
    return 0;
}
