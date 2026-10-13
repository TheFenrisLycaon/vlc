/*****************************************************************************
 * h1conn.c: HTTP 1.x connection handling
 *****************************************************************************
 * Copyright © 2015 Rémi Denis-Courmont
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
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <heimdall_common.h>
#include <heimdall_tls.h>
#include <heimdall_block.h>

#include "conn.h"
#include "message.h"

static unsigned heimdall_http_can_read(const char *buf, size_t len)
{
    static const char end[4] = { '\r', '\n', '\r', '\n' };

    for (unsigned i = 4; i > 0; i--)
        if (len >= i && !memcmp(buf + len - i, end, i))
            return 4 - i;
    return 4;
}

/**
 * Receives HTTP headers.
 *
 * Receives HTTP 1.x response line and headers.
 *
 * @return A heap-allocated null-terminated string contained the full
 * headers, including the final CRLF.
 */
static char *heimdall_https_headers_recv(struct heimdall_tls *tls, size_t *restrict lenp)
{
    size_t size = 0, len = 0, canread;
    char *buf = NULL;

    while ((canread = heimdall_http_can_read(buf, len)) > 0)
    {
        ssize_t val;

        if (len + canread >= size)
        {
            size += 2048;
            if (size > 65536)
                goto fail;

            char *newbuf = realloc(buf, size);
            if (unlikely(newbuf == NULL))
                goto fail;

            buf = newbuf;
        }

        assert(size - len >= canread);
        //heimdall_cleanup_push(free, buf);
        val = heimdall_tls_Read(tls, buf + len, canread, true);
        //heimdall_cleanup_pop();

        if (val != (ssize_t)canread)
            goto fail;

        len += val;
    }

    assert(size - len >= 1);
    buf[len] = '\0'; /* for convenience */
    if (lenp != NULL)
        *lenp = len;
    return buf;
fail:
    free(buf);
    return NULL;
}

/** Gets minor HTTP version */
static int heimdall_http_minor(const char *msg)
{
    int minor;

    if (sscanf(msg, "HTTP/1.%1d", &minor) == 1)
        return minor;
    return -1;
}

struct heimdall_h1_conn
{
    struct heimdall_http_conn conn;
    struct heimdall_http_stream stream;
    uintmax_t content_length;
    bool connection_close;
    bool active;
    bool released;
    bool proxy;
    void *opaque;
};

#define CO(conn) ((conn)->opaque)

static void heimdall_h1_conn_destroy(struct heimdall_h1_conn *conn);

static void *heimdall_h1_stream_fatal(struct heimdall_h1_conn *conn)
{
    if (conn->conn.tls != NULL)
    {
        heimdall_http_dbg(CO(conn), "connection failed");
        heimdall_tls_Shutdown(conn->conn.tls, true);
        heimdall_tls_Close(conn->conn.tls);
        conn->conn.tls = NULL;
    }
    return NULL;
}

static struct heimdall_h1_conn *heimdall_h1_stream_conn(struct heimdall_http_stream *stream)
{
    return container_of(stream, struct heimdall_h1_conn, stream);
}

static struct heimdall_http_stream *heimdall_h1_stream_open(struct heimdall_http_conn *c,
                                 const struct heimdall_http_msg *req, bool has_data)
{
    struct heimdall_h1_conn *conn = container_of(c, struct heimdall_h1_conn, conn);
    size_t len;
    ssize_t val;

    if (conn->active || conn->conn.tls == NULL)
        return NULL;

    char *payload = heimdall_http_msg_format(req, &len, conn->proxy, has_data);
    if (unlikely(payload == NULL))
        return NULL;

    heimdall_http_dbg(CO(conn), "outgoing request:\n%.*s", (int)len, payload);
    val = heimdall_tls_Write(conn->conn.tls, payload, len);
    free(payload);

    if (val < (ssize_t)len)
        return heimdall_h1_stream_fatal(conn);

    conn->active = true;
    conn->content_length = 0;
    conn->connection_close = false;
    return &conn->stream;
}

static struct heimdall_http_msg *heimdall_h1_stream_wait(struct heimdall_http_stream *stream)
{
    struct heimdall_h1_conn *conn = heimdall_h1_stream_conn(stream);
    struct heimdall_http_msg *resp;
    const char *str;
    size_t len;
    int minor;

    assert(conn->active);

    if (conn->conn.tls == NULL)
        return NULL;

    char *payload = heimdall_https_headers_recv(conn->conn.tls, &len);
    if (payload == NULL)
        return heimdall_h1_stream_fatal(conn);

    heimdall_http_dbg(CO(conn), "incoming response:\n%.*s", (int)len, payload);

    resp = heimdall_http_msg_headers(payload);
    minor = heimdall_http_minor(payload);
    free(payload);

    if (resp == NULL)
        return heimdall_h1_stream_fatal(conn);

    assert(minor >= 0);

    conn->content_length = heimdall_http_msg_get_size(resp);
    conn->connection_close = false;

    if (minor >= 1)
    {
        if (heimdall_http_msg_get_token(resp, "Connection", "close") != NULL)
            conn->connection_close = true;

        str = heimdall_http_msg_get_token(resp, "Transfer-Encoding", "chunked");
        if (str != NULL)
        {
            if (heimdall_http_next_token(str) != NULL)
            {
                heimdall_http_msg_destroy(resp);
                return heimdall_h1_stream_fatal(conn); /* unsupported TE */
            }

            assert(conn->content_length == UINTMAX_MAX);
            stream = heimdall_chunked_open(stream, conn->conn.tls);
            if (unlikely(stream == NULL))
            {
                heimdall_http_msg_destroy(resp);
                return heimdall_h1_stream_fatal(conn);
            }
        }
    }
    else
        conn->connection_close = true;

    heimdall_http_msg_attach(resp, stream);
    return resp;
}

static ssize_t heimdall_h1_stream_write(struct heimdall_http_stream *stream,
                                   const void *base, size_t length, bool eos)
{
    struct heimdall_h1_conn *conn = heimdall_h1_stream_conn(stream);

    assert(conn->active);

    if (conn->conn.tls == NULL)
    {
        errno = EPIPE;
        return -1;
    }

    return heimdall_https_chunked_write(conn->conn.tls, base, length, eos);
}

static block_t *heimdall_h1_stream_read(struct heimdall_http_stream *stream)
{
    struct heimdall_h1_conn *conn = heimdall_h1_stream_conn(stream);
    size_t size = 2048;

    assert(conn->active);

    if (conn->conn.tls == NULL)
        return heimdall_http_error;

    if (size > conn->content_length)
        size = conn->content_length;
    if (size == 0)
        return NULL;

    block_t *block = block_Alloc(size);
    if (unlikely(block == NULL))
        return heimdall_http_error;

    ssize_t val = heimdall_tls_Read(conn->conn.tls, block->p_buffer, size, false);
    if (val <= 0)
    {
        block_Release(block);
        if (val < 0)
            return heimdall_http_error;
        if (conn->content_length != UINTMAX_MAX)
        {
            errno = ECONNRESET;
            return heimdall_http_error;
        }
        return NULL;
    }

    block->i_buffer = val;
    if (conn->content_length != UINTMAX_MAX)
        conn->content_length -= val;

    return block;
}

static void heimdall_h1_stream_close(struct heimdall_http_stream *stream, bool abort)
{
    struct heimdall_h1_conn *conn = heimdall_h1_stream_conn(stream);

    assert(conn->active);

    if (conn->connection_close)
        /* Server requested closing the connection after this stream. */
        abort = true;

    if (conn->content_length > 0 && conn->content_length != UINTMAX_MAX)
        /* Client left some data to be read, so pipelining is impossible. */
        abort = true;

    if (abort)
        /* Shut the underlying connection down and prevent reuse. */
        heimdall_h1_stream_fatal(conn);

    conn->active = false;

    if (conn->released)
        heimdall_h1_conn_destroy(conn);
}

static const struct heimdall_http_stream_cbs heimdall_h1_stream_callbacks =
{
    heimdall_h1_stream_wait,
    heimdall_h1_stream_write,
    heimdall_h1_stream_read,
    heimdall_h1_stream_close,
};

static void heimdall_h1_conn_destroy(struct heimdall_h1_conn *conn)
{
    assert(!conn->active);
    assert(conn->released);

    if (conn->conn.tls != NULL)
    {
        heimdall_tls_Shutdown(conn->conn.tls, true);
        heimdall_tls_Close(conn->conn.tls);
    }
    free(conn);
}

static void heimdall_h1_conn_release(struct heimdall_http_conn *c)
{
    struct heimdall_h1_conn *conn = container_of(c, struct heimdall_h1_conn, conn);

    assert(!conn->released);
    conn->released = true;

    if (!conn->active)
        heimdall_h1_conn_destroy(conn);
}

static const struct heimdall_http_conn_cbs heimdall_h1_conn_callbacks =
{
    heimdall_h1_stream_open,
    heimdall_h1_conn_release,
};

struct heimdall_http_conn *heimdall_h1_conn_create(void *ctx, heimdall_tls_t *tls, bool proxy)
{
    struct heimdall_h1_conn *conn = malloc(sizeof (*conn));
    if (unlikely(conn == NULL))
        return NULL;

    conn->conn.cbs = &heimdall_h1_conn_callbacks;
    conn->conn.tls = tls;
    conn->stream.cbs = &heimdall_h1_stream_callbacks;
    conn->active = false;
    conn->released = false;
    conn->proxy = proxy;
    conn->opaque = ctx;

    return &conn->conn;
}

struct heimdall_http_stream *heimdall_h1_request(void *ctx, const char *hostname,
                                       unsigned port, bool proxy,
                                       const struct heimdall_http_msg *req,
                                       bool idempotent, bool has_data,
                                       struct heimdall_http_conn **restrict connp)
{
    struct addrinfo hints =
    {
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
    }, *res;

    heimdall_http_dbg(ctx, "resolving %s ...", hostname);

    int val = heimdall_getaddrinfo_i11e(hostname, port, &hints, &res);
    if (val != 0)
    {   /* TODO: C locale for gai_strerror() */
        heimdall_http_err(ctx, "cannot resolve %s: %s", hostname,
                     gai_strerror(val));
        return NULL;
    }

    for (const struct addrinfo *p = res; p != NULL; p = p->ai_next)
    {
        heimdall_tls_t *tcp = heimdall_tls_SocketOpenAddrInfo(p, idempotent);
        if (tcp == NULL)
        {
            heimdall_http_err(ctx, "socket error: %s", heimdall_strerror_c(errno));
            continue;
        }

        struct heimdall_http_conn *conn = heimdall_h1_conn_create(ctx, tcp, proxy);
        if (unlikely(conn == NULL))
        {
            heimdall_tls_SessionDelete(tcp);
            continue;
        }

        /* Send the HTTP request */
        struct heimdall_http_stream *stream = heimdall_http_stream_open(conn, req,
                                                              has_data);

        if (stream != NULL)
        {
            if (connp != NULL)
                *connp = conn;
            else
                heimdall_http_conn_release(conn);

            freeaddrinfo(res);
            return stream;
        }

        heimdall_http_conn_release(conn);

        if (!idempotent)
            break; /* If the request is nonidempotent, it cannot be resent. */
    }

    /* All address info failed. */
    freeaddrinfo(res);
    return NULL;
}
