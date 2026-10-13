/*****************************************************************************
 * message_test.c: HTTP request/response test
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
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <heimdall_common.h>
#include "message.h"
#include "h2frame.h"

const char heimdall_module_name[] = "test_http_msg";

static void check_req(const struct heimdall_http_msg *m)
{
    const char *str;

    assert(heimdall_http_msg_get_status(m) < 0);
    str = heimdall_http_msg_get_method(m);
    assert(str != NULL && !strcmp(str, "GET"));
    str = heimdall_http_msg_get_scheme(m);
    assert(str != NULL && !strcmp(str, "http"));
    str = heimdall_http_msg_get_authority(m);
    assert(str != NULL && !strcmp(str, "www.example.com"));
    str = heimdall_http_msg_get_path(m);
    assert(str != NULL && !strcmp(str, "/"));
    assert(heimdall_http_msg_get_size(m) == 0);

    str = heimdall_http_msg_get_header(m, "Cache-Control");
    assert(str != NULL && !strcmp(str, "no-cache"));
    str = heimdall_http_msg_get_header(m, "Custom-Key");
    assert(str != NULL && !strcmp(str, "custom-value"));

    str = heimdall_http_msg_get_header(m, "Date");
    assert(str == NULL);
}

static void check_resp(const struct heimdall_http_msg *m)
{
    const char *str;

    assert(heimdall_http_msg_get_status(m) == 200);
    str = heimdall_http_msg_get_method(m);
    assert(str == NULL);
    str = heimdall_http_msg_get_scheme(m);
    assert(str == NULL);
    str = heimdall_http_msg_get_authority(m);
    assert(str == NULL);
    str = heimdall_http_msg_get_path(m);
    assert(str == NULL);

    str = heimdall_http_msg_get_header(m, "Cache-Control");
    assert(str != NULL && !strcmp(str, "private"));
    str = heimdall_http_msg_get_header(m, "Date");
    assert(str != NULL && !strcmp(str, "Mon, 21 Oct 2013 20:13:22 GMT"));
    str = heimdall_http_msg_get_header(m, "Location");
    assert(str != NULL && !strcmp(str, "https://www.example.com"));
    str = heimdall_http_msg_get_header(m, "Content-Encoding");
    assert(str != NULL && !strcmp(str, "gzip"));
    str = heimdall_http_msg_get_header(m, "Set-Cookie");
    assert(str != NULL && !strcmp(str, "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; "
                                  "max-age=3600; version=1"));

    str = heimdall_http_msg_get_header(m, "Custom-Key");
    assert(str == NULL);
}

static void check_connect(const struct heimdall_http_msg *m)
{
    const char *str;

    assert(heimdall_http_msg_get_status(m) < 0);
    str = heimdall_http_msg_get_method(m);
    assert(str != NULL && !strcmp(str, "CONNECT"));
    str = heimdall_http_msg_get_scheme(m);
    assert(str == NULL);
    str = heimdall_http_msg_get_authority(m);
    assert(str != NULL && !strcmp(str, "www.example.com"));
    str = heimdall_http_msg_get_path(m);
    assert(str == NULL);

    str = heimdall_http_msg_get_header(m, "Custom-Key");
    assert(str == NULL);
}

static void check_msg(struct heimdall_http_msg *in,
                      void (*cb)(const struct heimdall_http_msg *))
{
    struct heimdall_http_msg *out;
    char *m1;
    size_t len;

    cb(in);

    m1 = heimdall_http_msg_format(in, &len, false, true);
    assert(m1 != NULL);
    assert(strlen(m1) == len);
    out = heimdall_http_msg_headers(m1);
    fprintf(stderr, "%s", m1);
    free(m1);
    /* XXX: request parsing not implemented/needed yet */
    if (heimdall_http_msg_get_status(in) >= 0)
    {
        assert(out != NULL);
        cb(out);
        heimdall_http_msg_destroy(out);
    }

    out = (struct heimdall_http_msg *)heimdall_http_msg_h2_frame(in, 1, true);
    assert(out != NULL);
    cb(out);
    assert(heimdall_http_msg_read(out) == NULL);
    heimdall_http_msg_destroy(out);

    cb(in);
    heimdall_http_msg_destroy(in);
}

static time_t parse_date(const char *str)
{
    struct heimdall_http_msg *m;
    time_t t1, t2;

    m = heimdall_http_req_create("GET", "http", "www.example.com", "/");
    assert(m != NULL);
    assert(heimdall_http_msg_add_header(m, "Date", "%s", str) == 0);
    t1 = heimdall_http_msg_get_atime(m);
    assert(heimdall_http_msg_add_header(m, "Last-Modified", "%s", str) == 0);
    t2 = heimdall_http_msg_get_mtime(m);
    assert(heimdall_http_msg_get_retry_after(m) == 0);
    assert(heimdall_http_msg_add_header(m, "Retry-After", "%s", str) == 0);
    heimdall_http_msg_get_retry_after(m);
    heimdall_http_msg_destroy(m);

    assert(t1 == t2);
    return t1;
}

static const char *check_realm(const char *line, const char *realm)
{
    struct heimdall_http_msg *m;
    char *value;

    m = heimdall_http_resp_create(401);
    assert(m != NULL);
    assert(heimdall_http_msg_add_header(m, "WWW-Authenticate", "%s", line) == 0);
    value = heimdall_http_msg_get_basic_realm(m);
    if (realm == NULL)
        assert(value == NULL);
    else
    {
        assert(value != NULL);
        assert(!strcmp(value, realm));
        free(value);
    }
    heimdall_http_msg_destroy(m);
    return realm;
}

int main(void)
{
    struct heimdall_http_msg *m;
    const char *str;
    int ret;

    /* Formatting and parsing */
    m = heimdall_http_req_create("GET", "http", "www.example.com", "/");
    assert(m != NULL);
    ret = heimdall_http_msg_add_header(m, "Cache-Control", "no-cache");
    assert(ret == 0);
    heimdall_http_msg_add_header(m, "Custom-Key", "%s", "custom-value");
    assert(ret == 0);
    check_msg(m, check_req);

    m = heimdall_http_resp_create(200);
    assert(m != NULL);
    ret = heimdall_http_msg_add_header(m, "cache-control", "private");
    assert(ret == 0);
    ret = heimdall_http_msg_add_header(m, "date", "Mon, 21 Oct 2013 20:13:22 GMT");
    assert(ret == 0);
    ret = heimdall_http_msg_add_header(m, "location", "https://www.example.com");
    assert(ret == 0);
    ret = heimdall_http_msg_add_header(m, "content-encoding", "gzip");
    assert(ret == 0);
    ret = heimdall_http_msg_add_header(m, "set-cookie", "foo=%s; max-age=%u; "
                                  "version=%u", "ASDJKHQKBZXOQWEOPIUAXQWEOIU",
                                  3600, 1);
    assert(ret == 0);
    check_msg(m, check_resp);

    m = heimdall_http_req_create("CONNECT", NULL, "www.example.com", NULL);
    assert(m != NULL);
    check_msg(m, check_connect);

    /* Helpers */
    assert(parse_date("Sun, 06 Nov 1994 08:49:37 GMT") == 784111777);
    assert(parse_date("Sunday, 06-Nov-94 08:49:37 GMT") == 784111777);
    assert(parse_date("Sun Nov  6 08:49:37 1994") == 784111777);
    assert(parse_date("Sunday, 06-Nov-14 08:49:37 GMT") == 1415263777);
    assert(parse_date("Sun, 06 Bug 1994 08:49:37 GMT") == -1);
    assert(parse_date("bogus") == -1);

    assert(check_realm("Basic realm=\"kingdom\"", "kingdom"));
    assert(check_realm("BaSiC REALM= \"kingdom\"", "kingdom"));
    assert(check_realm("basic Realm\t=\"kingdom\"", "kingdom"));
    assert(check_realm("Basic charset=\"utf-8\", realm=\"kingdom\"",
                       "kingdom"));
    assert(check_realm("Basic realm=\"Realm is \\\"Hello world!\\\"\"",
                       "Realm is \"Hello world!\""));
    assert(check_realm("Basic", NULL) == NULL);

    m = heimdall_http_req_create("PRI", "https", "*", NULL);
    assert(m != NULL);

    assert(heimdall_http_msg_add_agent(m, "Foo") == 0);
    assert(heimdall_http_msg_add_agent(m, "Foo/1.0") == 0);
    assert(heimdall_http_msg_add_agent(m, "Foo/1.0 (Hello world) Bar/2.3") == 0);
    assert(heimdall_http_msg_add_agent(m, "Foo/1.0 (compatible (\\(!))") == 0);

    assert(heimdall_http_msg_add_atime(m) == 0);
    time_t t = heimdall_http_msg_get_atime(m);
    assert(t != (time_t)-1);
    assert(heimdall_http_msg_add_time(m, "Last-Modified", &t) == 0);
    assert(t == heimdall_http_msg_get_mtime(m));

    assert(heimdall_http_msg_add_creds_basic(m, false,
                                        "Aladdin", "open sesame") == 0);
    assert(heimdall_http_msg_add_creds_basic(m, true, "test", "123£") == 0);
    str = heimdall_http_msg_get_header(m, "Authorization");
    assert(str != NULL && !strcmp(str, "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="));
    str = heimdall_http_msg_get_header(m, "Proxy-Authorization");
    assert(str != NULL && !strcmp(str, "Basic dGVzdDoxMjPCow=="));

    heimdall_http_msg_add_header(m, "Content-Length", "1234");
    assert(heimdall_http_msg_get_size(m) == 1234);

    /* Folding */
    heimdall_http_msg_add_header(m, "X-Folded", "Hello\n\tworld!");
    str = heimdall_http_msg_get_header(m, "x-folded");
    assert(str != NULL && !strcmp(str, "Hello \tworld!"));

    heimdall_http_msg_add_header(m, "TE", "gzip");
    heimdall_http_msg_add_header(m, "TE", "deflate");
    heimdall_http_msg_add_header(m, "Pragma", " features=\"broadcast,playlist\"");
    heimdall_http_msg_add_header(m, "Pragma", " client-id=123456789 ");
    heimdall_http_msg_add_header(m, "Pragma", "evulz=\"foo \\\"\\ bar\"");
    heimdall_http_msg_add_header(m, "Pragma", "no-cache ");

    str = heimdall_http_msg_get_header(m, "TE");
    assert(str != NULL && !strcmp(str, "gzip, deflate"));
    str = heimdall_http_msg_get_token(m, "TE", "gzip");
    assert(str != NULL && !strncmp(str, "gzip", 4));
    str = heimdall_http_msg_get_token(m, "TE", "deflate");
    assert(str != NULL && !strncmp(str, "deflate", 7));
    str = heimdall_http_msg_get_token(m, "TE", "compress");
    assert(str == NULL);
    str = heimdall_http_msg_get_token(m, "TE", "zip");
    assert(str == NULL);
    str = heimdall_http_msg_get_token(m, "TE", "late");
    assert(str == NULL);
    str = heimdall_http_msg_get_token(m, "Accept-Encoding", "gzip");
    assert(str == NULL);
    str = heimdall_http_msg_get_token(m, "Pragma", "features");
    assert(str != NULL && !strncmp(str, "features=\"", 10));
    str = heimdall_http_msg_get_token(m, "Pragma", "broadcast");
    assert(str == NULL);
    str = heimdall_http_msg_get_token(m, "Pragma", "playlist");
    assert(str == NULL);
    str = heimdall_http_msg_get_token(m, "Pragma", "client-id");
    assert(str != NULL && !strncmp(str, "client-id=", 10));
    str = heimdall_http_msg_get_token(m, "Pragma", "123456789");
    assert(str == NULL);
    str = heimdall_http_msg_get_token(m, "Pragma", "no-cache");
    assert(str != NULL && !strcmp(str, "no-cache"));

    heimdall_http_msg_add_header(m, "Cookie", "a=1");
    heimdall_http_msg_add_header(m, "Cookie", "b=2");
    str = heimdall_http_msg_get_header(m, "Cookie");
    assert(str != NULL && !strcmp(str, "a=1; b=2"));

    /* Error cases */
    assert(heimdall_http_msg_add_header(m, "/naughty", "header") == -1);
    assert(heimdall_http_msg_add_header(m, "", "void") == -1);

    assert(heimdall_http_msg_add_agent(m, "") != 0);
    assert(heimdall_http_msg_add_agent(m, "/1.0") != 0);
    assert(heimdall_http_msg_add_agent(m, "Bad/1.0\"") != 0);
    assert(heimdall_http_msg_add_agent(m, "Bad/1.0 (\\)") != 0);
    assert(heimdall_http_msg_add_agent(m, "Bad/1.0 (\\\x08)") != 0);
    assert(heimdall_http_msg_add_agent(m, "Bad/1.0 \"Evil\"") != 0);
    assert(heimdall_http_msg_add_agent(m, "(Hello world)") != 0);

    assert(heimdall_http_msg_add_creds_basic(m, false, "User:name", "12345") != 0);
    assert(heimdall_http_msg_add_creds_basic(m, false, "Alice\nand\nbob", "") != 0);
    assert(heimdall_http_msg_add_creds_basic(m, false, "john", "asdfg\x7f") != 0);

    heimdall_http_msg_destroy(m);

    const char *bad1[][2] = {
        { ":status", "200" },
        { ":status", "200" },
        { "Server",  "BigBad/1.0" },
    };

    m = heimdall_http_msg_h2_headers(3, bad1);
    assert(m == NULL);

    /* HTTP 1.x parser error cases */
    assert(heimdall_http_msg_headers("") == NULL);
    assert(heimdall_http_msg_headers("\r\n") == NULL);
    assert(heimdall_http_msg_headers("HTTP/1.1 200 OK") == NULL);
    assert(heimdall_http_msg_headers("HTTP/1.1 200 OK\r\nH: V\r\n") == NULL);
    assert(heimdall_http_msg_headers("HTTP/1.1 200 OK\r\n"
                                ":status: 200\r\n\r\n") == NULL);
    assert(heimdall_http_msg_headers("HTTP/1.1 200 OK\r\n"
                                "/naughty: invalid\r\n\r\n") == NULL);

    return 0;
}

/* Callback for heimdall_http_msg_h2_frame */
struct heimdall_h2_frame *
heimdall_h2_frame_headers(uint_fast32_t id, uint_fast32_t mtu, bool eos,
                     unsigned count, const char *const tab[][2])
{
    struct heimdall_http_msg *m;

    assert(id == 1);
    assert(mtu == HEIMDALL_H2_DEFAULT_MAX_FRAME);
    assert(eos);

    const char *headers[HEIMDALL_H2_MAX_HEADERS][2];

    for (unsigned i = 0; i < count; i++)
    {
        headers[i][0] = tab[i][0];
        headers[i][1] = tab[i][1];
    }

    m = heimdall_http_msg_h2_headers(count, headers);
    return (struct heimdall_h2_frame *)m; /* gruik */
}
