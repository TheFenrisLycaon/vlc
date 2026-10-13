/*****************************************************************************
 * h2conn_test.c: HTTP/2 connection tests
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

#undef NDEBUG

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <heimdall_common.h>
#include <heimdall_block.h>
#include <heimdall_tls.h>
#include "h2frame.h"
#include "conn.h"
#include "message.h"

#if defined(PF_UNIX) && !defined(PF_LOCAL)
#    define PF_LOCAL PF_UNIX
#endif

const char heimdall_module_name[] = "test_h2conn";

static struct heimdall_http_conn *conn;
static struct heimdall_tls *external_tls;

static void conn_send(struct heimdall_h2_frame *f)
{
    assert(f != NULL);

    size_t len = heimdall_h2_frame_size(f);
    ssize_t val = heimdall_tls_Write(external_tls, f->data, len);
    assert((size_t)val == len);
    free(f);
}

enum {
    DATA, HEADERS, PRIORITY, RST_STREAM, SETTINGS, PUSH_PROMISE, PING, GOAWAY,
    WINDOW_UPDATE, CONTINUATION,
};

static void conn_expect(uint_fast8_t wanted)
{
    size_t len;
    ssize_t val;
    uint8_t hdr[9];
    uint8_t got;

    do {
        val = heimdall_tls_Read(external_tls, hdr, 9, true);
        assert(val == 9);
        assert(hdr[0] == 0);

        /* Check type. We do not currently validate WINDOW_UPDATE. */
        got = hdr[3];
        assert(wanted == got || WINDOW_UPDATE == got);

        len = (hdr[1] << 8) | hdr[2];
        if (len > 0)
        {
            char buf[len];

            val = heimdall_tls_Read(external_tls, buf, len, true);
            assert(val == (ssize_t)len);
        }
    }
    while (got != wanted);
}

static void conn_create(void)
{
    ssize_t val;
    heimdall_tls_t *tlsv[2];
    char hello[24];

    if (heimdall_tls_SocketPair(PF_LOCAL, 0, tlsv))
        assert(!"socketpair");

    external_tls = tlsv[0];

    conn = heimdall_h2_conn_create(NULL, tlsv[1]);
    assert(conn != NULL);
    conn_send(heimdall_h2_frame_settings());

    val = heimdall_tls_Read(external_tls, hello, 24, true);
    assert(val == 24);
    assert(!memcmp(hello, "PRI * HTTP/2.0\r\n", 16));
    conn_expect(SETTINGS);
    conn_expect(SETTINGS);
}

static void conn_destroy(void)
{
    heimdall_tls_Shutdown(external_tls, false);
    heimdall_http_conn_release(conn);
    heimdall_tls_SessionDelete(external_tls);
}

static struct heimdall_http_stream *stream_open(bool has_data)
{
    const char *verb = has_data ? "POST" : "GET";
    struct heimdall_http_msg *m = heimdall_http_req_create(verb, "https",
                                                 "www.example.com", "/");
    assert(m != NULL);

    struct heimdall_http_stream *s = heimdall_http_stream_open(conn, m, has_data);
    heimdall_http_msg_destroy(m);
    return s;
}

static void stream_reply(uint_fast32_t id, bool nodata)
{
    struct heimdall_http_msg *m = heimdall_http_resp_create(200);
    assert(m != NULL);
    heimdall_http_msg_add_agent(m, "Heimdall-h2-tester");

    conn_send(heimdall_http_msg_h2_frame(m, id, nodata));
    heimdall_http_msg_destroy(m);
}

static void stream_continuation(uint_fast32_t id)
{
    const char *h[][2] = {
        { ":status", "100" },
    };

    conn_send(heimdall_h2_frame_headers(id, HEIMDALL_H2_DEFAULT_MAX_FRAME, false, 1, h));
}

static void stream_data(uint_fast32_t id, const char *str, bool eos)
{
    conn_send(heimdall_h2_frame_data(id, str, strlen(str), eos));
}

/* TODO: check messages coming from the connection under test */

int main(void)
{
    struct heimdall_http_stream *s, *s2;
    struct heimdall_http_msg *m;
    block_t *b;
    uint_fast32_t sid = -1; /* Second guessed stream IDs :-/ */

    conn_create();
    conn_destroy();

    conn_create();
    conn_send(heimdall_h2_frame_ping(42));
    conn_expect(PING);

    /* Test rejected stream */
    sid += 2;
    s = stream_open(false);
    assert(s != NULL);
    conn_expect(HEADERS);
    conn_send(heimdall_h2_frame_rst_stream(sid, HEIMDALL_H2_REFUSED_STREAM));
    m = heimdall_http_stream_read_headers(s);
    assert(m == NULL);
    b = heimdall_http_stream_read(s);
    assert(b == heimdall_http_error);
    heimdall_http_stream_close(s, false);
    conn_expect(RST_STREAM);

    /* Test accepted stream */
    sid += 2;
    s = stream_open(false);
    assert(s != NULL);
    stream_reply(sid, false);
    m = heimdall_http_msg_get_initial(s);
    assert(m != NULL);
    heimdall_http_msg_destroy(m);

    stream_data(3, "Hello ", false); /* late data */
    stream_data(3, "world!", true);

    conn_expect(HEADERS);
    conn_expect(RST_STREAM);
    conn_expect(RST_STREAM);
    conn_expect(RST_STREAM);

    /* Test continuation then accepted stream */
    sid += 2;
    s = stream_open(false);
    assert(s != NULL);
    stream_continuation(sid);
    m = heimdall_http_msg_get_initial(s);
    assert(m != NULL);
    assert(heimdall_http_msg_get_status(m) == 100);
    stream_reply(sid, false);
    m = heimdall_http_msg_iterate(m);
    assert(m != NULL);
    stream_data(sid, "Hello ", false);
    stream_data(sid, "world!", true);
    stream_data(sid, "Stray message", false); /* data after EOS */
    b = heimdall_http_msg_read(m);
    assert(b != NULL);
    block_Release(b);
    b = heimdall_http_msg_read(m);
    assert(b != NULL);
    block_Release(b);
    b = heimdall_http_msg_read(m);
    assert(b == NULL);
    heimdall_http_msg_destroy(m);

    conn_expect(HEADERS);
    conn_expect(RST_STREAM);
    conn_expect(RST_STREAM);

    /* Test accepted stream after continuation */
    sid += 2;
    s = stream_open(false);
    assert(s != NULL);
    stream_continuation(sid);
    stream_reply(sid, true);
    sid += 2;
    s2 = stream_open(false); /* 2nd stream to enforce test timing/ordering */
    assert(s2 != NULL);
    stream_reply(sid, true);
    m = heimdall_http_msg_get_initial(s2);
    assert(m != NULL);
    heimdall_http_msg_destroy(m);
    m = heimdall_http_msg_get_initial(s);
    assert(m != NULL);
    assert(heimdall_http_msg_get_status(m) == 200);
    b = heimdall_http_msg_read(m);
    assert(b == NULL);
    heimdall_http_msg_destroy(m);

    conn_expect(HEADERS);
    conn_expect(HEADERS);
    conn_expect(RST_STREAM);
    conn_expect(RST_STREAM);

    /* Test stream with request payload */
    sid += 2;
    s = stream_open(true);
    assert(s != NULL);
    stream_continuation(sid);
    m = heimdall_http_msg_get_initial(s);
    assert(m != NULL);
    b = block_Alloc(12);
    assert(b != NULL);
    memcpy(b->p_buffer, "Hello world!", 12);
    assert(heimdall_http_msg_write(m, b, false) == 0);
    assert(heimdall_http_msg_write(m, NULL, true) == 0);
    stream_reply(sid, false);
    m = heimdall_http_msg_iterate(m);
    assert(m != NULL);
    heimdall_http_msg_destroy(m);

    conn_expect(HEADERS);
    conn_expect(DATA);
    conn_expect(DATA);
    conn_expect(RST_STREAM);

    /* Test nonexistent stream reset */
    conn_send(heimdall_h2_frame_rst_stream(sid + 100, HEIMDALL_H2_REFUSED_STREAM));

    /* Test multiple streams in non-LIFO order */
    sid += 2;
    s = stream_open(false);
    assert(s != NULL);
    sid += 2;
    s2 = stream_open(false);
    assert(s2 != NULL);
    stream_reply(sid, false);
    stream_reply(sid - 2, true);
    stream_data(sid, "Discarded", false); /* not read data */
    m = heimdall_http_msg_get_initial(s);
    assert(m != NULL);
    heimdall_http_msg_destroy(m);
    m = heimdall_http_msg_get_initial(s2);
    assert(m != NULL);
    heimdall_http_msg_destroy(m);

    conn_expect(HEADERS);
    conn_expect(HEADERS);
    conn_expect(RST_STREAM);
    conn_expect(RST_STREAM);
    /* might or might not seen one or two extra RST_STREAM now */

    /* Test graceful connection termination */
    sid += 2;
    s = stream_open(false);
    assert(s != NULL);
    conn_send(heimdall_h2_frame_goaway(sid - 2, HEIMDALL_H2_NO_ERROR));
    m = heimdall_http_stream_read_headers(s);
    assert(m == NULL);

    /* Test stream after connection shut down */
    assert(stream_open(false) == NULL);

    /* Test releasing connection before stream */
    conn_destroy();
    heimdall_http_stream_close(s, false);

    return 0;
}
