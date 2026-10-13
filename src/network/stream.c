/*****************************************************************************
 * stream.c
 *****************************************************************************
 * Copyright © 2004-2016 Rémi Denis-Courmont
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

/**
 * @ingroup transport
 * @file
 * Transport-layer stream abstraction
 *
 * This file implements the transport-layer stream (heimdall_tls) abstraction.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_POLL_H
# include <poll.h>
#endif
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifndef SOL_TCP
# define SOL_TCP IPPROTO_TCP
#endif

#include <heimdall_common.h>
#include <heimdall_tls.h>
#include <heimdall_interrupt.h>

ssize_t heimdall_tls_Read(heimdall_tls_t *session, void *buf, size_t len, bool waitall)
{
    struct pollfd ufd;
    struct iovec iov;

    ufd.events = POLLIN;
    ufd.fd = heimdall_tls_GetPollFD(session, &ufd.events);
    iov.iov_base = buf;
    iov.iov_len = len;

    for (size_t rcvd = 0;;)
    {
        if (heimdall_killed())
        {
            errno = EINTR;
            return -1;
        }

        ssize_t val = session->ops->readv(session, &iov, 1);
        if (val > 0)
        {
            if (!waitall)
                return val;
            iov.iov_base = (char *)iov.iov_base + val;
            iov.iov_len -= val;
            rcvd += val;
        }
        if (iov.iov_len == 0 || val == 0)
            return rcvd;
        if (val == -1)
        {
            if (heimdall_killed())
                return -1;
            if (errno != EINTR && errno != EAGAIN)
                return rcvd ? (ssize_t)rcvd : -1;
        }

        heimdall_poll_i11e(&ufd, 1, -1);
    }
}

ssize_t heimdall_tls_Write(heimdall_tls_t *session, const void *buf, size_t len)
{
    struct pollfd ufd;
    struct iovec iov;

    ufd.events = POLLOUT;
    ufd.fd = heimdall_tls_GetPollFD(session, &ufd.events);
    iov.iov_base = (void *)buf;
    iov.iov_len = len;

    for (size_t sent = 0;;)
    {
        if (heimdall_killed())
        {
            errno = EINTR;
            return -1;
        }

        ssize_t val = session->ops->writev(session, &iov, 1);
        if (val > 0)
        {
            iov.iov_base = ((char *)iov.iov_base) + val;
            iov.iov_len -= val;
            sent += val;
        }
        if (iov.iov_len == 0 || val == 0)
            return sent;
        if (val == -1)
        {
            if (heimdall_killed())
                return -1;
            if (errno != EINTR && errno != EAGAIN)
                return sent ? (ssize_t)sent : -1;
        }

        heimdall_poll_i11e(&ufd, 1, -1);
    }
}

char *heimdall_tls_GetLine(heimdall_tls_t *session)
{
    char *line = NULL;
    size_t linelen = 0, linesize = 0;

    do
    {
        if (linelen == linesize)
        {
            linesize += 1024;

            char *newline = realloc(line, linesize);
            if (unlikely(newline == NULL))
                goto error;
            line = newline;
        }

        if (heimdall_tls_Read(session, line + linelen, 1, false) <= 0)
            goto error;
    }
    while (line[linelen++] != '\n');

    if (linelen >= 2 && line[linelen - 2] == '\r')
        line[linelen - 2] = '\0';
    else
        line[linelen - 1] = '\0';
    return line;

error:
    free(line);
    return NULL;
}

typedef struct heimdall_tls_socket
{
    struct heimdall_tls tls;
    int fd;
    socklen_t peerlen;
    struct sockaddr peer[];
} heimdall_tls_socket_t;

static int heimdall_tls_SocketGetFD(heimdall_tls_t *tls, short *restrict events)
{
    heimdall_tls_socket_t *sock = (struct heimdall_tls_socket *)tls;

    (void) events;
    return sock->fd;
}

static ssize_t heimdall_tls_SocketRead(heimdall_tls_t *tls, struct iovec *iov,
                                  unsigned count)
{
    heimdall_tls_socket_t *sock = (struct heimdall_tls_socket *)tls;
    struct msghdr msg =
    {
        .msg_iov = iov,
        .msg_iovlen = count,
    };

    return recvmsg(sock->fd, &msg, 0);
}

static ssize_t heimdall_tls_SocketWrite(heimdall_tls_t *tls, const struct iovec *iov,
                                   unsigned count)
{
    heimdall_tls_socket_t *sock = (struct heimdall_tls_socket *)tls;
    const struct msghdr msg =
    {
        .msg_iov = (struct iovec *)iov,
        .msg_iovlen = count,
    };

    return heimdall_sendmsg(sock->fd, &msg, 0);
}

static int heimdall_tls_SocketShutdown(heimdall_tls_t *tls, bool duplex)
{
    heimdall_tls_socket_t *sock = (struct heimdall_tls_socket *)tls;

    return shutdown(sock->fd, duplex ? SHUT_RDWR : SHUT_WR);
}

static void heimdall_tls_SocketClose(heimdall_tls_t *tls)
{
    heimdall_tls_socket_t *sock = (struct heimdall_tls_socket *)tls;

    net_Close(sock->fd);
    free(tls);
}

static const struct heimdall_tls_operations heimdall_tls_socket_ops =
{
    heimdall_tls_SocketGetFD,
    heimdall_tls_SocketRead,
    heimdall_tls_SocketWrite,
    heimdall_tls_SocketShutdown,
    heimdall_tls_SocketClose,
};

static heimdall_tls_t *heimdall_tls_SocketAlloc(int fd,
                                      const struct sockaddr *restrict peer,
                                      socklen_t peerlen)
{
    heimdall_tls_socket_t *sock = malloc(sizeof (*sock) + peerlen);
    if (unlikely(sock == NULL))
        return NULL;

    heimdall_tls_t *tls = &sock->tls;

    tls->ops = &heimdall_tls_socket_ops;
    tls->p = NULL;

    sock->fd = fd;
    sock->peerlen = peerlen;
    if (peerlen > 0)
        memcpy(sock->peer, peer, peerlen);
    return tls;
}

heimdall_tls_t *heimdall_tls_SocketOpen(int fd)
{
    return heimdall_tls_SocketAlloc(fd, NULL, 0);
}

int heimdall_tls_SocketPair(int family, int protocol, heimdall_tls_t *pair[2])
{
    int fds[2];

    if (heimdall_socketpair(family, SOCK_STREAM, protocol, fds, true))
        return -1;

    for (size_t i = 0; i < 2; i++)
    {
        setsockopt(fds[i], SOL_SOCKET, SO_REUSEADDR,
                   &(int){ 1 }, sizeof (int));

        pair[i] = heimdall_tls_SocketAlloc(fds[i], NULL, 0);
        if (unlikely(pair[i] == NULL))
        {
            net_Close(fds[i]);
            if (i)
                heimdall_tls_SessionDelete(pair[0]);
            else
                net_Close(fds[1]);
            return -1;
        }
    }
    return 0;
}

/**
 * Allocates an unconnected transport layer socket.
 */
static heimdall_tls_t *heimdall_tls_SocketAddrInfo(const struct addrinfo *restrict info)
{
    int fd = heimdall_socket(info->ai_family, info->ai_socktype, info->ai_protocol,
                        true /* nonblocking */);
    if (fd == -1)
        return NULL;

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof (int));

    if (info->ai_socktype == SOCK_STREAM && info->ai_protocol == IPPROTO_TCP)
        setsockopt(fd, SOL_TCP, TCP_NODELAY, &(int){ 1 }, sizeof (int));

    heimdall_tls_t *sk = heimdall_tls_SocketAlloc(fd, info->ai_addr, info->ai_addrlen);
    if (unlikely(sk == NULL))
        net_Close(fd);
    return sk;
}

/**
 * Waits for pending transport layer socket connection.
 */
static int heimdall_tls_WaitConnect(heimdall_tls_t *tls)
{
    const int fd = heimdall_tls_GetFD(tls);
    struct pollfd ufd;

    ufd.fd = fd;
    ufd.events = POLLOUT;

    do
    {
        if (heimdall_killed())
        {
            errno = EINTR;
            return -1;
        }
    }
    while (heimdall_poll_i11e(&ufd, 1, -1) <= 0);

    int val;
    socklen_t len = sizeof (val);

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &val, &len))
        return -1;

    if (val != 0)
    {
        errno = val;
        return -1;
    }
    return 0;
}

/**
 * Connects a transport layer socket.
 */
static ssize_t heimdall_tls_Connect(heimdall_tls_t *tls)
{
    const heimdall_tls_socket_t *sock = (heimdall_tls_socket_t *)tls;

    if (connect(sock->fd, sock->peer, sock->peerlen) == 0)
        return 0;
#ifndef _WIN32
    if (errno != EINPROGRESS)
        return -1;
#else
    if (WSAGetLastError() != WSAEWOULDBLOCK)
        return -1;
#endif
    return heimdall_tls_WaitConnect(tls);
}

/* Callback for combined connection establishment and initial send */
static ssize_t heimdall_tls_ConnectWrite(heimdall_tls_t *tls,
                                    const struct iovec *iov,unsigned count)
{
    /* Next time, write directly. Do not retry to connect. */
    tls->ops = &heimdall_tls_socket_ops;

#ifdef MSG_FASTOPEN
    heimdall_tls_socket_t *sock = (heimdall_tls_socket_t *)tls;
    const struct msghdr msg =
    {
        .msg_name = sock->peer,
        .msg_namelen = sock->peerlen,
        .msg_iov = (struct iovec *)iov,
        .msg_iovlen = count,
    };
    ssize_t ret;

    ret = heimdall_sendmsg(sock->fd, &msg, MSG_FASTOPEN);
    if (ret >= 0)
    {   /* Fast open in progress */
        return ret;
    }

    if (errno == EINPROGRESS)
    {
        if (heimdall_tls_WaitConnect(tls))
            return -1;
    }
    else
    if (errno != EOPNOTSUPP)
        return -1;
    /* Fast open not supported or disabled... fallback to normal mode */
#endif

    if (heimdall_tls_Connect(tls))
        return -1;

    return heimdall_tls_SocketWrite(tls, iov, count);
}

static const struct heimdall_tls_operations heimdall_tls_socket_fastopen_ops =
{
    heimdall_tls_SocketGetFD,
    heimdall_tls_SocketRead,
    heimdall_tls_ConnectWrite,
    heimdall_tls_SocketShutdown,
    heimdall_tls_SocketClose,
};

heimdall_tls_t *heimdall_tls_SocketOpenAddrInfo(const struct addrinfo *restrict info,
                                      bool defer_connect)
{
    heimdall_tls_t *sock = heimdall_tls_SocketAddrInfo(info);
    if (sock == NULL)
        return NULL;

    if (defer_connect)
    {   /* The socket is not connected yet.
         * The connection will be triggered on the first send. */
        sock->ops = &heimdall_tls_socket_fastopen_ops;
    }
    else
    {
        if (heimdall_tls_Connect(sock))
        {
            heimdall_tls_SessionDelete(sock);
            sock = NULL;
        }
    }
    return sock;
}

heimdall_tls_t *heimdall_tls_SocketOpenTCP(heimdall_object_t *obj, const char *name,
                                 unsigned port)
{
    struct addrinfo hints =
    {
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
    }, *res;

    assert(name != NULL);
    msg_Dbg(obj, "resolving %s ...", name);

    int val = heimdall_getaddrinfo_i11e(name, port, &hints, &res);
    if (val != 0)
    {   /* TODO: C locale for gai_strerror() */
        msg_Err(obj, "cannot resolve %s port %u: %s", name, port,
                gai_strerror(val));
        return NULL;
    }

    msg_Dbg(obj, "connecting to %s port %u ...", name, port);

    /* TODO: implement RFC8305 */
    for (const struct addrinfo *p = res; p != NULL; p = p->ai_next)
    {
        heimdall_tls_t *tls = heimdall_tls_SocketOpenAddrInfo(p, false);
        if (tls == NULL)
        {
            msg_Err(obj, "connection error: %s", heimdall_strerror_c(errno));
            continue;
        }

        freeaddrinfo(res);
        return tls;
    }

    freeaddrinfo(res);
    return NULL;
}
