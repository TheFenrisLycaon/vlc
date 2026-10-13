/*****************************************************************************
 * datagram.c:
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

#include <errno.h>
#include <stdlib.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#include <heimdall_common.h>
#include <heimdall_network.h>
#include <heimdall_poll.h>
#include "heimdall_dtls.h"

#ifndef MSG_TRUNC
#define MSG_TRUNC 0
#endif

struct heimdall_dgram_sock
{
    int fd;
    struct heimdall_dtls s;
};

static void heimdall_datagram_Close(struct heimdall_dtls *dgs)
{
    struct heimdall_dgram_sock *s = container_of(dgs, struct heimdall_dgram_sock, s);

#ifndef _WIN32
    heimdall_close(s->fd);
#else
    closesocket(s->fd);
#endif
    free(s);
}

/* Note: must not be a cancellation point */
static int heimdall_datagram_GetPollFD(struct heimdall_dtls *dgs, short *restrict ev)
{
    (void) ev; /* no changes there */
    return container_of(dgs, struct heimdall_dgram_sock, s)->fd;
}

static ssize_t heimdall_datagram_Recv(struct heimdall_dtls *dgs, struct iovec *iov,
                                 unsigned iovlen, bool *truncated)
{
    struct msghdr msg = {
        .msg_iov = iov,
        .msg_iovlen = iovlen,
    };
    int fd = container_of(dgs, struct heimdall_dgram_sock, s)->fd;
    ssize_t ret = recvmsg(fd, &msg, 0);

    if (ret >= 0)
        *truncated = (msg.msg_flags & MSG_TRUNC) != 0;

    return ret;
}

static ssize_t heimdall_datagram_Send(struct heimdall_dtls *dgs,
                                 const struct iovec *iov, unsigned iovlen)
{
    const struct msghdr msg = {
        .msg_iov = (struct iovec *)iov,
        .msg_iovlen = iovlen,
    };
    int fd = container_of(dgs, struct heimdall_dgram_sock, s)->fd;

    return heimdall_sendmsg(fd, &msg, 0);
}

static const struct heimdall_dtls_operations heimdall_datagram_ops = {
    heimdall_datagram_Close,
    heimdall_datagram_GetPollFD,
    heimdall_datagram_Recv,
    heimdall_datagram_Send,
};

struct heimdall_dtls *heimdall_datagram_CreateFD(int fd)
{
    struct heimdall_dgram_sock *s = malloc(sizeof (*s));

    if (likely(s != NULL)) {
        s->fd = fd;
        s->s.ops = &heimdall_datagram_ops;
    }

    return &s->s;
}

static ssize_t heimdall_dccp_Recv(struct heimdall_dtls *dgs, struct iovec *iov,
                             unsigned iovlen, bool *truncated)
{
    ssize_t ret = heimdall_datagram_Recv(dgs, iov, iovlen, truncated);

    if (unlikely(ret == 0)) {
        int fd = container_of(dgs, struct heimdall_dgram_sock, s)->fd;
        struct pollfd ufd = { .fd = fd, };

        /* On a connection-oriented datagram socket, recv() can return zero
         * when a zero-bytes packet was received or when the other end closed
         * the connection. We need to distinguish the two, so check if the
         * socket is hung up or not.
         *
         * Note that this test can only be done *after* the zero read. The HUP
         * flag is not set by the IP stack until then.
         */
        poll(&ufd, 1, 0);

        if (ufd.revents & POLLHUP) {
            /* We need a distinct error code. EPIPE is normally only used on
             * send(), so there are no ambiguities and it is somewhat
             * descriptive of the connection having been closed.
             */
            errno = EPIPE;
            ret = -1;
        }
    }

    return ret;
}

static const struct heimdall_dtls_operations heimdall_dccp_ops = {
    heimdall_datagram_Close,
    heimdall_datagram_GetPollFD,
    heimdall_dccp_Recv,
    heimdall_datagram_Send,
};

struct heimdall_dtls *heimdall_dccp_CreateFD(int fd)
{
    struct heimdall_dgram_sock *s = malloc(sizeof (*s));

    if (likely(s != NULL)) {
        s->fd = fd;
        s->s.ops = &heimdall_dccp_ops;
    }

    return &s->s;
}
