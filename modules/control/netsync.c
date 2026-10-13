/*****************************************************************************
 * netsync.c: synchronization between several network clients.
 *****************************************************************************
 * Copyright (C) 2004-2009 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
 *          Jean-Paul Saman <jpsaman@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <assert.h>

#define HEIMDALL_MODULE_LICENSE HEIMDALL_LICENSE_GPL_2_PLUS
#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_plugin.h>
#include <heimdall_interface.h>
#include <heimdall_playlist_legacy.h>
#include <heimdall_poll.h>

#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_POLL_H
#   include <poll.h>
#endif

#include <heimdall_network.h>

#define NETSYNC_PORT 9875

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open (heimdall_object_t *);
static void Close(heimdall_object_t *);

#define NETSYNC_TEXT N_("Network synchronisation" )
#define NETSYNC_LONGTEXT N_( "This allows you to remotely " \
        "synchronise clocks for server and client. The detailed settings " \
        "are available in Advanced / Network Sync." )

#define NETSYNC_TEXT N_("Network master clock")
#define NETSYNC_LONGTEXT N_("When set, " \
  "this Heimdall instance will act as the master clock for synchronization " \
  "for clients listening")

#define MIP_TEXT N_("Master server IP address")
#define MIP_LONGTEXT N_("The IP address of " \
  "the network master clock to use for clock synchronization.")

#define NETSYNC_TIMEOUT_TEXT N_("UDP timeout (in ms)")
#define NETSYNC_TIMEOUT_LONGTEXT N_("Length of time (in ms) " \
  "until aborting data reception.")

static void AutoRun(libheimdall_int_t *libheimdall)
{
    if (var_InheritBool(libheimdall, "network-synchronisation"))
        intf_Create(libheimdall, MODULE_STRING);
}

heimdall_module_begin()
    set_shortname(N_("Network Sync"))
    set_description(N_("Network synchronization"))
    set_subcategory(SUBCAT_ADVANCED_MISC)

    add_bool("network-synchronisation", false, NETSYNC_TEXT, NETSYNC_LONGTEXT)
    add_bool("netsync-master", false,
              NETSYNC_TEXT, NETSYNC_LONGTEXT)
    add_string("netsync-master-ip", NULL, MIP_TEXT, MIP_LONGTEXT)
    add_integer("netsync-timeout", 500,
                 NETSYNC_TIMEOUT_TEXT, NETSYNC_TIMEOUT_LONGTEXT)

    set_capability("interface", 0)
    set_callbacks(Open, Close)

    add_submodule()
    set_capability("autorun", 40)
    set_callback(AutoRun)
heimdall_module_end()

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
struct intf_sys_t {
    int            fd;
    int            timeout;
    bool           is_master;
    playlist_t     *playlist;

    /* */
    input_thread_t *input;
    heimdall_thread_t   thread;
};

static int PlaylistEvent(heimdall_object_t *, char const *cmd,
                         heimdall_value_t oldval, heimdall_value_t newval, void *data);

/*****************************************************************************
 * Activate: initialize and create stuff
 *****************************************************************************/
static int Open(heimdall_object_t *object)
{
    intf_thread_t *intf = (intf_thread_t*)object;
    intf_sys_t    *sys;
    int fd;

    if (!var_InheritBool(intf, "netsync-master")) {
        char *psz_master = var_InheritString(intf, "netsync-master-ip");
        if (psz_master == NULL) {
            msg_Err(intf, "master address not specified");
            return HEIMDALL_EGENERIC;
        }
        fd = net_ConnectUDP(HEIMDALL_OBJECT(intf), psz_master, NETSYNC_PORT, -1);
        free(psz_master);
    } else {
        fd = net_ListenUDP1(HEIMDALL_OBJECT(intf), NULL, NETSYNC_PORT);
    }

    if (fd == -1) {
        msg_Err(intf, "Netsync socket failure");
        return HEIMDALL_EGENERIC;
    }

    intf->p_sys = sys = malloc(sizeof(*sys));
    if (!sys) {
        net_Close(fd);
        return HEIMDALL_ENOMEM;
    }

    sys->fd = fd;
    sys->is_master = var_InheritBool(intf, "netsync-master");
    sys->timeout = var_InheritInteger(intf, "netsync-timeout");
    if (sys->timeout < 500)
        sys->timeout = 500;
    sys->playlist = pl_Get(intf);
    sys->input = NULL;

    var_AddCallback(sys->playlist, "input-current", PlaylistEvent, intf);
    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * Close: destroy interface
 *****************************************************************************/
void Close(heimdall_object_t *object)
{
    intf_thread_t *intf = (intf_thread_t*)object;
    intf_sys_t *sys = intf->p_sys;

    var_DelCallback(sys->playlist, "input-current", PlaylistEvent, intf);

    if (sys->input != NULL) {
        heimdall_cancel(sys->thread);
        heimdall_join(sys->thread, NULL);
    }

    net_Close(sys->fd);
    free(sys);
}

static heimdall_tick_t GetPcrSystem(input_thread_t *input)
{
    int canc = heimdall_savecancel();
    /* TODO use the delay */
    heimdall_tick_t system;
    if (input_GetPcrSystem(input, &system, NULL))
        system = -1;
    heimdall_restorecancel(canc);

    return system;
}

static void *Master(void *handle)
{
    intf_thread_t *intf = handle;
    intf_sys_t *sys = intf->p_sys;
    heimdall_thread_set_name("heimdall-netsyncboss");
    for (;;) {
        struct pollfd ufd = { .fd = sys->fd, .events = POLLIN, };
        uint64_t data[2];

        if (poll(&ufd, 1, -1) < 0)
            continue;

        /* We received something */
        heimdall_sockaddr from;
        socklen_t fromlen = sizeof (from.ss);

        if (recvfrom(sys->fd, data, 8, 0, &from.sa, &fromlen) < 8)
            continue;

        heimdall_tick_t master_system = GetPcrSystem(sys->input);
        if (master_system < 0)
            continue;

        data[0] = hton64(heimdall_tick_now());
        data[1] = hton64(master_system);

        /* Reply to the sender */
        sendto(sys->fd, data, 16, 0, &from.sa, fromlen);
#if 0
        /* not sure we need the client information to sync,
           since we are the master anyway */
        heimdall_tick_t client_system = ntoh64(data[0]);
        msg_Dbg(intf, "Master clockref: %"PRId64" -> %"PRId64", from %s "
                 "(date: %"PRId64")", client_system, master_system,
                 (from.ss_family == AF_INET) ? inet_ntoa(((struct sockaddr_in *)&from)->sin_addr)
                 : "non-IPv4", /*date*/ 0);
#endif
    }
    return NULL;
}

static void *Slave(void *handle)
{
    intf_thread_t *intf = handle;
    intf_sys_t *sys = intf->p_sys;

    heimdall_thread_set_name("heimdall-netsyncrecv");

    for (;;) {
        struct pollfd ufd = { .fd = sys->fd, .events = POLLIN, };
        uint64_t data[2];

        heimdall_tick_t system = GetPcrSystem(sys->input);
        if (system < 0)
            goto wait;

        /* Send clock request to the master */
        const heimdall_tick_t send_date = heimdall_tick_now();

        data[0] = hton64(system);
        send(sys->fd, data, 8, 0);

        /* Don't block */
        if (poll(&ufd, 1, sys->timeout) <= 0)
            continue;

        const heimdall_tick_t receive_date = heimdall_tick_now();
        if (recv(sys->fd, data, 16, 0) < 16)
            goto wait;

        const heimdall_tick_t master_date   = ntoh64(data[0]);
        const heimdall_tick_t master_system = ntoh64(data[1]);
        const heimdall_tick_t diff_date = receive_date -
                                  ((receive_date - send_date) / 2 + master_date);

        if (master_system > 0) {
            int canc = heimdall_savecancel();

            heimdall_tick_t client_system;
            if (!input_GetPcrSystem(sys->input, &client_system, NULL)) {
                const heimdall_tick_t diff_system = client_system - master_system - diff_date;
                if (diff_system != 0) {
                    input_ModifyPcrSystem(sys->input, true, master_system - diff_date);
#if 0
                    msg_Dbg(intf, "Slave clockref: %"PRId64" -> %"PRId64" -> %"PRId64","
                             " clock diff: %"PRId64", diff: %"PRId64"",
                             system, master_system, client_system,
                             diff_system, diff_date);
#endif
                }
            }
            heimdall_restorecancel(canc);
        }
    wait:
        heimdall_tick_sleep(INTF_IDLE_SLEEP);
    }
    return NULL;
}

static int PlaylistEvent(heimdall_object_t *object, char const *cmd,
                         heimdall_value_t oldval, heimdall_value_t newval, void *data)
{
    HEIMDALL_UNUSED(cmd); HEIMDALL_UNUSED(object);
    intf_thread_t  *intf = data;
    intf_sys_t     *sys = intf->p_sys;
    input_thread_t *input = newval.p_address;

    if (sys->input != NULL) {
        msg_Err(intf, "InputEvent DEAD");
        assert(oldval.p_address == sys->input);

        heimdall_cancel(sys->thread);
        heimdall_join(sys->thread, NULL);
    }

    sys->input = input;

    if (input != NULL) {
        if (heimdall_clone(&sys->thread, sys->is_master ? Master : Slave, intf))
            sys->input = NULL;
    }
    return HEIMDALL_SUCCESS;
}

