/*****************************************************************************
 * h2output.c: HTTP/2 send queue
 *****************************************************************************
 * Copyright (C) 2015 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
#include <errno.h>
#include <stdlib.h>
#ifdef HAVE_POLL_H
# include <poll.h>
#endif
#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif
#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_poll.h>
#include <heimdall_tls.h>
#include "h2frame.h"
#include "h2output.h"

#define HEIMDALL_H2_MAX_QUEUE (1u << 24)

struct heimdall_h2_queue
{
    struct heimdall_h2_frame *first;
    struct heimdall_h2_frame **last;
};

struct heimdall_h2_output
{
    struct heimdall_tls *tls;

    struct heimdall_h2_queue prio; /*< Priority send queue */
    struct heimdall_h2_queue queue; /*< Normal send queue */
    size_t size; /*< Total queues depth (bytes) */
    bool failed; /*< Connection failure flag */
    bool closing; /*< Connection shutdown pending flag */

    heimdall_mutex_t lock;
    heimdall_cond_t wait;
    heimdall_thread_t thread;
};

/** Queues one outgoing HTTP/2. */
static int heimdall_h2_output_queue(struct heimdall_h2_output *out,
                               struct heimdall_h2_queue *q, struct heimdall_h2_frame *f)
{
    if (unlikely(f == NULL))
        return -1; /* memory error */

    /* Iterate the list to count size and find tail pointer */
    struct heimdall_h2_frame **lastp = &f;
    size_t len = 0;

    do
    {
        struct heimdall_h2_frame *n = *lastp;

        len += heimdall_h2_frame_size(n);
        lastp = &n->next;
    }
    while (*lastp != NULL);

    heimdall_mutex_lock(&out->lock);
    if (out->failed)
        goto error;

    out->size += len;
    if (out->size >= HEIMDALL_H2_MAX_QUEUE)
    {   /* The queue is full. This should never happen but it can be triggered
         * by an evil peer at the other end (e.g. sending a lot of pings and
         * never receiving pongs. Returning an error is better than filling
         * all memory. */
        out->size -= len;
        goto error;
    }

    assert(*(q->last) == NULL);
    *(q->last) = f;
    q->last = lastp;
    heimdall_cond_signal(&out->wait);
    heimdall_mutex_unlock(&out->lock);
    return 0;

error:
    heimdall_mutex_unlock(&out->lock);
    while (f != NULL)
    {
        struct heimdall_h2_frame *n = f->next;

        free(f);
        f = n;
    }
    return -1;
}

int heimdall_h2_output_send_prio(struct heimdall_h2_output *out, struct heimdall_h2_frame *f)
{
    return heimdall_h2_output_queue(out, &out->prio, f);
}

int heimdall_h2_output_send(struct heimdall_h2_output *out, struct heimdall_h2_frame *f)
{
    return heimdall_h2_output_queue(out, &out->queue, f);
}

/** Dequeues one outgoing HTTP/2. */
static struct heimdall_h2_frame *heimdall_h2_output_dequeue(struct heimdall_h2_output *out)
{
    struct heimdall_h2_queue *q;
    struct heimdall_h2_frame *frame;
    size_t len;

    heimdall_mutex_lock(&out->lock);

    for (;;)
    {
        q = &out->prio;
        if (q->first != NULL)
            break;

        q = &out->queue;
        if (q->first != NULL)
            break;

        if (unlikely(out->closing))
        {
            heimdall_mutex_unlock(&out->lock);
            return NULL;
        }

        int canc = heimdall_savecancel();
        heimdall_cond_wait(&out->wait, &out->lock);
        heimdall_restorecancel(canc);
    }

    frame = q->first;
    q->first = frame->next;
    if (frame->next == NULL)
    {
        assert(q->last == &frame->next);
        q->last = &q->first;
    }
    assert(q->last != &frame->next);

    len = heimdall_h2_frame_size(frame);
    assert(out->size >= len);
    out->size -= len;

    heimdall_mutex_unlock(&out->lock);

    frame->next = NULL;
    return frame;
}

static void heimdall_h2_output_flush_unlocked(struct heimdall_h2_output *out)
{
    for (struct heimdall_h2_frame *f = out->prio.first, *n; f != NULL; f = n)
    {
        n = f->next;
        free(f);
    }
    for (struct heimdall_h2_frame *f = out->queue.first, *n; f != NULL; f = n)
    {
        n = f->next;
        free(f);
    }
}

/**
 * Sends bytes to a connection.
 * @note This may be a cancellation point.
 * The caller is responsible for serializing writes on a given connection.
 */
static ssize_t heimdall_https_send(heimdall_tls_t *tls, const void *buf, size_t len)
{
    struct iovec iov;
    size_t count = 0;

    iov.iov_base = (void *)buf;
    iov.iov_len = len;

    while (count < len)
    {
        int canc = heimdall_savecancel();
        ssize_t val = tls->ops->writev(tls, &iov, 1);

        heimdall_restorecancel(canc);

        if (val > 0)
        {
            iov.iov_base = (char *)iov.iov_base + val;
            iov.iov_len -= val;
            count += val;
            continue;
        }

        if (val == 0)
            break;

        if (errno != EINTR && errno != EAGAIN)
            return count ? (ssize_t)count : -1;

        struct pollfd ufd;

        ufd.events = POLLOUT;
        ufd.fd = heimdall_tls_GetPollFD(tls, &ufd.events);
        poll(&ufd, 1, -1);
    }

    return count;
}

/**
 * Sends one HTTP/2 frame through TLS.
 *
 * This function sends a whole HTTP/2 frame through a TLS session, then
 * releases the memory used by the frame.
 *
 * The caller must "own" the write side of the TLS session.
 *
 * @note This is a blocking function and may be a thread cancellation point.
 *
 * @return 0 on success, -1 if the connection failed
 */
static int heimdall_h2_frame_send(struct heimdall_tls *tls, struct heimdall_h2_frame *f)
{
    size_t len = heimdall_h2_frame_size(f);
    ssize_t val;

    heimdall_cleanup_push(free, f);
    val = heimdall_https_send(tls, f->data, len);
    heimdall_cleanup_pop();
    free(f);

    return ((size_t)val == len) ? 0 : -1;
}

/** Output thread */
static void *heimdall_h2_output_thread(void *data)
{
    struct heimdall_h2_output *out = data;
    struct heimdall_h2_frame *frame;

    heimdall_thread_set_name("heimdall-h2-send");

    while ((frame = heimdall_h2_output_dequeue(out)) != NULL)
    {
        if (heimdall_h2_frame_send(out->tls, frame))
        {   /* The connection failed asynchronously. The caller will be
             * notified at the next attempt to queue (as with TCP sockets). */
            heimdall_mutex_lock(&out->lock);
            out->failed = true;
            heimdall_mutex_unlock(&out->lock);

            /* The caller will leave the queues alone from now on until this
             * thread ends. The queues are flushed to free memory. */
            heimdall_h2_output_flush_unlocked(out);
            out->prio.first = NULL;
            out->prio.last = &out->prio.first;
            out->queue.first = NULL;
            out->queue.last = &out->queue.first;
            break;
        }
    }

    return NULL;
}

static void *heimdall_h2_client_output_thread(void *data)
{
    static const char http2_hello[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    struct heimdall_h2_output *out = data;

    heimdall_thread_set_name("heimdall-h2-csend");

    if (heimdall_https_send(out->tls, http2_hello, 24) < 24)
    {
        heimdall_mutex_lock(&out->lock);
        out->failed = true;
        heimdall_mutex_unlock(&out->lock);
        return NULL;
    }

    return heimdall_h2_output_thread(data);
}

struct heimdall_h2_output *heimdall_h2_output_create(struct heimdall_tls *tls, bool client)
{
    struct heimdall_h2_output *out = malloc(sizeof (*out));
    if (unlikely(out == NULL))
        return NULL;

    out->tls = tls;

    out->prio.first = NULL;
    out->prio.last = &out->prio.first;
    out->queue.first = NULL;
    out->queue.last = &out->queue.first;
    out->size = 0;
    out->failed = false;
    out->closing = false;

    heimdall_mutex_init(&out->lock);
    heimdall_cond_init(&out->wait);

    void *(*cb)(void *) = client ? heimdall_h2_client_output_thread
                                 : heimdall_h2_output_thread;
    if (heimdall_clone(&out->thread, cb, out))
    {
        free(out);
        out = NULL;
    }
    return out;
}

void heimdall_h2_output_destroy(struct heimdall_h2_output *out)
{
    heimdall_mutex_lock(&out->lock);
    out->closing = true;
    heimdall_cond_signal(&out->wait);
    heimdall_mutex_unlock(&out->lock);

    heimdall_cancel(out->thread);
    heimdall_join(out->thread, NULL);

    /* Flush queues in case the thread was terminated within poll() and some
     * packets were still queued. */
    heimdall_h2_output_flush_unlocked(out);
    free(out);
}
