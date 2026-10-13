/**
 * @file input.c
 * @brief RTP packet input
 */
/*****************************************************************************
 * Copyright © 2008 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <limits.h>
#include <errno.h>
#ifdef HAVE_POLL_H
# include <poll.h>
#endif
#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif
#ifdef _WIN32
# include <winsock2.h>
#endif

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_poll.h>
#include <heimdall_demux.h>
#include <heimdall_block.h>
#include "heimdall_dtls.h"

#include "rtp.h"
#ifdef HAVE_SRTP
# include "srtp.h"
#endif
#include "input.h"

#define DEFAULT_MRU (1500u - (20 + 8))

/**
 * Processes a packet received from the RTP socket.
 */
static void rtp_process (struct heimdall_logger *logger, rtp_input_sys_t *sys,
                         rtp_session_t *session, block_t *block)
{
    if (block->i_buffer < 2)
        goto drop;
    const uint8_t ptype = rtp_ptype (block);
    if (ptype >= 72 && ptype <= 76)
        goto drop; /* Muxed RTCP, ignore for now FIXME */

#ifdef HAVE_SRTP
    if (sys->srtp != NULL)
    {
        size_t len = block->i_buffer;
        if (srtp_recv (sys->srtp, block->p_buffer, &len))
        {
            heimdall_debug (logger, "SRTP authentication/decryption failed");
            goto drop;
        }
        block->i_buffer = len;
    }
#endif

    rtp_queue (logger, session, block);
    return;
drop:
    block_Release (block);
}

static int rtp_timeout (heimdall_tick_t deadline)
{
    if (deadline == HEIMDALL_TICK_INVALID)
        return -1; /* infinite */

    heimdall_tick_t t = heimdall_tick_now ();
    if (t >= deadline)
        return 0;

    t = (deadline - t) / (CLOCK_FREQ / INT64_C(1000));
    if (unlikely(t > INT_MAX))
        return INT_MAX;
    return t;
}

/**
 * RTP/RTCP session thread for datagram sockets
 */
void *rtp_dgram_thread (void *opaque)
{
    rtp_sys_t *sys = opaque;
    heimdall_tick_t deadline = HEIMDALL_TICK_INVALID;
    struct heimdall_dtls *rtp_sock = sys->input_sys.rtp_sock;

    heimdall_thread_set_name("heimdall-rtp");

    for (;;)
    {
        struct pollfd ufd[1];

        ufd[0].events = POLLIN;
        ufd[0].fd = heimdall_dtls_GetPollFD(rtp_sock, &ufd[0].events);

        int n = poll (ufd, 1, rtp_timeout (deadline));
        if (n == -1)
            continue;

        int canc = heimdall_savecancel ();
        if (n == 0)
            goto dequeue;

        if (ufd[0].revents)
        {
            block_t *block = block_Alloc(DEFAULT_MRU);
            if (unlikely(block == NULL))
                break; /* we are totallly screwed */

            bool truncated;
            ssize_t len = heimdall_dtls_Recv(rtp_sock, block->p_buffer,
                                       block->i_buffer, &truncated);
            if (len >= 0) {
                if (truncated) {
                    heimdall_error (sys->logger, "packet truncated (MRU was %zu)",
                            block->i_buffer);
                    block->i_flags |= BLOCK_FLAG_CORRUPTED;
                }
                else
                    block->i_buffer = len;

                rtp_process (sys->logger, &sys->input_sys, sys->session, block);
            }
            else
            {
                if (errno == EPIPE)
                    break; /* connection terminated */
                heimdall_warning (sys->logger, "RTP network error: %s",
                          heimdall_strerror_c(errno));
                block_Release (block);
            }

            n--;
        }

    dequeue:
        if (!rtp_dequeue (sys->logger, sys->session, heimdall_tick_now(), &deadline))
            deadline = HEIMDALL_TICK_INVALID;
        heimdall_restorecancel (canc);
    }
    return NULL;
}
