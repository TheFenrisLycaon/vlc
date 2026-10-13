/*****************************************************************************
 * posix/getaddrinfo.c: interruptible DNS resolution for POSIX
 *****************************************************************************
 * Copyright (C) 2016 Rémi Denis-Courmont
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
#include <stdio.h>
#include <netdb.h>

#include <heimdall_common.h>
#include <heimdall_interrupt.h>
#include <heimdall_network.h>

struct heimdall_gai_req
{
    const char *name;
    const char *service;
    const struct addrinfo *hints;
    struct addrinfo **result;
    int error;
    heimdall_sem_t done;
};

static void *heimdall_gai_thread(void *data)
{
    heimdall_thread_set_name("heimdall-getaddrinfo");

    struct heimdall_gai_req *req = data;

    req->error = EAI_SYSTEM;
    req->error = getaddrinfo(req->name, req->service, req->hints, req->result);
    heimdall_sem_post(&req->done);
    return NULL;
}

int heimdall_getaddrinfo_i11e(const char *name, unsigned port,
                         const struct addrinfo *hints,
                         struct addrinfo **res)
{
    struct heimdall_gai_req req =
    {
        .name = name,
        .service = NULL,
        .hints = hints,
        .result = res,
    };
    char portbuf[6];
    heimdall_thread_t th;

    if (port != 0)
    {
        if ((size_t)snprintf(portbuf, sizeof (portbuf), "%u",
                             port) >= sizeof (portbuf))
            return EAI_NONAME;

        req.service = portbuf;
    }

    heimdall_sem_init(&req.done, 0);

    if (heimdall_clone(&th, heimdall_gai_thread, &req))
        return EAI_SYSTEM;

    heimdall_sem_wait_i11e(&req.done);

    heimdall_cancel(th);
    heimdall_join(th, NULL);

    return req.error;
}
