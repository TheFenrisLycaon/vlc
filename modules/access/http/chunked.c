/*****************************************************************************
 * chunked.c: HTTP 1.1 chunked encoding
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
#include <heimdall_block.h>
#include <heimdall_tls.h> /* TODO: remove this */

#include "message.h"
#include "conn.h"

ssize_t heimdall_https_chunked_write(struct heimdall_tls *tls, const void *base,
                                size_t len, bool eos)
{
    if (len > 0)
    {
        char hdr[sizeof (size_t) * 2 + 3];
        int hlen = snprintf(hdr, sizeof (hdr), "%zx\r\n", len);

        /* TODO: use iovec */
        if (heimdall_tls_Write(tls, hdr, hlen) < hlen)
            return -1;
        if (heimdall_tls_Write(tls, base, len) < (ssize_t)len)
            return -1;
        if (heimdall_tls_Write(tls, "\r\n", 2) < 2)
            return -1;
    }

    if (eos)
    {
        if (heimdall_tls_Write(tls, "0\r\n\r\n", 5) < 5)
            return -1;
    }

    return len;
}

struct heimdall_chunked_stream
{
    struct heimdall_http_stream stream;
    struct heimdall_http_stream *parent;
    struct heimdall_tls *tls;
    uintmax_t chunk_length;
    bool eof;
    bool error;
};

static void *heimdall_chunked_fatal(struct heimdall_chunked_stream *s)
{
    s->error = true;
    return heimdall_http_error;
}

static struct heimdall_http_msg *heimdall_chunked_wait(struct heimdall_http_stream *stream)
{
    /* Request trailers are not supported so far.
     * There cannot be headers during chunked encoding. */
    (void) stream;
    return NULL;
}

static block_t *heimdall_chunked_read(struct heimdall_http_stream *stream)
{
    struct heimdall_chunked_stream *s =
        container_of(stream, struct heimdall_chunked_stream, stream);
    block_t *block = NULL;

    if (s->eof)
        return NULL;
    if (s->error)
        return heimdall_http_error;

    /* Read chunk size (hexadecimal length) */
    if (s->chunk_length == 0)
    {   /* NOTE: This accepts LF in addition to CRLF. No big deal. */
        char *line = heimdall_tls_GetLine(s->tls);
        if (line == NULL)
        {
            errno = EPROTO;
            return heimdall_chunked_fatal(s);
        }

        int end;

        if (sscanf(line, "%" SCNxMAX "%n", &s->chunk_length, &end) < 1
         || (line[end] != '\0' && line[end] != ';' /* ignore extension(s) */))
            s->chunk_length = UINTMAX_MAX;

        free(line);

        if (s->chunk_length == UINTMAX_MAX)
        {
            errno = EPROTO;
            return heimdall_chunked_fatal(s);
        }
    }

    /* Read chunk data */
    if (s->chunk_length > 0)
    {
        size_t size = 1536; /* arbitrary */
        if (size > s->chunk_length)
            size = s->chunk_length;

        block = block_Alloc(size);
        if (unlikely(block == NULL))
            return NULL;

        ssize_t val = heimdall_tls_Read(s->tls, block->p_buffer, size, false);
        if (val <= 0)
        {   /* Connection error (-) or unexpected end of connection (0) */
            block_Release(block);
            return heimdall_chunked_fatal(s);
        }

        block->i_buffer = val;
        s->chunk_length -= val;
    }
    else
        s->eof = true;

    /* Read chunk end (CRLF) */
    if (s->chunk_length == 0)
    {
        char crlf[2];

        if (heimdall_tls_Read(s->tls, crlf, 2, true) < 2 || memcmp(crlf, "\r\n", 2))
            heimdall_chunked_fatal(s);
    }
    return block;
}

static void heimdall_chunked_close(struct heimdall_http_stream *stream, bool abort)
{
    struct heimdall_chunked_stream *s =
        container_of(stream, struct heimdall_chunked_stream, stream);

    if (!s->eof) /* Abort connection if stream is closed before end */
        heimdall_chunked_fatal(s);

    heimdall_http_stream_close(s->parent, abort || s->error);
    free(s);
}

static struct heimdall_http_stream_cbs heimdall_chunked_callbacks =
{
    heimdall_chunked_wait,
    NULL,
    heimdall_chunked_read,
    heimdall_chunked_close,
};

struct heimdall_http_stream *heimdall_chunked_open(struct heimdall_http_stream *parent,
                                         struct heimdall_tls *tls)
{
    struct heimdall_chunked_stream *s = malloc(sizeof (*s));
    if (unlikely(s == NULL))
        return NULL;

    s->stream.cbs = &heimdall_chunked_callbacks;
    s->parent = parent;
    s->tls = tls;
    s->chunk_length = 0;
    s->eof = false;
    s->error = false;
    return &s->stream;
}
