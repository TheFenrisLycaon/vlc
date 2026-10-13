/*****************************************************************************
 * tunnel.c: HTTP CONNECT
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_tls.h>
#include <heimdall_url.h>
#include "message.h"
#include "conn.h"
#include "transport.h"

static struct heimdall_http_msg *heimdall_http_tunnel_open(struct heimdall_http_conn *conn,
                                                 const char *hostname,
                                                 unsigned port,
                                                 const char *username,
                                                 const char *password)
{
    char *authority = heimdall_http_authority(hostname, port);
    if (authority == NULL)
        return NULL;

    struct heimdall_http_msg *req = heimdall_http_req_create("CONNECT", NULL, authority,
                                                   NULL);
    free(authority);
    if (unlikely(req == NULL))
        return NULL;

    heimdall_http_msg_add_header(req, "ALPN", "h2, http%%2F1.1");
    heimdall_http_msg_add_agent(req, PACKAGE_NAME "/" PACKAGE_VERSION);
    if (username != NULL)
        heimdall_http_msg_add_creds_basic(req, true, username,
                                     (password != NULL) ? password : "");

    struct heimdall_http_stream *stream = heimdall_http_stream_open(conn, req, false);

    heimdall_http_msg_destroy(req);
    if (stream == NULL)
        return NULL;

    struct heimdall_http_msg *resp = heimdall_http_msg_get_initial(stream);
    resp = heimdall_http_msg_get_final(resp);
    if (resp == NULL)
        return NULL;

    int status = heimdall_http_msg_get_status(resp);
    if ((status / 100) != 2)
    {
        heimdall_http_msg_destroy(resp);
        resp = NULL;
    }
    return resp;
}

typedef struct heimdall_tls_proxy
{
    heimdall_tls_t tls;
    heimdall_tls_t *sock;
} heimdall_tls_proxy_t;

static int heimdall_tls_ProxyGetFD(heimdall_tls_t *tls, short *restrict events)
{
    heimdall_tls_proxy_t *proxy = (heimdall_tls_proxy_t *)tls;

    return heimdall_tls_GetPollFD(proxy->sock, events);
}

static ssize_t heimdall_tls_ProxyRead(heimdall_tls_t *tls, struct iovec *iov,
                                 unsigned count)
{
    heimdall_tls_proxy_t *proxy = (heimdall_tls_proxy_t *)tls;
    heimdall_tls_t *sock = proxy->sock;

    return sock->ops->readv(sock, iov, count);
}

static ssize_t heimdall_tls_ProxyWrite(heimdall_tls_t *tls, const struct iovec *iov,
                                  unsigned count)
{
    heimdall_tls_proxy_t *proxy = (heimdall_tls_proxy_t *)tls;
    heimdall_tls_t *sock = proxy->sock;

    return sock->ops->writev(sock, iov, count);
}

static int heimdall_tls_ProxyShutdown(heimdall_tls_t *tls, bool duplex)
{
    heimdall_tls_proxy_t *proxy = (heimdall_tls_proxy_t *)tls;

    return heimdall_tls_Shutdown(proxy->sock, duplex);
}

static void heimdall_tls_ProxyClose(heimdall_tls_t *tls)
{
    heimdall_tls_proxy_t *proxy = (heimdall_tls_proxy_t *)tls;

    free(proxy);
}

static const struct heimdall_tls_operations heimdall_tls_proxy_ops =
{
    heimdall_tls_ProxyGetFD,
    heimdall_tls_ProxyRead,
    heimdall_tls_ProxyWrite,
    heimdall_tls_ProxyShutdown,
    heimdall_tls_ProxyClose,
};

heimdall_tls_t *heimdall_https_connect_proxy(void *ctx, heimdall_tls_client_t *creds,
                                   const char *hostname, unsigned port,
                                   bool *restrict two, const char *proxy)
{
    heimdall_url_t url;
    int canc;

    assert(proxy != NULL);

    if (port == 0)
        port = 443;

    canc = heimdall_savecancel();
    heimdall_UrlParse(&url, proxy);
    heimdall_restorecancel(canc);

    if (url.psz_protocol == NULL || url.psz_host == NULL)
    {
        heimdall_UrlClean(&url);
        return NULL;
    }

    heimdall_tls_t *sock = NULL;
    bool ptwo = false;
    if (!strcasecmp(url.psz_protocol, "https"))
        sock = heimdall_https_connect(creds, url.psz_host, url.i_port, &ptwo);
    else
    if (!strcasecmp(url.psz_protocol, "http"))
        sock = heimdall_tls_SocketOpenTCP(creds ? heimdall_object_parent(creds) : NULL,
                                     url.psz_host, url.i_port);
    else
        sock = NULL;

    if (sock == NULL)
    {
        heimdall_UrlClean(&url);
        return NULL;
    }

    assert(!ptwo); /* HTTP/2 proxy not supported yet */

    heimdall_tls_proxy_t *psock = malloc(sizeof (*psock));
    if (unlikely(psock == NULL))
    {
        heimdall_UrlClean(&url);
        goto error;
    }

    psock->tls.ops = &heimdall_tls_proxy_ops;
    psock->tls.p = NULL;
    psock->sock = sock;

    struct heimdall_http_conn *conn = /*ptwo ? heimdall_h2_conn_create(ctx, &psock->tls)
                               :*/ heimdall_h1_conn_create(ctx, &psock->tls, false);
    if (unlikely(conn == NULL))
    {
        heimdall_tls_Close(&psock->tls);
        heimdall_UrlClean(&url);
        goto error;
    }

    struct heimdall_http_msg *resp = heimdall_http_tunnel_open(conn, hostname, port,
                                                     url.psz_username,
                                                     url.psz_password);
    heimdall_UrlClean(&url);

    /* TODO: reuse connection to HTTP/2 proxy */
    heimdall_http_conn_release(conn); /* psock is destroyed there too */

    if (resp == NULL)
        goto error;

    heimdall_tls_t *tls;
    const char *alpn[] = { "h2", "http/1.1", NULL };
    char *alp;

    tls = heimdall_tls_ClientSessionCreate(creds, sock, hostname, "https",
                                      alpn + !*two, &alp);
    if (tls == NULL)
        goto error;

    *two = (alp != NULL) && !strcmp(alp, "h2");
    free(alp);
    return tls;
error:
    heimdall_tls_Close(sock);
    return NULL;
}
