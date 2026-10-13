/*****************************************************************************
 * h2frame.c: HTTP/2 frame formatting
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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <heimdall_common.h>

#include "conn.h"
#include "hpack.h"
#include "h2frame.h"

static struct heimdall_h2_frame *
heimdall_h2_frame_alloc(uint_fast8_t type, uint_fast8_t flags,
                   uint_fast32_t stream_id, size_t length)
{
    assert((stream_id >> 31) == 0);

    if (unlikely(length > HEIMDALL_H2_MAX_MAX_FRAME))
    {
        errno = EINVAL;
        return NULL;
    }

    struct heimdall_h2_frame *f = malloc(sizeof (*f) + 9 + length);
    if (unlikely(f == NULL))
        return NULL;

    f->next = NULL;
    f->data[0] = length >> 16;
    f->data[1] = length >> 8;
    f->data[2] = length;
    f->data[3] = type;
    f->data[4] = flags;
    SetDWBE(f->data + 5, stream_id);
    return f;
}

#define heimdall_h2_frame_payload(f) ((f)->data + 9)

static uint_fast32_t heimdall_h2_frame_length(const struct heimdall_h2_frame *f)
{
    const uint8_t *buf = f->data;
    return (buf[0] << 16) | (buf[1] << 8) | buf[2];
}

size_t heimdall_h2_frame_size(const struct heimdall_h2_frame *f)
{
    return 9 + heimdall_h2_frame_length(f);
}

static uint_fast8_t heimdall_h2_frame_type(const struct heimdall_h2_frame *f)
{
    return f->data[3];
}

static uint_fast8_t heimdall_h2_frame_flags(const struct heimdall_h2_frame *f)
{
    return f->data[4];
}

static uint_fast32_t heimdall_h2_frame_id(const struct heimdall_h2_frame *f)
{
    return GetDWBE(f->data + 5) & 0x7FFFFFFF;
}

enum {
    HEIMDALL_H2_FRAME_DATA,
    HEIMDALL_H2_FRAME_HEADERS,
    HEIMDALL_H2_FRAME_PRIORITY,
    HEIMDALL_H2_FRAME_RST_STREAM,
    HEIMDALL_H2_FRAME_SETTINGS,
    HEIMDALL_H2_FRAME_PUSH_PROMISE,
    HEIMDALL_H2_FRAME_PING,
    HEIMDALL_H2_FRAME_GOAWAY,
    HEIMDALL_H2_FRAME_WINDOW_UPDATE,
    HEIMDALL_H2_FRAME_CONTINUATION,
};

static const char *heimdall_h2_type_name(uint_fast8_t type)
{
    static const char names[][14] = {
        [HEIMDALL_H2_FRAME_DATA]          = "DATA",
        [HEIMDALL_H2_FRAME_HEADERS]       = "HEADERS",
        [HEIMDALL_H2_FRAME_PRIORITY]      = "PRIORITY",
        [HEIMDALL_H2_FRAME_RST_STREAM]    = "RST_STREAM",
        [HEIMDALL_H2_FRAME_SETTINGS]      = "SETTINGS",
        [HEIMDALL_H2_FRAME_PUSH_PROMISE]  = "PUSH_PROMISE",
        [HEIMDALL_H2_FRAME_PING]          = "PING",
        [HEIMDALL_H2_FRAME_GOAWAY]        = "GOAWAY",
        [HEIMDALL_H2_FRAME_WINDOW_UPDATE] = "WINDOW_UPDATE",
        [HEIMDALL_H2_FRAME_CONTINUATION]  = "CONTINUATION",
    };

    if (type >= ARRAY_SIZE(names) || names[type][0] == '\0')
        return "<unknown>";
    return names[type];
}

enum {
    HEIMDALL_H2_DATA_END_STREAM = 0x01,
    HEIMDALL_H2_DATA_PADDED     = 0x08,
};

enum {
    HEIMDALL_H2_HEADERS_END_STREAM  = 0x01,
    HEIMDALL_H2_HEADERS_END_HEADERS = 0x04,
    HEIMDALL_H2_HEADERS_PADDED      = 0x08,
    HEIMDALL_H2_HEADERS_PRIORITY    = 0x20,
};

enum {
    HEIMDALL_H2_SETTINGS_ACK = 0x01,
};

enum {
    HEIMDALL_H2_PUSH_PROMISE_END_HEADERS = 0x04,
    HEIMDALL_H2_PUSH_PROMISE_PADDED      = 0x08,
};

enum {
    HEIMDALL_H2_PING_ACK = 0x01,
};

enum {
    HEIMDALL_H2_CONTINUATION_END_HEADERS = 0x04,
};

struct heimdall_h2_frame *
heimdall_h2_frame_headers(uint_fast32_t stream_id, uint_fast32_t mtu, bool eos,
                     unsigned count, const char *const headers[][2])
{
    struct heimdall_h2_frame *f;
    uint8_t flags = eos ? HEIMDALL_H2_HEADERS_END_STREAM : 0;

    size_t len = hpack_encode(NULL, 0, headers, count);

    if (likely(len <= mtu))
    {   /* Most common case: single frame - with zero copy */
        flags |= HEIMDALL_H2_HEADERS_END_HEADERS;

        f = heimdall_h2_frame_alloc(HEIMDALL_H2_FRAME_HEADERS, flags, stream_id, len);
        if (unlikely(f == NULL))
            return NULL;

        hpack_encode(heimdall_h2_frame_payload(f), len, headers, count);
        return f;
    }

    /* Edge case: HEADERS frame then CONTINUATION frame(s) */
    uint8_t *payload = malloc(len);
    if (unlikely(payload == NULL))
        return NULL;

    hpack_encode(payload, len, headers, count);

    struct heimdall_h2_frame **pp = &f, *n;
    const uint8_t *offset = payload;
    uint_fast8_t type = HEIMDALL_H2_FRAME_HEADERS;

    f = NULL;

    while (len > mtu)
    {
        n = heimdall_h2_frame_alloc(type, flags, stream_id, mtu);
        if (unlikely(n == NULL))
            goto error;

        memcpy(heimdall_h2_frame_payload(n), offset, mtu);
        *pp = n;
        pp = &n->next;

        type = HEIMDALL_H2_FRAME_CONTINUATION;
        flags = 0;
        offset += mtu;
        len -= mtu;
    }

    flags |= HEIMDALL_H2_CONTINUATION_END_HEADERS;

    n = heimdall_h2_frame_alloc(type, flags, stream_id, len);
    if (unlikely(n == NULL))
        goto error;

    memcpy(heimdall_h2_frame_payload(n), offset, len);
    *pp = n;

    free(payload);
    return f;

error:
    while (f != NULL)
    {
        n = f->next;
        free(f);
        f = n;
    }
    free(payload);
    return NULL;
}

struct heimdall_h2_frame *
heimdall_h2_frame_data(uint_fast32_t stream_id, const void *buf, size_t len,
                  bool eos)
{
    struct heimdall_h2_frame *f;
    uint8_t flags = eos ? HEIMDALL_H2_DATA_END_STREAM : 0;

    f = heimdall_h2_frame_alloc(HEIMDALL_H2_FRAME_DATA, flags, stream_id, len);
    if (len > 0 && likely(f != NULL))
        memcpy(heimdall_h2_frame_payload(f), buf, len);
    return f;
}

struct heimdall_h2_frame *
heimdall_h2_frame_rst_stream(uint_fast32_t stream_id, uint_fast32_t error_code)
{
    struct heimdall_h2_frame *f = heimdall_h2_frame_alloc(HEIMDALL_H2_FRAME_RST_STREAM, 0,
                                                stream_id, 4);
    if (likely(f != NULL))
        SetDWBE(heimdall_h2_frame_payload(f), error_code);
    return f;
}

struct heimdall_h2_frame *heimdall_h2_frame_settings(void)
{
    unsigned n = (HEIMDALL_H2_MAX_HEADER_TABLE != HEIMDALL_H2_DEFAULT_MAX_HEADER_TABLE)
               + 1 /* ENABLE_PUSH */
#if defined(HEIMDALL_H2_MAX_STREAMS)
               + 1
#endif
               + (HEIMDALL_H2_INIT_WINDOW != HEIMDALL_H2_DEFAULT_INIT_WINDOW)
               + (HEIMDALL_H2_MAX_FRAME != HEIMDALL_H2_DEFAULT_MAX_FRAME)
#if defined(HEIMDALL_H2_MAX_HEADER_LIST)
               + 1
#endif
    ;
    struct heimdall_h2_frame *f = heimdall_h2_frame_alloc(HEIMDALL_H2_FRAME_SETTINGS, 0, 0,
                                                n * 6);
    if (unlikely(f == NULL))
        return NULL;

    uint8_t *p = heimdall_h2_frame_payload(f);

#if (HEIMDALL_H2_MAX_HEADER_TABLE != HEIMDALL_H2_DEFAULT_MAX_HEADER_TABLE)
    SetWBE(p, HEIMDALL_H2_SETTING_HEADER_TABLE_SIZE);
    SetDWBE(p + 2, HEIMDALL_H2_MAX_HEADER_TABLE);
    p += 6;
#endif

    SetWBE(p, HEIMDALL_H2_SETTING_ENABLE_PUSH);
    SetDWBE(p + 2, 0);
    p += 6;

#if defined(HEIMDALL_H2_MAX_STREAMS)
    SetWBE(p, HEIMDALL_H2_SETTING_MAX_CONCURRENT_STREAMS);
    SetDWBE(p + 2, HEIMDALL_H2_MAX_STREAMS);
    p += 6;
#endif

#if (HEIMDALL_H2_INIT_WINDOW != HEIMDALL_H2_DEFAULT_INIT_WINDOW)
    static_assert (HEIMDALL_H2_INIT_WINDOW < 2147483648,
                   "Illegal initial window value");
    SetWBE(p, HEIMDALL_H2_SETTING_INITIAL_WINDOW_SIZE);
    SetDWBE(p + 2, HEIMDALL_H2_INIT_WINDOW);
    p += 6;
#endif

#if (HEIMDALL_H2_MAX_FRAME != HEIMDALL_H2_DEFAULT_MAX_FRAME)
    static_assert (HEIMDALL_H2_MAX_FRAME >= 16384 && HEIMDALL_H2_MAX_FRAME < 16777216,
                   "Illegal maximum frame size");
    SetWBE(p, HEIMDALL_H2_SETTING_MAX_FRAME_SIZE);
    SetDWBE(p + 2, HEIMDALL_H2_MAX_FRAME);
    p += 6;
#endif

#if defined(HEIMDALL_H2_MAX_HEADER_LIST)
    SetWBE(p, HEIMDALL_H2_SETTING_MAX_HEADER_LIST_SIZE);
    SetDWBE(p + 2, HEIMDALL_H2_MAX_HEADER_LIST);
    p += 6;
#endif

    return f;
}

struct heimdall_h2_frame *heimdall_h2_frame_settings_ack(void)
{
    return heimdall_h2_frame_alloc(HEIMDALL_H2_FRAME_SETTINGS, HEIMDALL_H2_SETTINGS_ACK, 0,
                              0);
}

const char *heimdall_h2_setting_name(uint_fast16_t id)
{
    static const char names[][20] = {
        [0]                                     = "Unknown setting",
        [HEIMDALL_H2_SETTING_HEADER_TABLE_SIZE]      = "Header table size",
        [HEIMDALL_H2_SETTING_ENABLE_PUSH]            = "Enable push",
        [HEIMDALL_H2_SETTING_MAX_CONCURRENT_STREAMS] = "Concurrent streams",
        [HEIMDALL_H2_SETTING_INITIAL_WINDOW_SIZE]    = "Initial window size",
        [HEIMDALL_H2_SETTING_MAX_FRAME_SIZE]         = "Frame size",
        [HEIMDALL_H2_SETTING_MAX_HEADER_LIST_SIZE]   = "Header list size",
    };

    if (id >= ARRAY_SIZE(names) || names[id][0] == '\0')
        id = 0;
    return names[id];
}

struct heimdall_h2_frame *heimdall_h2_frame_ping(uint64_t opaque)
{
    struct heimdall_h2_frame *f = heimdall_h2_frame_alloc(HEIMDALL_H2_FRAME_PING, 0, 0, 8);
    if (likely(f != NULL))
        memcpy(heimdall_h2_frame_payload(f), &opaque, 8);
    return f;
}

struct heimdall_h2_frame *heimdall_h2_frame_pong(uint64_t opaque)
{
    struct heimdall_h2_frame *f = heimdall_h2_frame_alloc(HEIMDALL_H2_FRAME_PING,
                                                HEIMDALL_H2_PING_ACK, 0, 8);
    if (likely(f != NULL))
        memcpy(heimdall_h2_frame_payload(f), &opaque, 8);
    return f;
}

struct heimdall_h2_frame *
heimdall_h2_frame_goaway(uint_fast32_t last_stream_id, uint_fast32_t error_code)
{
    struct heimdall_h2_frame *f = heimdall_h2_frame_alloc(HEIMDALL_H2_FRAME_GOAWAY, 0, 0, 8);
    if (likely(f != NULL))
    {
        uint8_t *p = heimdall_h2_frame_payload(f);

        SetDWBE(p, last_stream_id);
        SetDWBE(p + 4, error_code);
    }
    return f;
}

struct heimdall_h2_frame *
heimdall_h2_frame_window_update(uint_fast32_t stream_id, uint_fast32_t credit)
{
    assert((stream_id >> 31) == 0);

    struct heimdall_h2_frame *f = heimdall_h2_frame_alloc(HEIMDALL_H2_FRAME_WINDOW_UPDATE,
                                                0, stream_id, 4);
    if (likely(f != NULL))
    {
        uint8_t *p = heimdall_h2_frame_payload(f);

        SetDWBE(p, credit);
    }
    return f;
}

const char *heimdall_h2_strerror(uint_fast32_t code)
{
    static const char names[][20] = {
        [HEIMDALL_H2_NO_ERROR]            = "No error",
        [HEIMDALL_H2_PROTOCOL_ERROR]      = "Protocol error",
        [HEIMDALL_H2_INTERNAL_ERROR]      = "Internal error",
        [HEIMDALL_H2_FLOW_CONTROL_ERROR]  = "Flow control error",
        [HEIMDALL_H2_SETTINGS_TIMEOUT]    = "Settings time-out",
        [HEIMDALL_H2_STREAM_CLOSED]       = "Stream closed",
        [HEIMDALL_H2_FRAME_SIZE_ERROR]    = "Frame size error",
        [HEIMDALL_H2_REFUSED_STREAM]      = "Refused stream",
        [HEIMDALL_H2_CANCEL]              = "Cancellation",
        [HEIMDALL_H2_COMPRESSION_ERROR]   = "Compression error",
        [HEIMDALL_H2_CONNECT_ERROR]       = "CONNECT error",
        [HEIMDALL_H2_ENHANCE_YOUR_CALM]   = "Excessive load",
        [HEIMDALL_H2_INADEQUATE_SECURITY] = "Inadequate security",
        [HEIMDALL_H2_HTTP_1_1_REQUIRED]   = "Required HTTP/1.1",
    };

    if (code >= ARRAY_SIZE(names) || names[code][0] == '\0')
        return "Unknown error";
    return names[code];
}

void heimdall_h2_frame_dump(void *opaque, const struct heimdall_h2_frame *f,
                       const char *msg)
{
    size_t len = heimdall_h2_frame_length(f);
    uint_fast8_t type = heimdall_h2_frame_type(f);
    uint_fast8_t flags = heimdall_h2_frame_flags(f);
    uint_fast32_t sid = heimdall_h2_frame_id(f);

    if (sid != 0)
        heimdall_http_dbg(opaque, "%s %s (0x%02"PRIxFAST8") frame of %zu bytes, "
                     "flags 0x%02"PRIxFAST8", stream %"PRIuFAST32, msg,
                     heimdall_h2_type_name(type), type, len,  flags, sid);
    else
        heimdall_http_dbg(opaque, "%s %s (0x%02"PRIxFAST8") frame of %zu bytes, "
                     "flags 0x%02"PRIxFAST8", global", msg,
                     heimdall_h2_type_name(type), type, len,  flags);
}

const uint8_t *(heimdall_h2_frame_data_get)(const struct heimdall_h2_frame *f,
                                       size_t *restrict lenp)
{
    assert(heimdall_h2_frame_type(f) == HEIMDALL_H2_FRAME_DATA);

    size_t len = heimdall_h2_frame_length(f);
    uint_fast8_t flags = heimdall_h2_frame_flags(f);
    const uint8_t *ptr = heimdall_h2_frame_payload(f);

    /* At this point, the frame has already been validated by the parser. */
    if (flags & HEIMDALL_H2_DATA_PADDED)
    {
        assert(len >= 1u && len >= 1u + ptr[0]);
        len -= 1u + *(ptr++);
    }

    *lenp = len;
    return ptr;
}

typedef int (*heimdall_h2_parser)(struct heimdall_h2_parser *, struct heimdall_h2_frame *,
                             size_t, uint_fast32_t);

/** HTTP/2 incoming frames parser */
struct heimdall_h2_parser
{
    void *opaque;
    const struct heimdall_h2_parser_cbs *cbs;

    heimdall_h2_parser parser; /*< Parser state / callback for next frame */

    struct
    {
        uint32_t sid; /*< Ongoing stream identifier */
        bool eos; /*< End of stream after headers block */
        size_t len; /*< Compressed headers buffer length */
        uint8_t *buf; /*< Compressed headers buffer base address */
        struct hpack_decoder *decoder; /*< HPACK decompressor state */
    } headers; /*< Compressed headers reception state */

    uint32_t rcwd_size; /*< Receive congestion window (bytes) */
};

static int heimdall_h2_parse_generic(struct heimdall_h2_parser *, struct heimdall_h2_frame *,
                                size_t, uint_fast32_t);
static int heimdall_h2_parse_headers_block(struct heimdall_h2_parser *,
                                      struct heimdall_h2_frame *, size_t,
                                      uint_fast32_t);

static int heimdall_h2_parse_error(struct heimdall_h2_parser *p, uint_fast32_t code)
{
    p->cbs->error(p->opaque, code);
    return -1;
}

static int heimdall_h2_stream_error(struct heimdall_h2_parser *p, uint_fast32_t id,
                               uint_fast32_t code)
{
    return p->cbs->stream_error(p->opaque, id, code);
}

static void *heimdall_h2_stream_lookup(struct heimdall_h2_parser *p, uint_fast32_t id)
{
    return p->cbs->stream_lookup(p->opaque, id);
}

static void heimdall_h2_parse_headers_start(struct heimdall_h2_parser *p,
                                       uint_fast32_t sid, bool eos)
{
    assert(sid != 0);
    assert(p->headers.sid == 0);

    p->parser = heimdall_h2_parse_headers_block;
    p->headers.sid = sid;
    p->headers.eos = eos;
    p->headers.len = 0;
}

static int heimdall_h2_parse_headers_append(struct heimdall_h2_parser *p,
                                       const uint8_t *data, size_t len)
{
    assert(p->headers.sid != 0);

    if (p->headers.len + len > 65536)
        return heimdall_h2_parse_error(p, HEIMDALL_H2_INTERNAL_ERROR);

    uint8_t *buf = realloc(p->headers.buf, p->headers.len + len);
    if (unlikely(buf == NULL))
        return heimdall_h2_parse_error(p, HEIMDALL_H2_INTERNAL_ERROR);

    p->headers.buf = buf;
    memcpy(p->headers.buf + p->headers.len, data, len);
    p->headers.len += len;
    return 0;
}

static int heimdall_h2_parse_headers_end(struct heimdall_h2_parser *p)
{
    char *headers[HEIMDALL_H2_MAX_HEADERS][2];

    /* TODO: limit total decompressed size of the headers list */
    int n = hpack_decode(p->headers.decoder, p->headers.buf, p->headers.len,
                         headers, HEIMDALL_H2_MAX_HEADERS);
    if (n > HEIMDALL_H2_MAX_HEADERS)
    {
        for (unsigned i = 0; i < HEIMDALL_H2_MAX_HEADERS; i++)
        {
            free(headers[i][0]);
            free(headers[i][1]);
        }
        n = -1;
    }
    if (n < 0)
        return heimdall_h2_parse_error(p, HEIMDALL_H2_COMPRESSION_ERROR);

    void *s = heimdall_h2_stream_lookup(p, p->headers.sid);
    int val = 0;

    if (s != NULL)
    {
        const char *ch[HEIMDALL_H2_MAX_HEADERS][2];

        for (int i = 0; i < n; i++)
            ch[i][0] = headers[i][0], ch[i][1] = headers[i][1];

        p->cbs->stream_headers(s, n, ch);

        if (p->headers.eos)
            p->cbs->stream_end(s);
    }
    else
        /* NOTE: The specification implies that the error should be sent for
         * the first header frame. But we actually want to receive the whole
         * fragmented headers block, to preserve the HPACK decoder state.
         * So we send the error at the last header frame instead. */
        val = heimdall_h2_stream_error(p, p->headers.sid, HEIMDALL_H2_REFUSED_STREAM);

    for (int i = 0; i < n; i++)
    {
        free(headers[i][0]);
        free(headers[i][1]);
    }

    p->parser = heimdall_h2_parse_generic;
    p->headers.sid = 0;
    return val;
}

/** Parses an HTTP/2 DATA frame */
static int heimdall_h2_parse_frame_data(struct heimdall_h2_parser *p,
                                   struct heimdall_h2_frame *f, size_t len,
                                   uint_fast32_t id)
{
    uint_fast8_t flags = heimdall_h2_frame_flags(f);
    const uint8_t *ptr = heimdall_h2_frame_payload(f);

    if (id == 0)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    if (len > HEIMDALL_H2_MAX_FRAME)
    {
        free(f);
        return heimdall_h2_stream_error(p, id, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    if (flags & HEIMDALL_H2_DATA_PADDED)
    {
        if (len < 1 || len < (1u + ptr[0]))
        {
            free(f);
            return heimdall_h2_stream_error(p, id, HEIMDALL_H2_FRAME_SIZE_ERROR);
        }
        len -= 1 + ptr[0];
    }

    if (len > p->rcwd_size)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_FLOW_CONTROL_ERROR);
    }

    p->rcwd_size -= len;
    p->cbs->window_status(p->opaque, &p->rcwd_size);

    void *s = heimdall_h2_stream_lookup(p, id);
    if (s == NULL)
    {
        free(f);
        return heimdall_h2_stream_error(p, id, HEIMDALL_H2_STREAM_CLOSED);
    }

    int ret = p->cbs->stream_data(s, f);
    /* Frame gets consumed here ^^ */

    if (flags & HEIMDALL_H2_DATA_END_STREAM)
        p->cbs->stream_end(s);
    return ret;
}

/** Parses an HTTP/2 HEADERS frame */
static int heimdall_h2_parse_frame_headers(struct heimdall_h2_parser *p,
                                      struct heimdall_h2_frame *f, size_t len,
                                      uint_fast32_t id)
{
    uint_fast8_t flags = heimdall_h2_frame_flags(f);
    const uint8_t *ptr = heimdall_h2_frame_payload(f);

    if (id == 0)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    if (len > HEIMDALL_H2_MAX_FRAME)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    if (flags & HEIMDALL_H2_HEADERS_PADDED)
    {
        if (len < 1 || len < (1u + ptr[0]))
        {
            free(f);
            return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
        }
        len -= 1 + ptr[0];
        ptr++;
    }

    if (flags & HEIMDALL_H2_HEADERS_PRIORITY)
    {   /* Ignore priorities for now as we do not upload anything. */
        if (len < 5)
        {
            free(f);
            return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
        }
        ptr += 5;
        len -= 5;
    }

    heimdall_h2_parse_headers_start(p, id, flags & HEIMDALL_H2_HEADERS_END_STREAM);

    int ret = heimdall_h2_parse_headers_append(p, ptr, len);

    if (ret == 0 && (flags & HEIMDALL_H2_HEADERS_END_HEADERS))
        ret = heimdall_h2_parse_headers_end(p);

    free(f);
    return ret;
}

/** Parses an HTTP/2 PRIORITY frame */
static int heimdall_h2_parse_frame_priority(struct heimdall_h2_parser *p,
                                       struct heimdall_h2_frame *f, size_t len,
                                       uint_fast32_t id)
{
    free(f);

    if (id == 0)
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);

    if (len != 5)
        return heimdall_h2_stream_error(p, id, HEIMDALL_H2_FRAME_SIZE_ERROR);

    /* Ignore priorities for now as we do not upload much. */
    return 0;
}

/** Parses an HTTP/2 RST_STREAM frame */
static int heimdall_h2_parse_frame_rst_stream(struct heimdall_h2_parser *p,
                                         struct heimdall_h2_frame *f, size_t len,
                                         uint_fast32_t id)
{
    if (id == 0)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    if (len != 4)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    void *s = heimdall_h2_stream_lookup(p, id);
    uint_fast32_t code = GetDWBE(heimdall_h2_frame_payload(f));

    free(f);

    if (s == NULL)
        return 0;
    return p->cbs->stream_reset(s, code);
}

/** Parses an HTTP/2 SETTINGS frame */
static int heimdall_h2_parse_frame_settings(struct heimdall_h2_parser *p,
                                       struct heimdall_h2_frame *f, size_t len,
                                       uint_fast32_t id)
{
    const uint8_t *ptr = heimdall_h2_frame_payload(f);

    if (id != 0)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    if (len % 6 || len > HEIMDALL_H2_MAX_FRAME)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    if (heimdall_h2_frame_flags(f) & HEIMDALL_H2_SETTINGS_ACK)
    {
        free(f);
        if (len != 0)
            return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
        /* Ignore ACKs for now as we never change settings. */
        return 0;
    }

    for (const uint8_t *end = ptr + len; ptr < end; ptr += 6)
        p->cbs->setting(p->opaque, GetWBE(ptr), GetDWBE(ptr + 2));

    free(f);
    return p->cbs->settings_done(p->opaque);
}

/** Parses an HTTP/2 PUSH_PROMISE frame */
static int heimdall_h2_parse_frame_push_promise(struct heimdall_h2_parser *p,
                                           struct heimdall_h2_frame *f, size_t len,
                                           uint_fast32_t id)
{
    uint8_t flags = heimdall_h2_frame_flags(f);
    const uint8_t *ptr = heimdall_h2_frame_payload(f);

    if (id == 0)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    if (len > HEIMDALL_H2_MAX_FRAME)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    if (flags & HEIMDALL_H2_PUSH_PROMISE_PADDED)
    {
        if (len < 1 || len < (1u + ptr[0]))
        {
            free(f);
            return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
        }
        len -= 1 + ptr[0];
        ptr++;
    }

    /* Not permitted by our settings. */
    free(f);
    return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
}

/** Parses an HTTP/2 PING frame */
static int heimdall_h2_parse_frame_ping(struct heimdall_h2_parser *p,
                                   struct heimdall_h2_frame *f, size_t len,
                                   uint_fast32_t id)
{
    uint64_t opaque;

    if (id != 0)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    if (len != 8)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    if (heimdall_h2_frame_flags(f) & HEIMDALL_H2_PING_ACK)
    {
        free(f);
        return 0;
    }

    memcpy(&opaque, heimdall_h2_frame_payload(f), 8);
    free(f);

    return p->cbs->ping(p->opaque, opaque);
}

/** Parses an HTTP/2 GOAWAY frame */
static int heimdall_h2_parse_frame_goaway(struct heimdall_h2_parser *p,
                                     struct heimdall_h2_frame *f, size_t len,
                                     uint_fast32_t id)
{
    const uint8_t *ptr = heimdall_h2_frame_payload(f);

    if (id != 0)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    if (len < 8 || len > HEIMDALL_H2_MAX_FRAME)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    uint_fast32_t last_id = GetDWBE(ptr) & 0x7FFFFFFF;
    uint_fast32_t code = GetDWBE(ptr + 4);

    free(f);
    return p->cbs->reset(p->opaque, last_id, code);
}

/** Parses an HTTP/2 WINDOW_UPDATE frame */
static int heimdall_h2_parse_frame_window_update(struct heimdall_h2_parser *p,
                                            struct heimdall_h2_frame *f, size_t len,
                                            uint_fast32_t id)
{
    if (len != 4)
    {
        free(f);

        if (id == 0)
            return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
        return heimdall_h2_stream_error(p, id, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    uint_fast32_t credit = GetDWBE(heimdall_h2_frame_payload(f)) & 0x7fffffffu;

    free(f);

    if (credit == 0)
    {
        if (id == 0)
            return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
        return heimdall_h2_stream_error(p, id, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    if (id == 0)
        p->cbs->window_update(p->opaque, credit);
    else
    {
        void *s = heimdall_h2_stream_lookup(p, id);

        if (s != NULL)
            p->cbs->stream_window_update(s, credit);
    }
    return 0;
}

/** Parses an HTTP/2 CONTINUATION frame */
static int heimdall_h2_parse_frame_continuation(struct heimdall_h2_parser *p,
                                           struct heimdall_h2_frame *f, size_t len,
                                           uint_fast32_t id)
{
    const uint8_t *ptr = heimdall_h2_frame_payload(f);

    /* Stream ID must match with the previous frame. */
    if (id == 0 || id != p->headers.sid)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    if (len > HEIMDALL_H2_MAX_FRAME)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    int ret = heimdall_h2_parse_headers_append(p, ptr, len);

    if (ret == 0 && (heimdall_h2_frame_flags(f) & HEIMDALL_H2_CONTINUATION_END_HEADERS))
        ret = heimdall_h2_parse_headers_end(p);

    free(f);
    return 0;
}

/** Parses an HTTP/2 frame of unknown type */
static int heimdall_h2_parse_frame_unknown(struct heimdall_h2_parser *p,
                                      struct heimdall_h2_frame *f, size_t len,
                                      uint_fast32_t id)
{
    free(f);

    if (len > HEIMDALL_H2_MAX_FRAME)
    {
        if (id == 0)
            return heimdall_h2_parse_error(p, HEIMDALL_H2_FRAME_SIZE_ERROR);
        return heimdall_h2_stream_error(p, id, HEIMDALL_H2_FRAME_SIZE_ERROR);
    }

    /* Ignore frames of unknown type as specified. */
    return 0;
}

static const heimdall_h2_parser heimdall_h2_parsers[] = {
    [HEIMDALL_H2_FRAME_DATA]          = heimdall_h2_parse_frame_data,
    [HEIMDALL_H2_FRAME_HEADERS]       = heimdall_h2_parse_frame_headers,
    [HEIMDALL_H2_FRAME_PRIORITY]      = heimdall_h2_parse_frame_priority,
    [HEIMDALL_H2_FRAME_RST_STREAM]    = heimdall_h2_parse_frame_rst_stream,
    [HEIMDALL_H2_FRAME_SETTINGS]      = heimdall_h2_parse_frame_settings,
    [HEIMDALL_H2_FRAME_PUSH_PROMISE]  = heimdall_h2_parse_frame_push_promise,
    [HEIMDALL_H2_FRAME_PING]          = heimdall_h2_parse_frame_ping,
    [HEIMDALL_H2_FRAME_GOAWAY]        = heimdall_h2_parse_frame_goaway,
    [HEIMDALL_H2_FRAME_WINDOW_UPDATE] = heimdall_h2_parse_frame_window_update,
    [HEIMDALL_H2_FRAME_CONTINUATION]  = heimdall_h2_parse_frame_continuation,
};

/** Parses the HTTP/2 connection preface. */
static int heimdall_h2_parse_preface(struct heimdall_h2_parser *p,
                                struct heimdall_h2_frame *f, size_t len,
                                uint_fast32_t id)
{
    /* The length must be within the specification default limits. */
    if (len > HEIMDALL_H2_DEFAULT_MAX_FRAME
    /* The type must SETTINGS. */
     || heimdall_h2_frame_type(f) != HEIMDALL_H2_FRAME_SETTINGS
    /* The SETTINGS ACK flag must be clear. */
     || (heimdall_h2_frame_flags(f) & HEIMDALL_H2_SETTINGS_ACK))
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    p->parser = heimdall_h2_parse_generic;

    return heimdall_h2_parse_frame_settings(p, f, len, id);
}

/** Parses any HTTP/2 frame. */
static int heimdall_h2_parse_generic(struct heimdall_h2_parser *p,
                                struct heimdall_h2_frame *f, size_t len,
                                uint_fast32_t id)
{
    uint_fast8_t type = heimdall_h2_frame_type(f);
    heimdall_h2_parser func = heimdall_h2_parse_frame_unknown;

    assert(p->headers.sid == 0);

    if (type < ARRAY_SIZE(heimdall_h2_parsers) && heimdall_h2_parsers[type] != NULL)
        func = heimdall_h2_parsers[type];

    return func(p, f, len, id);
}

static int heimdall_h2_parse_headers_block(struct heimdall_h2_parser *p,
                                      struct heimdall_h2_frame *f, size_t len,
                                      uint_fast32_t id)
{
    assert(p->headers.sid != 0);

    /* After a HEADER, PUSH_PROMISE of CONTINUATION frame without the
     * END_HEADERS flag, must come a CONTINUATION frame. */
    if (heimdall_h2_frame_type(f) != HEIMDALL_H2_FRAME_CONTINUATION)
    {
        free(f);
        return heimdall_h2_parse_error(p, HEIMDALL_H2_PROTOCOL_ERROR);
    }

    return heimdall_h2_parse_frame_continuation(p, f, len, id);
}

static int heimdall_h2_parse_failed(struct heimdall_h2_parser *p, struct heimdall_h2_frame *f,
                               size_t len, uint_fast32_t id)
{
    free(f);
    (void) p; (void) len; (void) id;
    return -1;
}

int heimdall_h2_parse(struct heimdall_h2_parser *p, struct heimdall_h2_frame *f)
{
    int ret = 0;

    while (f != NULL)
    {
        struct heimdall_h2_frame *next = f->next;
        size_t len = heimdall_h2_frame_length(f);
        uint_fast32_t id = heimdall_h2_frame_id(f);

        f->next = NULL;
        ret = p->parser(p, f, len, id);
        if (ret)
            p->parser = heimdall_h2_parse_failed;
        f = next;
    }

    return ret;
}

struct heimdall_h2_parser *heimdall_h2_parse_init(void *ctx,
                                        const struct heimdall_h2_parser_cbs *cbs)
{
    struct heimdall_h2_parser *p = malloc(sizeof (*p));
    if (unlikely(p == NULL))
        return NULL;

    p->opaque = ctx;
    p->cbs = cbs;
    p->parser = heimdall_h2_parse_preface;
    p->headers.sid = 0;
    p->headers.buf = NULL;
    p->headers.len = 0;
    p->headers.decoder = hpack_decode_init(HEIMDALL_H2_MAX_HEADER_TABLE);
    if (unlikely(p->headers.decoder == NULL))
    {
        free(p);
        return NULL;
    }
    p->rcwd_size = 65535; /* initial per-connection value */
    return p;
}

void heimdall_h2_parse_destroy(struct heimdall_h2_parser *p)
{
    hpack_decode_destroy(p->headers.decoder);
    free(p->headers.buf);
    free(p);
}
