/*****************************************************************************
 * interrupt.c:
 *****************************************************************************
 * Copyright (C) 2015 Rémi Denis-Courmont
 * Copyright (C) 2023 Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Rémi Denis-Courmont
 *          Alexandre Janniaux <ajanni@videolabs.io>
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

/** @ingroup interrupt */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_EVENTFD_H
# include <sys/eventfd.h>
#endif

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_poll.h>
#include <heimdall_fs.h> /* heimdall_pipe */
#include <heimdall_network.h> /* heimdall_accept */

#include "interrupt.h"
#include "../libheimdall.h"

static thread_local heimdall_interrupt_t *heimdall_interrupt_var;

/**
 * Initializes an interruption context.
 */
void heimdall_interrupt_init(heimdall_interrupt_t *ctx)
{
    heimdall_mutex_init(&ctx->lock);
    ctx->interrupted = false;
    atomic_init(&ctx->killed, false);
    ctx->callback = NULL;
}

heimdall_interrupt_t *heimdall_interrupt_create(void)
{
    heimdall_interrupt_t *ctx = malloc(sizeof (*ctx));
    if (likely(ctx != NULL))
        heimdall_interrupt_init(ctx);
    return ctx;
}

/**
 * Deinitializes an interruption context.
 * The context shall no longer be used by any thread.
 */
void heimdall_interrupt_deinit(heimdall_interrupt_t *ctx)
{
    assert(ctx->callback == NULL);
}

void heimdall_interrupt_destroy(heimdall_interrupt_t *ctx)
{
    assert(ctx != NULL);
    heimdall_interrupt_deinit(ctx);
    free(ctx);
}

void heimdall_interrupt_raise(heimdall_interrupt_t *ctx)
{
    assert(ctx != NULL);

    /* This function must be reentrant. But the callback typically is not
     * reentrant. The lock ensures that all calls to the callback for a given
     * context are serialized. The lock also protects against invalid memory
     * accesses to the callback pointer proper, and the interrupted flag. */
    heimdall_mutex_lock(&ctx->lock);
    ctx->interrupted = true;
    if (ctx->callback != NULL)
        ctx->callback(ctx->data);
    heimdall_mutex_unlock(&ctx->lock);
}

heimdall_interrupt_t *heimdall_interrupt_set(heimdall_interrupt_t *newctx)
{
    heimdall_interrupt_t *oldctx = heimdall_interrupt_var;

    heimdall_interrupt_var = newctx;
    return oldctx;
}

/**
 * Prepares to enter interruptible wait.
 *
 * @param ctx an unused interruption context to prepare
 * @param cb callback to interrupt the wait (i.e. wake up the thread)
 * @param data opaque data pointer for the callback
 *
 * @note Any <b>successful</b> call <b>must</b> be paired with a call to
 * heimdall_interrupt_finish().
 */
static void heimdall_interrupt_prepare(heimdall_interrupt_t *ctx,
                                  void (*cb)(void *), void *data)
{
    assert(ctx != NULL);
    assert(ctx == heimdall_interrupt_var);

    heimdall_mutex_lock(&ctx->lock);
    assert(ctx->callback == NULL);
    ctx->callback = cb;
    ctx->data = data;

    if (unlikely(ctx->interrupted))
        cb(data);
    heimdall_mutex_unlock(&ctx->lock);
}

/**
 * Cleans up after an interruptible wait: waits for any pending invocations of
 * the callback previously registered with heimdall_interrupt_prepare(), and rechecks
 * for any pending interruption.
 *
 * @warning As this function waits for ongoing callback invocation to complete,
 * the caller must not hold any resource necessary for the callback to run.
 * Otherwise a deadlock may occur.
 *
 * @return EINTR if an interruption occurred, zero otherwise
 */
static int heimdall_interrupt_finish(heimdall_interrupt_t *ctx)
{
    int ret = 0;

    assert(ctx != NULL);
    assert(ctx == heimdall_interrupt_var);

    /* Wait for pending callbacks to prevent access by other threads. */
    heimdall_mutex_lock(&ctx->lock);
    ctx->callback = NULL;
    if (ctx->interrupted)
    {
        ret = EINTR;
        ctx->interrupted = false;
    }
    heimdall_mutex_unlock(&ctx->lock);
    return ret;
}

void heimdall_interrupt_register(void (*cb)(void *), void *opaque)
{
    heimdall_interrupt_t *ctx = heimdall_interrupt_var;
    if (ctx != NULL)
        heimdall_interrupt_prepare(ctx, cb, opaque);
}

int heimdall_interrupt_unregister(void)
{
    heimdall_interrupt_t *ctx = heimdall_interrupt_var;
    return (ctx != NULL) ? heimdall_interrupt_finish(ctx) : 0;
}

static void heimdall_interrupt_cleanup(void *opaque)
{
    heimdall_interrupt_finish(opaque);
}

void heimdall_interrupt_kill(heimdall_interrupt_t *ctx)
{
    assert(ctx != NULL);

    atomic_store(&ctx->killed, true);
    heimdall_interrupt_raise(ctx);
}

bool heimdall_killed(void)
{
    heimdall_interrupt_t *ctx = heimdall_interrupt_var;

    return (ctx != NULL) && atomic_load(&ctx->killed);
}

static void heimdall_interrupt_sem(void *opaque)
{
    heimdall_sem_post(opaque);
}

int heimdall_sem_wait_i11e(heimdall_sem_t *sem)
{
    heimdall_interrupt_t *ctx = heimdall_interrupt_var;
    if (ctx == NULL)
        return heimdall_sem_wait(sem), 0;

    heimdall_interrupt_prepare(ctx, heimdall_interrupt_sem, sem);

    heimdall_cleanup_push(heimdall_interrupt_cleanup, ctx);
    heimdall_sem_wait(sem);
    heimdall_cleanup_pop();

    return heimdall_interrupt_finish(ctx);
}

static void heimdall_mwait_i11e_wake(void *opaque)
{
    heimdall_cond_signal(opaque);
}

static void heimdall_mwait_i11e_cleanup(void *opaque)
{
    heimdall_interrupt_t *ctx = opaque;

    heimdall_mutex_unlock(&ctx->lock);
    heimdall_interrupt_finish(ctx);
}

int heimdall_mwait_i11e(heimdall_tick_t deadline)
{
    heimdall_interrupt_t *ctx = heimdall_interrupt_var;
    if (ctx == NULL)
        return heimdall_tick_wait(deadline), 0;

    heimdall_cond_t wait;
    heimdall_cond_init(&wait);

    heimdall_interrupt_prepare(ctx, heimdall_mwait_i11e_wake, &wait);

    heimdall_mutex_lock(&ctx->lock);
    heimdall_cleanup_push(heimdall_mwait_i11e_cleanup, ctx);
    while (!ctx->interrupted
        && heimdall_cond_timedwait(&wait, &ctx->lock, deadline) == 0);
    heimdall_cleanup_pop();
    heimdall_mutex_unlock(&ctx->lock);

    return heimdall_interrupt_finish(ctx);
}

static void heimdall_interrupt_forward_wake(void *opaque)
{
    void **data = opaque;
    heimdall_interrupt_t *to = data[0];
    heimdall_interrupt_t *from = data[1];

    (atomic_load(&from->killed) ? heimdall_interrupt_kill
                                : heimdall_interrupt_raise)(to);
}

void heimdall_interrupt_forward_start(heimdall_interrupt_t *to, void *data[2])
{
    data[0] = data[1] = NULL;

    heimdall_interrupt_t *from = heimdall_interrupt_var;
    if (from == NULL)
        return;

    assert(from != to);
    data[0] = to;
    data[1] = from;
    heimdall_interrupt_prepare(from, heimdall_interrupt_forward_wake, data);
}

int heimdall_interrupt_forward_stop(void *const data[2])
{
    heimdall_interrupt_t *from = data[1];
    if (from == NULL)
        return 0;

    assert(from->callback == heimdall_interrupt_forward_wake);
    assert(from->data == data);
    return heimdall_interrupt_finish(from);
}

#ifndef _WIN32
# include <fcntl.h>
# include <sys/uio.h>
# ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
# endif

static void heimdall_poll_i11e_wake(void *opaque)
{
    uint64_t value = 1;
    int *fd = opaque;
    int canc;

    canc = heimdall_savecancel();
    while (write(fd[1], &value, sizeof (value)) == -1)
    {
        /* Only EINTR can happen here, so ignore the error.
         * We still need to ensure we really wrote the value to trigger
         * the interruption check though. */
    }
    heimdall_restorecancel(canc);
}

static void heimdall_poll_i11e_cleanup(void *opaque)
{
    heimdall_interrupt_t *ctx = opaque;
    int *fd = ctx->data;

    heimdall_interrupt_finish(ctx);
    if (fd[1] != fd[0])
        heimdall_close(fd[1]);
    heimdall_close(fd[0]);
}

static int heimdall_poll_i11e_inner(struct pollfd *restrict fds, unsigned nfds,
                               int timeout, heimdall_interrupt_t *ctx,
                               struct pollfd *restrict ufd)
{
    int fd[2];
    int ret = -1;
    int canc;

    /* TODO: cache this */
# if defined (HAVE_EVENTFD) && defined (EFD_CLOEXEC)
    canc = heimdall_savecancel();
    fd[0] = eventfd(0, EFD_CLOEXEC);
    heimdall_restorecancel(canc);
    if (fd[0] != -1)
        fd[1] = fd[0];
    else
# endif
    if (heimdall_pipe(fd))
    {
        heimdall_testcancel();
        errno = ENOMEM;
        return -1;
    }

    for (unsigned i = 0; i < nfds; i++)
    {
        ufd[i].fd = fds[i].fd;
        ufd[i].events = fds[i].events;
    }
    ufd[nfds].fd = fd[0];
    ufd[nfds].events = POLLIN;

    heimdall_interrupt_prepare(ctx, heimdall_poll_i11e_wake, fd);

    heimdall_cleanup_push(heimdall_poll_i11e_cleanup, ctx);
    ret = poll(ufd, nfds + 1, timeout);

    for (unsigned i = 0; i < nfds; i++)
        fds[i].revents = ufd[i].revents;

    if (ret > 0 && ufd[nfds].revents)
    {
        uint64_t dummy;

        while (read(fd[0], &dummy, sizeof (dummy)) == -1)
        {
            /* Only EINTR can happen here, so ignore the error.
             * We still need to read() the value though, to ensure
             * that the interruption is correctly acknowledged. */
        }
        ret--;
    }
    heimdall_cleanup_pop();

    if (heimdall_interrupt_finish(ctx))
    {
        errno = EINTR;
        ret = -1;
    }

    canc = heimdall_savecancel();
    if (fd[1] != fd[0])
        heimdall_close(fd[1]);
    heimdall_close(fd[0]);
    heimdall_restorecancel(canc);
    return ret;
}

int heimdall_poll_i11e(struct pollfd *fds, unsigned nfds, int timeout)
{
    heimdall_interrupt_t *ctx = heimdall_interrupt_var;
    if (ctx == NULL)
        return poll(fds, nfds, timeout);

    int ret;

    if (likely(nfds < 255))
    {   /* Fast path with stack allocation */
        struct pollfd ufd[nfds + 1];

        ret = heimdall_poll_i11e_inner(fds, nfds, timeout, ctx, ufd);
    }
    else
    {   /* Slow path but poll() is slow with large nfds anyway. */
        struct pollfd *ufd = heimdall_alloc(nfds + 1, sizeof (*ufd));
        if (unlikely(ufd == NULL))
            return -1; /* ENOMEM */

        heimdall_cleanup_push(free, ufd);
        ret = heimdall_poll_i11e_inner(fds, nfds, timeout, ctx, ufd);
        heimdall_cleanup_pop();
        free(ufd);
    }
    return ret;
}

static int heimdall_poll_file(int fd, unsigned int mask)
{
    struct pollfd ufd;

    ufd.fd = fd;
    ufd.events = mask;
    return heimdall_poll_i11e(&ufd, 1, -1);
}

static int heimdall_poll_sock(int sock, unsigned int mask)
{
    return heimdall_poll_file(sock, mask);
}

#else /* !_WIN32 */
static void CALLBACK heimdall_poll_i11e_wake_self(ULONG_PTR data)
{
    (void) data; /* Nothing to do */
}

static void heimdall_poll_i11e_wake(void *opaque)
{
    HANDLE th = opaque;
    QueueUserAPC(heimdall_poll_i11e_wake_self, th, 0);
}

static void heimdall_poll_i11e_cleanup(void *opaque)
{
    heimdall_interrupt_t *ctx = opaque;
    HANDLE th = ctx->data;

    heimdall_interrupt_finish(ctx);
    CloseHandle(th);
}

int heimdall_poll_i11e(struct pollfd *fds, unsigned nfds, int timeout)
{
    heimdall_interrupt_t *ctx = heimdall_interrupt_var;
    if (ctx == NULL)
        return heimdall_poll(fds, nfds, timeout);

    int ret = -1;
    HANDLE th;

    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                         GetCurrentProcess(), &th, 0, FALSE,
                         DUPLICATE_SAME_ACCESS))
    {
        errno = ENOMEM;
        return -1;
    }

    heimdall_interrupt_prepare(ctx, heimdall_poll_i11e_wake, th);

    heimdall_cleanup_push(heimdall_poll_i11e_cleanup, ctx);
    ret = heimdall_poll(fds, nfds, timeout);
    heimdall_cleanup_pop();

    if (heimdall_interrupt_finish(ctx))
    {
        errno = EINTR;
        ret = -1;
    }

    CloseHandle(th);
    return ret;
}

static int heimdall_poll_file(int fd, unsigned int mask)
{
    (void) fd; (void) mask;
    return 1;
}

static int heimdall_poll_sock(int sock, unsigned int mask)
{
    struct pollfd ufd;

    ufd.fd = sock;
    ufd.events = mask;
    return heimdall_poll_i11e(&ufd, 1, -1);
}
#endif /* _WIN32 */

/* There are currently no ways to atomically force a non-blocking read or write
 * operations. Even for sockets, the MSG_DONTWAIT flag is non-standard.
 *
 * So in the event that more than one thread tries to read or write on the same
 * file at the same time, there is a race condition where these functions might
 * block in spite of an interruption. This should never happen in practice.
 */

/**
 * Wrapper for readv() that returns the EINTR error upon Heimdall I/O interruption.
 * @warning This function ignores the non-blocking file flag.
 */
ssize_t heimdall_readv_i11e(int fd, struct iovec *iov, int count)
{
    if (heimdall_poll_file(fd, POLLIN) < 0)
        return -1;
    return readv(fd, iov, count);
}

/**
 * Wrapper for writev() that returns the EINTR error upon Heimdall I/O interruption.
 *
 * @note Like writev(), once some but not all bytes are written, the function
 * might wait for write completion, regardless of signals and interruptions.
 * @warning This function ignores the non-blocking file flag.
 */
ssize_t heimdall_writev_i11e(int fd, const struct iovec *iov, int count)
{
    if (heimdall_poll_file(fd, POLLOUT) < 0)
        return -1;
    return heimdall_writev(fd, iov, count);
}

/**
 * Wrapper for read() that returns the EINTR error upon Heimdall I/O interruption.
 * @warning This function ignores the non-blocking file flag.
 */
ssize_t heimdall_read_i11e(int fd, void *buf, size_t count)
{
    struct iovec iov = { .iov_base = buf, .iov_len = count };
    return heimdall_readv_i11e(fd, &iov, 1);
}

/**
 * Wrapper for write() that returns the EINTR error upon Heimdall I/O interruption.
 *
 * @note Like write(), once some but not all bytes are written, the function
 * might wait for write completion, regardless of signals and interruptions.
 * @warning This function ignores the non-blocking file flag.
 */
ssize_t heimdall_write_i11e(int fd, const void *buf, size_t count)
{
    struct iovec iov = { .iov_base = (void*)buf, .iov_len = count };
    return heimdall_writev_i11e(fd, &iov, 1);
}

ssize_t heimdall_recvmsg_i11e(int fd, struct msghdr *msg, int flags)
{
    if (heimdall_poll_sock(fd, POLLIN) < 0)
        return -1;
    /* NOTE: MSG_OOB and MSG_PEEK should work fine here.
     * MSG_WAITALL is not supported at this point. */
    return recvmsg(fd, msg, flags);
}

#ifndef _WIN32
ssize_t heimdall_recvfrom_i11e(int fd, void *buf, size_t len, int flags,
                        struct sockaddr *addr, socklen_t *addrlen)
{
    struct iovec iov = { .iov_base = buf, .iov_len = len };
    struct msghdr msg = {
        .msg_name = addr,
        .msg_namelen = (addrlen != NULL) ? *addrlen : 0,
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    ssize_t ret = heimdall_recvmsg_i11e(fd, &msg, flags);
    if (ret >= 0 && addrlen != NULL)
        *addrlen = msg.msg_namelen;
    return ret;
}
#endif

ssize_t heimdall_sendmsg_i11e(int fd, const struct msghdr *msg, int flags)
{
    if (heimdall_poll_sock(fd, POLLOUT) < 0)
        return -1;
    /* NOTE: MSG_EOR and MSG_OOB should all work fine here. */
    return heimdall_sendmsg(fd, msg, flags);
}

#ifndef _WIN32
ssize_t heimdall_sendto_i11e(int fd, const void *buf, size_t len, int flags,
                      const struct sockaddr *addr, socklen_t addrlen)
{
    struct iovec iov = { .iov_base = (void *)buf, .iov_len = len };
    struct msghdr msg = {
        .msg_name = (struct sockaddr *)addr,
        .msg_namelen = addrlen,
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    return heimdall_sendmsg_i11e(fd, &msg, flags);
}

#else /* _WIN32 */

ssize_t heimdall_recvfrom_i11e(int fd, void *buf, size_t len, int flags,
                        struct sockaddr *addr, socklen_t *addrlen)
{
    if (heimdall_poll_sock(fd, POLLIN) < 0)
        return -1;

    ssize_t ret = recvfrom(fd, buf, len, flags, addr, addrlen);
    if (ret < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
        errno = EAGAIN;
    return ret;
}

ssize_t heimdall_sendto_i11e(int fd, const void *buf, size_t len, int flags,
                      const struct sockaddr *addr, socklen_t addrlen)
{
    if (heimdall_poll_sock(fd, POLLOUT) < 0)
        return -1;

    ssize_t ret = sendto(fd, buf, len, flags, addr, addrlen);
    if (ret < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
        errno = EAGAIN;
    return ret;
}

#endif

int heimdall_accept_i11e(int fd, struct sockaddr *addr, socklen_t *addrlen,
                  bool nonblock)
{
    if (heimdall_poll_sock(fd, POLLIN) < 0)
        return -1;
    return heimdall_accept(fd, addr, addrlen, nonblock);
}
