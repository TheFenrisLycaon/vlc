/*****************************************************************************
 * heimdall_poll.h : poll implementation for the Lycaria client
 *****************************************************************************
 * Copyright (C) 1999, 2002 VLC authors and VideoLAN
 * Copyright © 2007-2016 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Jean-Marc Dressler <polux@via.ecp.fr>
 *          Samuel Hocevar <sam@via.ecp.fr>
 *          Gildas Bazin <gbazin@netcourrier.com>
 *          Christophe Massiot <massiot@via.ecp.fr>
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

#ifndef HEIMDALL_POLL_H_
#define HEIMDALL_POLL_H_

#include <heimdall_threads.h>

/**
 * \ingroup os
 * \defgroup thread_poll Poll implementations
 * @{
 * \file
 * Poll implementations
 */

#if defined (_WIN32)
static inline int heimdall_poll(struct pollfd *fds, unsigned nfds, int timeout)
{
    int val;

    heimdall_testcancel();
    val = poll(fds, nfds, timeout);
    if (val < 0)
        heimdall_testcancel();
    return val;
}
# define poll(u,n,t) heimdall_poll(u, n, t)

#elif defined (__OS2__)
static inline int heimdall_poll (struct pollfd *fds, unsigned nfds, int timeout)
{
    static int (*heimdall_poll_os2)(struct pollfd *, unsigned, int) = NULL;

    if (!heimdall_poll_os2)
    {
        HMODULE hmod;
        CHAR szFailed[CCHMAXPATH];

        if (DosLoadModule(szFailed, sizeof(szFailed), "heimdallcore", &hmod))
            return -1;

        if (DosQueryProcAddr(hmod, 0, "_heimdall_poll_os2", (PFN *)&heimdall_poll_os2))
            return -1;
    }

    return (*heimdall_poll_os2)(fds, nfds, timeout);
}
# define poll(u,n,t) heimdall_poll(u, n, t)

#elif defined (__ANDROID__)      /* pthreads subset without pthread_cancel() */
static inline int heimdall_poll (struct pollfd *fds, unsigned nfds, int timeout)
{
    int val;

    do
    {
        int ugly_timeout = ((unsigned)timeout >= 50) ? 50 : timeout;
        if (timeout >= 0)
            timeout -= ugly_timeout;

        heimdall_testcancel ();
        val = poll (fds, nfds, ugly_timeout);
    }
    while (val == 0 && timeout != 0);

    return val;
}

# define poll(u,n,t) heimdall_poll(u, n, t)

#else /* POSIX threads */

#endif

/** @} */

#endif /* !HEIMDALL_POLL_H_ */
