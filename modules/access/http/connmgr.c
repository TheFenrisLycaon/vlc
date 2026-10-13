/*****************************************************************************
 * connmgr.c: HTTP/TLS Heimdall connection manager
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
# include "config.h"
#endif

#include <assert.h>
#include <heimdall_common.h>
#include <heimdall_network.h>
#include <heimdall_tls.h>
#include <heimdall_url.h>
#include "transport.h"
#include "conn.h"
#include "connmgr.h"
#include "message.h"

#pragma GCC visibility push(default)

void heimdall_http_err(void *ctx, const char *fmt, ...)
{
    struct heimdall_logger *logger = ctx;
    va_list ap;

    va_start(ap, fmt);
    heimdall_vaLog(&logger, HEIMDALL_MSG_ERR, "generic", "http",
              __FILE__, __LINE__, __func__, fmt, ap);
    va_end(ap);
}

void heimdall_http_dbg(void *ctx, const char *fmt, ...)
{
    struct heimdall_logger *logger = ctx;
    va_list ap;

    va_start(ap, fmt);
    heimdall_vaLog(&logger, HEIMDALL_MSG_DBG, "generic", "http",
              __FILE__, __LINE__, __func__, fmt, ap);
    va_end(ap);
}

heimdall_tls_t *heimdall_https_connect(heimdall_tls_client_t *creds, const char *name,
                             unsigned port, bool *restrict two)
{
    if (port == 0)
        port = 443;

    /* TLS with ALPN */
    const char *alpn[] = { "h2", "http/1.1", NULL };
    char *alp;

    heimdall_tls_t *tls = heimdall_tls_SocketOpenTLS(creds, name, port, "https",
                                           alpn + !*two, &alp);
    if (tls != NULL)
    {
        *two = (alp != NULL) && !strcmp(alp, "h2");
        free(alp);
    }
    return tls;
}

static char *heimdall_http_proxy_find(const char *hostname, unsigned port,
                                 bool secure)
{
    const char *fmt;
    char *url, *proxy = NULL;

    if (strchr(hostname, ':') != NULL)
        fmt = port ? "http%s://[%s]:%u" : "http%s://[%s]";
    else
        fmt = port ? "http%s://%s:%u" : "http%s://%s";

    if (likely(asprintf(&url, fmt, secure ? "s" : "", hostname, port) >= 0))
    {
        proxy = heimdall_getProxyUrl(url);
        free(url);
    }
    return proxy;
}


struct heimdall_http_mgr
{
    struct heimdall_logger *logger;
    heimdall_object_t *obj;
    heimdall_tls_client_t *creds;
    struct heimdall_http_cookie_jar_t *jar;
    struct heimdall_http_conn *conn;
};

static struct heimdall_http_conn *heimdall_http_mgr_find(struct heimdall_http_mgr *mgr,
                                               const char *host, unsigned port)
{
    (void) host; (void) port;
    return mgr->conn;
}

static void heimdall_http_mgr_release(struct heimdall_http_mgr *mgr,
                                 struct heimdall_http_conn *conn)
{
    assert(mgr->conn == conn);
    mgr->conn = NULL;

    heimdall_http_conn_release(conn);
}

static
struct heimdall_http_msg *heimdall_http_mgr_reuse(struct heimdall_http_mgr *mgr,
                                        const char *host, unsigned port,
                                        const struct heimdall_http_msg *req,
                                        bool payload)
{
    struct heimdall_http_conn *conn = heimdall_http_mgr_find(mgr, host, port);
    if (conn == NULL)
        return NULL;

    struct heimdall_http_stream *stream = heimdall_http_stream_open(conn, req, payload);
    if (stream != NULL)
    {
        struct heimdall_http_msg *m = heimdall_http_msg_get_initial(stream);
        if (m != NULL)
            return m;
    }
    /* Get rid of closing or reset connection */
    heimdall_http_mgr_release(mgr, conn);
    return NULL;
}

static struct heimdall_http_msg *heimdall_https_request(struct heimdall_http_mgr *mgr,
                                              const char *host, unsigned port,
                                              const struct heimdall_http_msg *req,
                                              bool idempotent, bool payload)
{
    heimdall_tls_t *tls;
    bool http2 = true;

    if (mgr->creds == NULL && mgr->conn != NULL)
        return NULL; /* switch from HTTP to HTTPS not implemented */

    if (mgr->creds == NULL)
    {   /* First TLS connection: load x509 credentials */
        mgr->creds = heimdall_tls_ClientCreate(mgr->obj);
        if (mgr->creds == NULL)
            return NULL;
    }

    if (idempotent)
    {   /* If the request is idempotent, try to reuse an existing connection.
         * Otherwise, it is possible but unadvisable as we would not know if
         * the nonidempotent request was processed if the connection fails
         * before the response is received.
         */
        struct heimdall_http_msg *resp = heimdall_http_mgr_reuse(mgr, host, port, req,
                                                       payload);
        if (resp != NULL)
            return resp; /* existing connection reused */
    }

    char *proxy = heimdall_http_proxy_find(host, port, true);
    if (proxy != NULL)
    {
        tls = heimdall_https_connect_proxy(mgr->creds, mgr->creds,
                                      host, port, &http2, proxy);
        free(proxy);
    }
    else
        tls = heimdall_https_connect(mgr->creds, host, port, &http2);

    if (tls == NULL)
        return NULL;

    struct heimdall_http_conn *conn;

    /* For HTTPS, TLS-ALPN determines whether HTTP version 2.0 ("h2") or 1.1
     * ("http/1.1") is used.
     * NOTE: If the negotiated protocol is explicitly "http/1.1", HTTP 1.0
     * should not be used. HTTP 1.0 should only be used if ALPN is not
     * supported by the server.
     * NOTE: We do not enforce TLS version 1.2 for HTTP 2.0 explicitly.
     */
    if (http2)
        conn = heimdall_h2_conn_create(mgr->logger, tls);
    else
        conn = heimdall_h1_conn_create(mgr->logger, tls, false);

    if (unlikely(conn == NULL))
    {
        heimdall_tls_Close(tls);
        return NULL;
    }

    if (mgr->conn != NULL)
        heimdall_http_mgr_release(mgr, mgr->conn);

    mgr->conn = conn;
    return heimdall_http_mgr_reuse(mgr, host, port, req, payload);
}

static struct heimdall_http_msg *heimdall_http_request(struct heimdall_http_mgr *mgr,
                                             const char *host, unsigned port,
                                             const struct heimdall_http_msg *req,
                                             bool idempotent, bool payload)
{
    if (mgr->creds != NULL && mgr->conn != NULL)
        return NULL; /* switch from HTTPS to HTTP not implemented */

    if (idempotent)
    {
        struct heimdall_http_msg *resp = heimdall_http_mgr_reuse(mgr, host, port, req,
                                                       payload);
        if (resp != NULL)
            return resp;
    }

    struct heimdall_http_conn *conn;
    struct heimdall_http_stream *stream;

    char *proxy = heimdall_http_proxy_find(host, port, false);
    if (proxy != NULL)
    {
        heimdall_url_t url;

        heimdall_UrlParse(&url, proxy);
        free(proxy);

        if (url.psz_host != NULL)
            stream = heimdall_h1_request(mgr->logger, url.psz_host,
                                    url.i_port ? url.i_port : 80, true, req,
                                    idempotent, payload, &conn);
        else
            stream = NULL;

        heimdall_UrlClean(&url);
    }
    else
        stream = heimdall_h1_request(mgr->logger, host, port ? port : 80, false,
                                req, idempotent, payload, &conn);

    if (stream == NULL)
        return NULL;

    struct heimdall_http_msg *resp = heimdall_http_msg_get_initial(stream);
    if (resp == NULL)
    {
        heimdall_http_conn_release(conn);
        return NULL;
    }

    if (mgr->conn != NULL)
        heimdall_http_mgr_release(mgr, mgr->conn);

    mgr->conn = conn;
    return resp;
}

struct heimdall_http_msg *heimdall_http_mgr_request(struct heimdall_http_mgr *mgr, bool https,
                                          const char *host, unsigned port,
                                          const struct heimdall_http_msg *m,
                                          bool idempotent, bool payload)
{
    if (port && heimdall_http_port_blocked(port))
        return NULL;

    return (https ? heimdall_https_request : heimdall_http_request)(mgr, host, port, m,
                                                          idempotent, payload);
}

struct heimdall_http_cookie_jar_t *heimdall_http_mgr_get_jar(struct heimdall_http_mgr *mgr)
{
    return mgr->jar;
}

struct heimdall_http_mgr *heimdall_http_mgr_create(heimdall_object_t *obj,
                                         struct heimdall_http_cookie_jar_t *jar)
{
    struct heimdall_http_mgr *mgr = malloc(sizeof (*mgr));
    if (unlikely(mgr == NULL))
        return NULL;

    mgr->logger = obj->logger;
    mgr->obj = obj;
    mgr->creds = NULL;
    mgr->jar = jar;
    mgr->conn = NULL;
    return mgr;
}

void heimdall_http_mgr_destroy(struct heimdall_http_mgr *mgr)
{
    if (mgr->conn != NULL)
        heimdall_http_mgr_release(mgr, mgr->conn);
    if (mgr->creds != NULL)
        heimdall_tls_ClientDelete(mgr->creds);
    free(mgr);
}
