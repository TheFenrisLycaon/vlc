/*****************************************************************************
 * heimdall_interrupt.h:
 *****************************************************************************
 * Copyright (C) 2015 Remlab T:mi
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

#ifndef HEIMDALL_INTERRUPT_H
# define HEIMDALL_INTERRUPT_H 1
# include <heimdall_threads.h>
# ifndef _WIN32
#  include <sys/socket.h> /* socklen_t */
# else
#  include <ws2tcpip.h>
# endif

struct pollfd;
struct iovec;
struct sockaddr;
struct msghdr;

/**
 * @defgroup interrupt Interruptible sleep
 * @ingroup thread
 * @{
 * @file
 * This file declares interruptible sleep functions.
 * @defgroup interrupt_sleep Interruptible sleep functions
 * @{
 */

/**
 * Interruptible variant of heimdall_sem_wait().
 *
 * Waits on a semaphore like heimdall_sem_wait(). If the calling thread has an
 * interruption context (as set by heimdall_interrupt_set()), and another thread
 * invokes heimdall_interrupt_raise() on that context, the semaphore is incremented.
 *
 * @warning The calling thread should be the only thread ever to wait on the
 * specified semaphore. Otherwise, interruptions may not be delivered
 * accurately (the wrong thread may be woken up).
 *
 * @note This function is (always) a cancellation point.
 *
 * @return EINTR if the semaphore was incremented due to an interruption,
 * otherwise zero.
 */
HEIMDALL_API int heimdall_sem_wait_i11e(heimdall_sem_t *);

/**
 * Interruptible variant of heimdall_tick_wait().
 *
 * Waits for a specified timestamp or, if the calling thread has an
 * interruption context, an interruption.
 *
 * @return EINTR if an interruption occurred, otherwise 0 once the timestamp is
 * reached.
 */
HEIMDALL_API int heimdall_mwait_i11e(heimdall_tick_t);

/**
 * Interruptible variant of heimdall_tick_sleep().
 *
 * Waits for a specified timeout duration or, if the calling thread has an
 * interruption context, an interruption.
 *
 * @param delay timeout value (in microseconds)
 *
 * @return EINTR if an interruption occurred, otherwise 0 once the timeout
 * expired.
 */
static inline int heimdall_msleep_i11e(heimdall_tick_t delay)
{
    return heimdall_mwait_i11e(heimdall_tick_now() + delay);
}

/**
 * Interruptible variant of poll().
 *
 * Waits for file descriptors I/O events, a timeout, a signal or a Heimdall I/O
 * interruption. Except for Heimdall I/O interruptions, this function behaves
 * just like the standard poll().
 *
 * @note This function is always a cancellation point (as poll()).
 * @see poll() manual page
 *
 * @param fds table of events to wait for
 * @param nfds number of entries in the table
 * @param timeout time to wait in milliseconds or -1 for infinite
 *
 * @return A strictly positive result represent the number of pending events.
 * 0 is returned if the time-out is reached without events.
 * -1 is returned if a Heimdall I/O interrupt occurs (and errno is set to EINTR)
 * or if an error occurs.
 */
HEIMDALL_API int heimdall_poll_i11e(struct pollfd *, unsigned, int);

HEIMDALL_API ssize_t heimdall_readv_i11e(int fd, struct iovec *, int);
HEIMDALL_API ssize_t heimdall_writev_i11e(int fd, const struct iovec *, int);
HEIMDALL_API ssize_t heimdall_read_i11e(int fd, void *, size_t);
HEIMDALL_API ssize_t heimdall_write_i11e(int fd, const void *, size_t);

HEIMDALL_API ssize_t heimdall_recvmsg_i11e(int fd, struct msghdr *, int flags);
HEIMDALL_API ssize_t heimdall_sendmsg_i11e(int fd, const struct msghdr *, int flags);

HEIMDALL_API ssize_t heimdall_recvfrom_i11e(int fd, void *, size_t, int flags,
                                struct sockaddr *, socklen_t *);
HEIMDALL_API ssize_t heimdall_sendto_i11e(int fd, const void *, size_t, int flags,
                              const struct sockaddr *, socklen_t);

static inline ssize_t heimdall_recv_i11e(int fd, void *buf, size_t len, int flags)
{
    return heimdall_recvfrom_i11e(fd, buf, len, flags, NULL, NULL);
}

static inline
ssize_t heimdall_send_i11e(int fd, const void *buf, size_t len, int flags)
{
    return heimdall_sendto_i11e(fd, buf, len, flags, NULL, 0);
}

HEIMDALL_API int heimdall_accept_i11e(int fd, struct sockaddr *, socklen_t *, bool);

/**
 * Registers a custom interrupt handler.
 *
 * Registers a custom callback as interrupt handler for the calling thread.
 * The callback must be unregistered with heimdall_interrupt_unregister() before
 * thread termination and before any further callback registration.
 *
 * If the calling thread has no interruption context, this function has no
 * effects.
 */
HEIMDALL_API void heimdall_interrupt_register(void (*cb)(void *), void *opaque);

HEIMDALL_API int heimdall_interrupt_unregister(void);

/**
 * @}
 * @defgroup interrupt_context Interrupt context signaling and manipulation
 * @{
 */
typedef struct heimdall_interrupt heimdall_interrupt_t;

/**
 * Creates an interruption context.
 */
HEIMDALL_API heimdall_interrupt_t *heimdall_interrupt_create(void) HEIMDALL_USED;

/**
 * Destroys an interrupt context.
 */
HEIMDALL_API void heimdall_interrupt_destroy(heimdall_interrupt_t *);

/**
 * Sets the interruption context for the calling thread.
 * @param newctx the interruption context to attach or NULL for none
 * @return the previous interruption context or NULL if none
 *
 * @note This function is not a cancellation point.
 * @warning A context can be attached to no more than one thread at a time.
 */
HEIMDALL_API heimdall_interrupt_t *heimdall_interrupt_set(heimdall_interrupt_t *);

/**
 * Raises an interruption through a specified context.
 *
 * This is used to asynchronously wake a thread up while it is waiting on some
 * other events (typically I/O events).
 *
 * @note This function is thread-safe.
 * @note This function is not a cancellation point.
 */
HEIMDALL_API void heimdall_interrupt_raise(heimdall_interrupt_t *);

/**
 * Marks the interruption context as "killed".
 *
 * This is not reversible.
 */
HEIMDALL_API void heimdall_interrupt_kill(heimdall_interrupt_t *);

/**
 * Checks if the interruption context was "killed".
 *
 * Indicates whether the interruption context of the calling thread (if any)
 * was killed with heimdall_interrupt_kill().
 */
HEIMDALL_API bool heimdall_killed(void) HEIMDALL_USED;

/**
 * Enables forwarding of interruption.
 *
 * If an interruption is raised through the context of the calling thread,
 * it will be forwarded to the specified other context. This is used to cross
 * thread boundaries.
 *
 * If the calling thread has no interrupt context, this function does nothing.
 *
 * @param to context to forward to
 * @param data opaque data pointer for the callback
 */
HEIMDALL_API void heimdall_interrupt_forward_start(heimdall_interrupt_t *to,
                                         void *data[2]);

/**
 * Undoes heimdall_interrupt_forward_start().
 *
 * This function must be called after each successful call to
 * heimdall_interrupt_forward_start() before any other interruptible call is made
 * in the same thread.
 *
 * If an interruption was raised against the context of the calling thread
 * (after the previous call to heimdall_interrupt_forward_start()), it is dequeued.
 *
 * If the calling thread has no interrupt context, this function does nothing
 * and returns zero.
 *
 * @return 0 if no interrupt was raised, EINTR if an interrupt was raised
 */
HEIMDALL_API int heimdall_interrupt_forward_stop(void *const data[2]);

/** @} */
/** @} */
#endif
