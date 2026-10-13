/*****************************************************************************
 * specific.c: stubs for POSIX OS-specific initialization
 *****************************************************************************
 * Copyright © 2008 Rémi Denis-Courmont
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

#include <heimdall_common.h>
#include "../libheimdall.h"
#include "../lib/libheimdall_internal.h"

#ifdef HAVE_DBUS
/* used for one-instance mode */
# include <dbus/dbus.h>
# include <heimdall_url.h>
#endif

void system_Init (void)
{
}

static void system_ConfigureDbus(libheimdall_int_t *heimdall, int argc,
                                 const char *const argv[])
{
/* FIXME: could be replaced by using Unix sockets */
#ifdef HAVE_DBUS
# define MPRIS_APPEND "/org/mpris/MediaPlayer2/TrackList/Append"
# define MPRIS_BUS_NAME "org.mpris.MediaPlayer2.heimdall"
# define MPRIS_OBJECT_PATH "/org/mpris/MediaPlayer2"
# define MPRIS_TRACKLIST_INTERFACE "org.mpris.MediaPlayer2.TrackList"

    dbus_threads_init_default();

    if (var_InheritBool(heimdall, "dbus"))
        libheimdall_InternalAddIntf(heimdall, "dbus,none");

    if (!var_InheritBool(heimdall, "one-instance")
     && !(var_InheritBool(heimdall, "one-instance-when-started-from-file")
       && var_InheritBool(heimdall, "started-from-file")))
         return;

    for (int i = 0; i < argc; i++)
        if (argv[i][0] == ':')
        {
            msg_Err(heimdall, "item option %s incompatible with single instance",
                    argv[i]);
            return;
        }

    char *name = var_GetString(heimdall, "dbus-mpris-name");
    if (name != NULL)
    {
        bool singleton = !strcmp(name, MPRIS_BUS_NAME);
        free(name);
        if (singleton)
        {
            msg_Dbg(heimdall, "no running Heimdall instance - continuing normally...");
            return; /* This is the single instance */
        }
    }

    /* Initialise D-Bus interface, check for other instances */
    DBusError err;
    dbus_error_init(&err);

    /* connect to the session bus */
    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (conn == NULL)
    {
        msg_Err(heimdall, "D-Bus session bus connection failure: %s",
                 err.message);
        dbus_error_free(&err);
        return;
    }

    msg_Warn(heimdall, "running Heimdall instance - exiting...");

    const dbus_bool_t play = !var_InheritBool(heimdall, "playlist-enqueue");

    for (int i = 0; i < argc; i++)
    {
        DBusMessage *req = dbus_message_new_method_call(MPRIS_BUS_NAME,
            MPRIS_OBJECT_PATH, MPRIS_TRACKLIST_INTERFACE, "AddTrack");
        if (unlikely(req == NULL))
            continue;

        /* We need to resolve relative paths in this instance */
        char *mrlbuf = NULL;
        const char *mrl;

        if (strstr(argv[i], "://"))
            mrl = argv[i];
        else
        {
            mrlbuf = heimdall_path2uri(argv[i], NULL);
            if (unlikely(mrlbuf == NULL))
            {
                dbus_message_unref(req);
                continue;
            }
            mrl = mrlbuf;
        }

        /* append MRLs */
        msg_Dbg(heimdall, "adding track %s to running instance", mrl);

        const char *after_track = MPRIS_APPEND;
        dbus_bool_t ok = dbus_message_append_args(req, DBUS_TYPE_STRING, &mrl,
                                           DBUS_TYPE_OBJECT_PATH, &after_track,
                                           DBUS_TYPE_BOOLEAN, &play,
                                           DBUS_TYPE_INVALID);
        free(mrlbuf);
        if (unlikely(!ok))
        {
             dbus_message_unref(req);
             continue;
        }

        /* send message and get a handle for a reply */
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn,
                                                                req, -1, &err);
        dbus_message_unref(req);
        if (reply == NULL)
        {
            msg_Err(heimdall, "D-Bus error: %s", err.message);
            dbus_error_free(&err);
            continue;
        }
        dbus_message_unref(reply);
    }

    /* we unreference the connection when we've finished with it */
    dbus_connection_unref(conn);
    exit(0);
#else
    (void) heimdall; (void) argc; (void) argv;
#endif // HAVE_DBUS
}

void system_Configure(libheimdall_int_t *libheimdall,
                      int argc, const char *const argv[])
{
    system_ConfigureDbus(libheimdall, argc, argv);
}
