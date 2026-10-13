/*****************************************************************************
 * tls.c
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
 * @ingroup tls
 * @file
 * Transport Layer Session protocol API.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_POLL_H
# include <poll.h>
#endif
#include <assert.h>
#include <errno.h>
#ifndef SOL_TCP
# define SOL_TCP IPPROTO_TCP
#endif

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include "../libheimdall.h"

#include <heimdall_tls.h>
#include <heimdall_modules.h>
#include <heimdall_interrupt.h>

/*** TLS credentials ***/

heimdall_tls_server_t *
heimdall_tls_ServerCreate (heimdall_object_t *obj, const char *cert_path,
                      const char *key_path)
{
    heimdall_tls_server_t *srv = heimdall_custom_create(obj, sizeof (*srv),
                                              "tls server");
    if (unlikely(srv == NULL))
        return NULL;

    if (key_path == NULL)
        key_path = cert_path;

    module_t **mods;
    ssize_t total = heimdall_module_match("tls server", NULL, false, &mods, NULL);

    for (ssize_t i = 0; i < total; i++) {
        int (*probe)(heimdall_tls_server_t *, const char *, const char *);

        probe = heimdall_module_map(obj->logger, mods[i]);

        if (probe != NULL && probe(srv, cert_path, key_path) == HEIMDALL_SUCCESS) {
            free(mods);
            return srv;
        }

        heimdall_objres_clear(HEIMDALL_OBJECT(srv));
    }

    free(mods);
    msg_Err(srv, "TLS server plugin not available");
    heimdall_object_delete(srv);
    return NULL;
}

void heimdall_tls_ServerDelete(heimdall_tls_server_t *crd)
{
    if (crd == NULL)
        return;

    crd->ops->destroy(crd);
    heimdall_objres_clear(HEIMDALL_OBJECT(crd));
    heimdall_object_delete(crd);
}

heimdall_tls_client_t *heimdall_tls_ClientCreate(heimdall_object_t *obj)
{
    heimdall_tls_client_t *crd = heimdall_custom_create(obj, sizeof (*crd),
                                              "tls client");
    if (unlikely(crd == NULL))
        return NULL;

    module_t **mods;
    ssize_t total = heimdall_module_match("tls client", NULL, false, &mods, NULL);

    for (ssize_t i = 0; i < total; i++) {
        int (*probe)(heimdall_tls_client_t *);

        probe = heimdall_module_map(obj->logger, mods[i]);

        if (probe != NULL && probe(crd) == HEIMDALL_SUCCESS) {
            free(mods);
            return crd;
        }

        heimdall_objres_clear(HEIMDALL_OBJECT(crd));
    }

    free(mods);
    msg_Err(crd, "TLS client plugin not available");
    heimdall_object_delete(crd);
    return NULL;
}

void heimdall_tls_ClientDelete(heimdall_tls_client_t *crd)
{
    if (crd == NULL)
        return;

    crd->ops->destroy(crd);
    heimdall_objres_clear(HEIMDALL_OBJECT(crd));
    heimdall_object_delete(crd);
}


/*** TLS  session ***/

void heimdall_tls_SessionDelete (heimdall_tls_t *session)
{
    int canc = heimdall_savecancel();
    session->ops->close(session);
    heimdall_restorecancel(canc);
}

static void cleanup_tls(void *data)
{
    heimdall_tls_t *session = data;

    heimdall_tls_SessionDelete (session);
}

heimdall_tls_t *heimdall_tls_ClientSessionCreate(heimdall_tls_client_t *crd, heimdall_tls_t *sock,
                                       const char *host, const char *service,
                                       const char *const *alpn, char **alp)
{
    int val;
    int canc = heimdall_savecancel();
    heimdall_tls_t *session = crd->ops->open(crd, sock, host, alpn);
    heimdall_restorecancel(canc);

    if (session == NULL)
        return NULL;

    session->p = sock;

    canc = heimdall_savecancel();
    heimdall_tick_t deadline = heimdall_tick_now ();
    deadline += HEIMDALL_TICK_FROM_MS( var_InheritInteger (crd, "ipv4-timeout") );

    heimdall_cleanup_push (cleanup_tls, session);
    while ((val = crd->ops->handshake(session, host, service, alp)) != 0)
    {
        struct pollfd ufd[1];

        if (val < 0 || heimdall_killed() )
        {
            if (val < 0)
                msg_Err(crd, "TLS session handshake error");
error:
            heimdall_tls_SessionDelete (session);
            session = NULL;
            break;
        }

        heimdall_tick_t now = heimdall_tick_now ();
        if (now > deadline)
           now = deadline;

        assert (val <= 2);

        ufd[0].events = (val == 1) ? POLLIN : POLLOUT;
        ufd[0].fd = heimdall_tls_GetPollFD(sock, &ufd->events);

        heimdall_restorecancel(canc);
        val = heimdall_poll_i11e(ufd, 1, MS_FROM_HEIMDALL_TICK(deadline - now));
        canc = heimdall_savecancel();
        if (val == 0)
        {
            msg_Err(crd, "TLS session handshake timeout");
            goto error;
        }
    }
    heimdall_cleanup_pop();
    heimdall_restorecancel(canc);
    return session;
}

heimdall_tls_t *heimdall_tls_ServerSessionCreate(heimdall_tls_server_t *crd,
                                       heimdall_tls_t *sock,
                                       const char *const *alpn)
{
    int canc = heimdall_savecancel();
    heimdall_tls_t *session = crd->ops->open(crd, sock, alpn);
    heimdall_restorecancel(canc);
    if (session != NULL)
        session->p = sock;
    return session;
}

heimdall_tls_t *heimdall_tls_SocketOpenTLS(heimdall_tls_client_t *creds, const char *name,
                                 unsigned port, const char *service,
                                 const char *const *alpn, char **alp)
{
    struct addrinfo hints =
    {
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
    }, *res;

    msg_Dbg(creds, "resolving %s ...", name);

    int val = heimdall_getaddrinfo_i11e(name, port, &hints, &res);
    if (val != 0)
    {   /* TODO: C locale for gai_strerror() */
        msg_Err(creds, "cannot resolve %s port %u: %s", name, port,
                gai_strerror(val));
        return NULL;
    }

    for (const struct addrinfo *p = res; p != NULL; p = p->ai_next)
    {
        heimdall_tls_t *tcp = heimdall_tls_SocketOpenAddrInfo(p, true);
        if (tcp == NULL)
        {
            msg_Err(creds, "socket error: %s", heimdall_strerror_c(errno));
            continue;
        }

        heimdall_tls_t *tls = heimdall_tls_ClientSessionCreate(creds, tcp, name, service,
                                                     alpn, alp);
        if (tls != NULL)
        {   /* Success! */
            freeaddrinfo(res);
            return tls;
        }

        msg_Err(creds, "connection error: %s", heimdall_strerror_c(errno));
        heimdall_tls_SessionDelete(tcp);
    }

    /* Failure! */
    freeaddrinfo(res);
    return NULL;
}
