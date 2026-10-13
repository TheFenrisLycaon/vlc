/*****************************************************************************
 * message.c: HTTP request/response
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
#include <stdalign.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <heimdall_common.h>
#include <heimdall_http.h>
#include <heimdall_block.h>
#include <heimdall_strings.h>
#include <heimdall_memstream.h>
#include "message.h"
#include "h2frame.h"

struct heimdall_http_msg
{
    short status;
    char *method;
    char *scheme;
    char *authority;
    char *path;
    char *(*headers)[2];
    unsigned count;
    struct heimdall_http_stream *payload;
};

/* Maximum alignment for safe conversion to/from any specific pointer type */
static const char alignas (max_align_t) heimdall_http_error_loc;

void *const heimdall_http_error = (char *)&heimdall_http_error_loc;

static bool heimdall_http_is_token(const char *);

static ssize_t heimdall_http_msg_find_header(const struct heimdall_http_msg *m,
                                        const char *name)
{
    for (unsigned i = 0; i < m->count; i++)
        if (!strcasecmp(m->headers[i][0], name))
            return i;
    return -1;
}

static int heimdall_http_msg_vadd_header(struct heimdall_http_msg *m, const char *name,
                                    const char *fmt, va_list ap)
{
    if (!heimdall_http_is_token(name))
    {   /* Not a valid field name, i.e. not an HTTP token */
        errno = EINVAL;
        return -1;
    }

    char *value;
    int len = vasprintf(&value, fmt, ap);
    if (unlikely(len < 0))
        return -1;

    /* IETF RFC7230 §3.2.4 */
    for (char *p = value; *p; p++)
        if (*p == '\r' || *p == '\n')
            *p = ' ';

    /* Discard leading OWS */
    size_t crop = strspn(value, "\t ");
    if (crop > 0)
    {
        assert((unsigned)len >= crop);
        memmove(value, value + crop, len - crop + 1);
        len -= crop;
    }

    /* Discard trailing OWS */
    while (len > 0 && (value[len - 1] == '\t' || value[len - 1] == ' '))
        value[--len] = '\0';

    /* Fold identically named header field values. This is unfortunately not
     * possible for Set-Cookie, while Cookie requires a special separator. */
    ssize_t idx = heimdall_http_msg_find_header(m, name);
    if (idx >= 0 && strcasecmp(name, "Set-Cookie"))
    {
        char *merged;
        char sep = strcasecmp(name, "Cookie") ? ',' : ';';

        int val = asprintf(&merged, "%s%c %s", m->headers[idx][1], sep, value);

        free(value);

        if (unlikely(val == -1))
            return -1;

        free(m->headers[idx][1]);
        m->headers[idx][1] = merged;
        return 0;
    }

    char *(*h)[2] = realloc(m->headers, sizeof (char *[2]) * (m->count + 1));
    if (unlikely(h == NULL))
    {
        free(value);
        return -1;
    }

    m->headers = h;
    h += m->count;

    h[0][0] = strdup(name);
    if (unlikely(h[0][0] == NULL))
    {
        free(value);
        return -1;
    }

    h[0][1] = value;
    m->count++;
    return 0;
}

int heimdall_http_msg_add_header(struct heimdall_http_msg *m, const char *name,
                            const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = heimdall_http_msg_vadd_header(m, name, fmt, ap);
    va_end(ap);
    return ret;
}

const char *heimdall_http_msg_get_header(const struct heimdall_http_msg *m,
                                    const char *name)
{
    ssize_t idx = heimdall_http_msg_find_header(m, name);
    if (idx < 0)
    {
        errno = ENOENT;
        return NULL;
    }
    return m->headers[idx][1];
}

int heimdall_http_msg_get_status(const struct heimdall_http_msg *m)
{
    return m->status;
}

const char *heimdall_http_msg_get_method(const struct heimdall_http_msg *m)
{
    return m->method;
}

const char *heimdall_http_msg_get_scheme(const struct heimdall_http_msg *m)
{
    return m->scheme;
}

const char *heimdall_http_msg_get_authority(const struct heimdall_http_msg *m)
{
    return m->authority;
}

const char *heimdall_http_msg_get_path(const struct heimdall_http_msg *m)
{
    return m->path;
}

void heimdall_http_msg_destroy(struct heimdall_http_msg *m)
{
    if (m->payload != NULL)
        heimdall_http_stream_close(m->payload, false);

    for (unsigned i = 0; i < m->count; i++)
    {
        free(m->headers[i][0]);
        free(m->headers[i][1]);
    }

    free(m->headers);
    free(m->path);
    free(m->authority);
    free(m->scheme);
    free(m->method);
    free(m);
}

struct heimdall_http_msg *
heimdall_http_req_create(const char *method, const char *scheme,
                    const char *authority, const char *path)
{
    struct heimdall_http_msg *m = malloc(sizeof (*m));
    if (unlikely(m == NULL))
        return NULL;

    assert(method != NULL);
    m->status = -1;
    m->method = strdup(method);
    m->scheme = (scheme != NULL) ? strdup(scheme) : NULL;
    m->authority = (authority != NULL) ? strdup(authority) : NULL;
    m->path = (path != NULL) ? strdup(path) : NULL;
    m->count = 0;
    m->headers = NULL;
    m->payload = NULL;

    if (unlikely(m->method == NULL
              || (scheme != NULL && m->scheme == NULL)
              || (authority != NULL && m->authority == NULL)
              || (path != NULL && m->path == NULL)))
    {
        heimdall_http_msg_destroy(m);
        m = NULL;
    }
    return m;
}

struct heimdall_http_msg *heimdall_http_resp_create(unsigned status)
{
    struct heimdall_http_msg *m = malloc(sizeof (*m));
    if (unlikely(m == NULL))
        return NULL;

    assert(status < 1000);
    m->status = status;
    m->method = NULL;
    m->scheme = NULL;
    m->authority = NULL;
    m->path = NULL;
    m->count = 0;
    m->headers = NULL;
    m->payload = NULL;
    return m;
}

void heimdall_http_msg_attach(struct heimdall_http_msg *m, struct heimdall_http_stream *s)
{
    assert(m->payload == NULL);
    m->payload = s;
}

struct heimdall_http_msg *heimdall_http_msg_iterate(struct heimdall_http_msg *m)
{
    struct heimdall_http_msg *next = heimdall_http_stream_read_headers(m->payload);

    if (next != NULL)
        m->payload = NULL;

    heimdall_http_msg_destroy(m);
    return next;
}

struct heimdall_http_msg *heimdall_http_msg_get_initial(struct heimdall_http_stream *s)
{
    struct heimdall_http_msg *m = heimdall_http_stream_read_headers(s);
    if (m == NULL)
        heimdall_http_stream_close(s, false);
    return m;
}

struct heimdall_http_msg *heimdall_http_msg_get_final(struct heimdall_http_msg *m)
{
    while (m != NULL && (heimdall_http_msg_get_status(m) / 100) == 1)
        m = heimdall_http_msg_iterate(m);
    return m;
}

block_t *heimdall_http_msg_read(struct heimdall_http_msg *m)
{
    if (m->payload == NULL)
        return NULL;

    return heimdall_http_stream_read(m->payload);
}

int heimdall_http_msg_write(struct heimdall_http_msg *m, block_t *block, bool eos)
{
    if (m->payload == NULL)
        return -1;

    /* Ideally, we'd send the end-of-stream with the last block of data.
     * But if there are zero blocks, then we have to send it separately.
     */
    if (block == NULL && eos)
        return heimdall_http_stream_write(m->payload, NULL, 0, true);

    while (block != NULL)
    {
        block_t *next = block->p_next;
        bool end = eos && next == NULL;

        if (heimdall_http_stream_write(m->payload, block->p_buffer, block->i_buffer,
                                  end) < (ssize_t)block->i_buffer)
            goto error;

        block_Release(block);
        block = next;
    }

    return 0;
error:
    block_ChainRelease(block);
    return -1;
}

/* Serialization and deserialization */

char *heimdall_http_msg_format(const struct heimdall_http_msg *m, size_t *restrict lenp,
                          bool proxied, bool chunked)
{
    struct heimdall_memstream stream;

    heimdall_memstream_open(&stream);

    if (m->status < 0)
    {
        heimdall_memstream_printf(&stream, "%s ", m->method);
        if (proxied)
            heimdall_memstream_printf(&stream, "%s://%s", m->scheme, m->authority);
        heimdall_memstream_printf(&stream, "%s HTTP/1.1\r\nHost: %s\r\n",
                             m->path ? m->path : m->authority, m->authority);
    }
    else
        heimdall_memstream_printf(&stream, "HTTP/1.1 %03hd .\r\n", m->status);

    for (unsigned i = 0; i < m->count; i++)
        heimdall_memstream_printf(&stream, "%s: %s\r\n",
                             m->headers[i][0], m->headers[i][1]);

    if (chunked)
        heimdall_memstream_puts(&stream, "Transfer-Encoding: chunked\r\n");

    heimdall_memstream_puts(&stream, "\r\n");

    if (heimdall_memstream_close(&stream))
        return NULL;

    if (lenp != NULL)
        *lenp = stream.length;
    return stream.ptr;
}

struct heimdall_http_msg *heimdall_http_msg_headers(const char *msg)
{
    struct heimdall_http_msg *m;
    unsigned short code;

    if (sscanf(msg, "HTTP/1.%*1u %3hu %*s", &code) == 1)
    {
        m = heimdall_http_resp_create(code);
        if (unlikely(m == NULL))
            return NULL;
    }
    else
        return NULL; /* TODO: request support */

    msg = strstr(msg, "\r\n");
    if (msg == NULL)
        goto error;

    while (strcmp(msg + 2, "\r\n"))
    {
        const char *eol = msg;

        do
        {
            eol = strstr(eol + 2, "\r\n");
            if (eol == NULL)
                goto error;
        }   /* Deal with legacy obs-fold (i.e. multi-line header) */
        while (eol[2] == ' ' || eol[2] == '\t');

        msg += 2; /* skip CRLF */

        const char *colon = memchr(msg, ':', eol - msg);
        if (colon == NULL || colon == msg)
            goto error;

        char *name = strndup(msg, colon - msg);
        if (unlikely(name == NULL))
            goto error;

        colon++;
        colon += strspn(colon, " \t");

        if (unlikely(heimdall_http_msg_add_header(m, name, "%.*s",
                                             (int)(eol - colon), colon)))
        {
            free(name);
            goto error;
        }
        free(name);
        msg = eol;
    }

    return m;
error:
    heimdall_http_msg_destroy(m);
    return NULL;
}

struct heimdall_h2_frame *heimdall_http_msg_h2_frame(const struct heimdall_http_msg *m,
                                           uint_fast32_t stream_id, bool eos)
{
    for (unsigned j = 0; j < m->count; j++)
    {   /* No HTTP 1 specific headers */
        assert(strcasecmp(m->headers[j][0], "Connection"));
        assert(strcasecmp(m->headers[j][0], "Upgrade"));
        assert(strcasecmp(m->headers[j][0], "HTTP2-Settings"));
    }

    const char *(*headers)[2] = heimdall_alloc(m->count + 5, sizeof (char *[2]));
    if (unlikely(headers == NULL))
        return NULL;

    struct heimdall_h2_frame *f;
    unsigned i = 0;
    char status[4];

    if (m->status >= 0)
    {
        if (m->status >= 1000) heimdall_assert_unreachable();
        snprintf(status, ARRAY_SIZE(status), "%hd", m->status);
        headers[i][0] = ":status";
        headers[i][1] = status;
        i++;
    }
    if (m->method != NULL)
    {
        headers[i][0] = ":method";
        headers[i][1] = m->method;
        i++;
    }
    if (m->scheme != NULL)
    {
        headers[i][0] = ":scheme";
        headers[i][1] = m->scheme;
        i++;
    }
    if (m->authority != NULL)
    {
        headers[i][0] = ":authority";
        headers[i][1] = m->authority;
        i++;
    }
    if (m->path != NULL)
    {
        headers[i][0] = ":path";
        headers[i][1] = m->path;
        i++;
    }
    if (m->count > 0)
    {
        memcpy(headers + i, m->headers, m->count * sizeof (*headers));
        i += m->count;
    }

    f = heimdall_h2_frame_headers(stream_id, HEIMDALL_H2_DEFAULT_MAX_FRAME, eos,
                             i, headers);
    free(headers);
    return f;
}

struct heimdall_http_msg *heimdall_http_msg_h2_headers(unsigned n,
                                             const char *const hdrs[][2])
{
    struct heimdall_http_msg *m = heimdall_http_resp_create(0);
    if (unlikely(m == NULL))
        return NULL;

    for (unsigned i = 0; i < n; i++)
    {
        const char *name = hdrs[i][0];
        const char *value = hdrs[i][1];

        /* NOTE: HPACK always returns lower-case names, so strcmp() is fine. */
        if (!strcmp(name, ":status"))
        {
            char *end;
            unsigned long status = strtoul(value, &end, 10);

            if (m->status != 0 || status > 999 || *end != '\0')
                goto error; /* Not a three decimal digits status! */

            m->status = status;
            continue;
        }

        if (!strcmp(name, ":method"))
        {
            if (m->method != NULL)
                goto error;

            m->method = strdup(value);
            if (unlikely(m->method == NULL))
                goto error;

            m->status = -1; /* this is a request */
            continue;
        }

        if (!strcmp(name, ":scheme"))
        {
            if (m->scheme != NULL)
                goto error;

            m->scheme = strdup(value);
            if (unlikely(m->scheme == NULL))
                goto error;
            continue;
        }

        if (!strcmp(name, ":authority"))
        {
            if (m->authority != NULL)
                goto error;

            m->authority = strdup(value);
            if (unlikely(m->authority == NULL))
                goto error;
            continue;
        }

        if (!strcmp(name, ":path"))
        {
            if (m->path != NULL)
                goto error;

            m->path = strdup(value);
            if (unlikely(m->path == NULL))
                goto error;
            continue;
        }

        if (heimdall_http_msg_add_header(m, name, "%s", value))
            goto error;
    }

    if ((m->status < 0) == (m->method == NULL))
    {   /* Must be either a request or response. Not both, not neither. */
error:
        heimdall_http_msg_destroy(m);
        m = NULL;
    }
    return m;
}

/* Header helpers */

char *heimdall_http_authority(const char *host, unsigned port)
{
    static const char *const formats[4] = { "%s", "[%s]", "%s:%u", "[%s]:%u" };
    const bool brackets = strchr(host, ':') != NULL;
    const char *fmt = formats[brackets + 2 * (port != 0)];
    char *authority;

    if (unlikely(asprintf(&authority, fmt, host, port) == -1))
        return NULL;
    return authority;
}

static int heimdall_http_istoken(int c)
{   /* IETF RFC7230 §3.2.6 */
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c && strchr("!#$%&'*+-.^_`|~", c) != NULL);
}

static int heimdall_http_isctext(int c)
{   /* IETF RFC7230 §3.2.6 */
    return (c == '\t') || (c == ' ') || (c >= 0x21 && c <= 0x27)
        || (c >= 0x2A && c <= 0x5B) || (c >= 0x5D && c <= 0x7E)
        || (c >= 0x80);
}

static size_t heimdall_http_token_length(const char *str)
{
    size_t i = 0;

    while (heimdall_http_istoken(str[i]))
        i++;
    return i;
}

static size_t heimdall_http_quoted_length(const char *str)
{
    size_t i = 0;
    unsigned char c;

    if (str[i++] != '"')
        return 0;

    do
    {
        c = str[i++];

        if (c == '\0')
            return 0;

        if (c == '\\') /* Quoted pair */
        {
            unsigned char q = str[i++];
            if (q < 32 && q != '\t')
                return 0;
        }
    }
    while (c != '"');

    return i;
}

static bool heimdall_http_is_token(const char *str)
{
    size_t len = heimdall_http_token_length(str);
    return len > 0 && str[len] == '\0';
}

const char *heimdall_http_next_token(const char *value)
{   /* We handle either token or token = token / quoted-string */
    value += strcspn(value, ",\"");
    if (!*value)
        return NULL;

    value += heimdall_http_quoted_length(value);
    return value + strspn(value, "\t ,");
}

static const char *heimdall_http_get_token(const char *value, const char *token)
{
    const size_t length = strlen(token);

    while (value != NULL)
    {
        if (heimdall_http_token_length(value) == length
         && !strncasecmp(token, value, length))
            return value;

        value = heimdall_http_next_token(value);
    }

    return NULL;
}

static char *heimdall_http_get_token_value(const char *value, const char *token)
{
    value = heimdall_http_get_token(value, token);
    if (value == NULL)
        return NULL;

    value += heimdall_http_token_length(value);
    value += strspn(value, " \t"); /* BWS */

    if (*value != '=')
        return NULL;

    value++;
    value += strspn(value, " \t"); /* BWS */

    size_t len = heimdall_http_quoted_length(value);
    if (len == 0)
        return NULL;

    assert(len >= 2);
    value++;
    len -= 2;

    char *out = malloc(len + 1), *p;
    if (unlikely(out == NULL))
        return NULL;

    for (p = out; len > 0; len--)
    {
        char c = *(value++);
        if (c == '\\') /* Quoted pair */
        {
            c = *(value++);
            len--;
        }
        *(p++) = c;
    }
    *p = '\0';
    return out;
}

const char *heimdall_http_msg_get_token(const struct heimdall_http_msg *msg,
                                   const char *field, const char *token)
{
    return heimdall_http_get_token(heimdall_http_msg_get_header(msg, field), token);
}

static size_t heimdall_http_comment_length(const char *str)
{   /* IETF RFC7230 §3.2.6 */
    if (*str != '(')
        return 0;

    size_t i = 1;

    for (size_t nested = 1; nested > 0; i++)
    {
        unsigned char c = str[i];

        if (c == ')')
            nested--;
        else
        if (c == '(') /* Nested comment */
            nested++;
        else
        if (c == '\\') /* Quoted pair */
        {
            i++;
            if (str[i] < 32)
                return 0;
        }
        else
        if (!heimdall_http_isctext(c))
            return 0;
    }
    return i;
}

static bool heimdall_http_is_agent(const char *s)
{   /* IETF RFC7231 §5.5.3 and §7.4.2 */
    if (!heimdall_http_istoken(*s))
        return false;

    for (;;)
    {
        size_t l = heimdall_http_token_length(s);
        if (l != 0) /* product */
        {
            if (s[l] == '/') /* product version */
            {
                s += l + 1;
                l = heimdall_http_token_length(s);
            }
        }
        else
            l = heimdall_http_comment_length(s);

        if (l == 0)
            break;

        s += l;

        if (*s == '\0')
            return true;

        l = strspn(s, "\t "); /* RWS */

        if (l == 0)
            break;

        s += l;
    }

    return false;
}

int heimdall_http_msg_add_agent(struct heimdall_http_msg *m, const char *str)
{
    const char *hname = (m->status < 0) ? "User-Agent" : "Server";

    if (!heimdall_http_is_agent(str))
    {
        errno = EINVAL;
        return -1;
    }
    return heimdall_http_msg_add_header(m, hname, "%s", str);
}

const char *heimdall_http_msg_get_agent(const struct heimdall_http_msg *m)
{
    const char *hname = (m->status < 0) ? "User-Agent" : "Server";
    const char *str = heimdall_http_msg_get_header(m, hname);

    return (str != NULL && heimdall_http_is_agent(str)) ? str : NULL;
}

static const char heimdall_http_days[7][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char heimdall_http_months[12][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

int heimdall_http_msg_add_time(struct heimdall_http_msg *m, const char *hname,
                          const time_t *t)
{
    struct tm tm;

    if (gmtime_r(t, &tm) == NULL)
        return -1;
    return heimdall_http_msg_add_header(m, hname,
                                   "%s, %02d %s %04d %02d:%02d:%02d GMT",
                                   heimdall_http_days[tm.tm_wday], tm.tm_mday,
                                   heimdall_http_months[tm.tm_mon],
                                   1900 + tm.tm_year,
                                   tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int heimdall_http_msg_add_atime(struct heimdall_http_msg *m)
{
    time_t now;

    time(&now);
    return heimdall_http_msg_add_time(m, "Date", &now);
}

time_t heimdall_http_mktime(const char *str)
{   /* IETF RFC7231 §7.1.1.1 */
    struct tm tm;
    char mon[4];

    /* Internet Message Format date */
    if (sscanf(str, "%*c%*c%*c, %2d %3s %4d %2d:%2d:%2d", &tm.tm_mday, mon,
               &tm.tm_year, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6
    /* ANSI C format */
     || sscanf(str, "%*3s %3s %2d %2d:%2d:%2d %4d", mon, &tm.tm_mday,
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &tm.tm_year) == 6)
        tm.tm_year -= 1900;
    /* RFC850 date */
    else if (sscanf(str, "%*[^,], %2d-%3s-%2d %2d:%2d:%2d", &tm.tm_mday, mon,
                    &tm.tm_year, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6)
    {
        if (tm.tm_year <= 75)
            tm.tm_year += 100; /* Y2K compat, sort of */
    }
    else /* Unknown format */
        goto error;

    for (tm.tm_mon = 0; tm.tm_mon < 12; tm.tm_mon++)
        if (!strcmp(mon, heimdall_http_months[tm.tm_mon])) /* found month */
            return timegm(&tm);
error:
    errno = EINVAL;
    return -1; /* invalid month */
}

time_t heimdall_http_msg_get_time(const struct heimdall_http_msg *m, const char *name)
{
    const char *str = heimdall_http_msg_get_header(m, name);
    if (str == NULL)
        return -1;
    return heimdall_http_mktime(str);
}

time_t heimdall_http_msg_get_atime(const struct heimdall_http_msg *m)
{
    return heimdall_http_msg_get_time(m, "Date");
}

time_t heimdall_http_msg_get_mtime(const struct heimdall_http_msg *m)
{
    return heimdall_http_msg_get_time(m, "Last-Modified");
}

unsigned heimdall_http_msg_get_retry_after(const struct heimdall_http_msg *m)
{
    const char *str = heimdall_http_msg_get_header(m, "Retry-After");
    if (str == NULL)
        return 0;

    char *end;
    unsigned long delay = strtoul(str, &end, 10);
    if (end != str && *end == '\0')
        return delay;

    time_t t = heimdall_http_mktime(str);
    if (t != (time_t)-1)
    {
        time_t now;

        time(&now);
        if (t >= now)
            return t - now;
    }
    return 0;
}

uintmax_t heimdall_http_msg_get_size(const struct heimdall_http_msg *m)
{   /* IETF RFC7230 §3.3.3 */
    if ((m->status / 100) == 1 /* Informational 1xx (implicitly void) */
     || m->status == 204 /* No Content (implicitly void) */
     || m->status == 205 /* Reset Content (must be explicitly void) */
     || m->status == 304 /* Not Modified */)
        return 0;

    const char *str = heimdall_http_msg_get_header(m, "Transfer-Encoding");
    if (str != NULL) /* Transfer-Encoding preempts Content-Length */
        return -1;

    str = heimdall_http_msg_get_header(m, "Content-Length");
    if (str == NULL)
    {
        if (m->status < 0)
            return 0; /* Requests are void by default */
        return -1; /* Response of unknown size (e.g. chunked) */
    }

    uintmax_t length;

    if (sscanf(str, "%" SCNuMAX, &length) == 1)
        return length;

    errno = EINVAL;
    return -1;
}

void heimdall_http_msg_get_cookies(const struct heimdall_http_msg *m,
                              heimdall_http_cookie_jar_t *jar,
                              const char *host, const char *path)
{
    if (jar == NULL)
        return;

    for (unsigned i = 0; i < m->count; i++)
        if (!strcasecmp(m->headers[i][0], "Set-Cookie"))
            heimdall_http_cookies_store(jar, m->headers[i][1], host, path);
}

int heimdall_http_msg_add_cookies(struct heimdall_http_msg *m,
                             heimdall_http_cookie_jar_t *jar)
{
    char *host, *cookies;
    int val = 0;
    bool secure;

    if (m->scheme == NULL || m->authority == NULL || m->path == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (!strcasecmp(m->scheme, "https"))
        secure = true;
    else if (!strcasecmp(m->scheme, "http"))
        secure = false;
    else
        return 0;

    if (jar == NULL)
        return 0;

    if (m->authority[0] == '[')
        host = strndup(m->authority + 1, strcspn(m->authority + 1, "]"));
    else
        host = strndup(m->authority, strcspn(m->authority, ":"));
    if (unlikely(host == NULL))
        return -1;

    cookies = heimdall_http_cookies_fetch(jar, secure, host, m->path);
    free(host);

    if (cookies != NULL)
    {
        val = heimdall_http_msg_add_header(m, "Cookie", "%s", cookies);
        free(cookies);
    }
    return val;
}

char *heimdall_http_msg_get_basic_realm(const struct heimdall_http_msg *m)
{
    const char *auth;

    /* TODO: Support other authentication schemes. */
    /* NOTE: In principles, RFC7235 allows for multiple authentication schemes,
     * including multiple times Basic with a different realm each. There are no
     * UI paradigms though. */
    auth = heimdall_http_msg_get_token(m, "WWW-Authenticate", "Basic");
    if (auth == NULL)
        return NULL;

    auth += 5;
    auth += strspn(auth, " "); /* 1*SP */

    return heimdall_http_get_token_value(auth, "realm");
}

int heimdall_http_msg_add_creds_basic(struct heimdall_http_msg *m, bool proxy,
                                 const char *username, const char *password)
{
    char *str, *token;
    int ret;
    unsigned char c;

    /* CTL characters and colons not permitted in username */
    for (size_t len = 0; (c = username[len]) != '\0'; len++)
        if (c < 32 || c == 127 || c == ':')
        {
            errno = EINVAL;
            return -1;
        }

    /* CTL characters not permitted in password */
    for (size_t len = 0; (c = password[len]) != '\0'; len++)
        if (c < 32 || c == 127)
        {
            errno = EINVAL;
            return -1;
        }

    ret = asprintf(&str, "%s:%s", username, password);
    if (unlikely(ret < 0))
        return -1;

    token = heimdall_b64_encode_binary((unsigned char *)str, ret);
    free(str);
    if (unlikely(token == NULL))
        return -1;

    ret = heimdall_http_msg_add_header(m, proxy ? "Proxy-Authorization" :
                                  "Authorization", "Basic %s", token);
    free(token);
    return ret;
}
