/*****************************************************************************
 * interface.c: interface access for other threads
 * This library provides basic functions for threads to interact with user
 * interface, such as command line.
 *****************************************************************************
 * Copyright (C) 1998-2007 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
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
 *   \file
 *   This file contains functions related to interface management
 */


/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>

#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_modules.h>
#include <heimdall_interface.h>
#include <heimdall_playlist.h>
#include "../libheimdall.h"
#include "../lib/libheimdall_internal.h"

static int AddIntfCallback( heimdall_object_t *, char const *,
                            heimdall_value_t , heimdall_value_t , void * );

heimdall_playlist_t *
heimdall_intf_GetMainPlaylist(intf_thread_t *intf)
{
    heimdall_playlist_t *pl = libheimdall_GetMainPlaylist(heimdall_object_instance(intf));
    assert(pl);
    return pl;
}

/**
 * Create and start an interface.
 *
 * @param libheimdall the private libheimdall instance starting the interface
 * @param chain configuration chain string
 * @return HEIMDALL_SUCCESS or an error code
 */
int intf_Create( libheimdall_int_t *libheimdall, const char *chain )
{
    assert( libheimdall );
    libheimdall_priv_t *priv = libheimdall_priv(libheimdall);

    /* Ensure that each interfaces can access the main playlist */
    if (libheimdall_GetMainPlaylist(libheimdall) == NULL)
        return HEIMDALL_ENOMEM;

    /* Allocate structure */
    intf_thread_t *p_intf = heimdall_custom_create( libheimdall, sizeof( *p_intf ),
                                               "interface" );
    if( unlikely(p_intf == NULL) )
        return HEIMDALL_ENOMEM;

    /* Variable used for interface spawning */
    heimdall_value_t val;
    var_Create( p_intf, "intf-add", HEIMDALL_VAR_STRING | HEIMDALL_VAR_ISCOMMAND );
    var_Change( p_intf, "intf-add", HEIMDALL_VAR_SETTEXT, _("Add Interface") );
#if defined(HAVE_ISATTY)
    if( isatty( 0 ) )
#endif
    {
        val.psz_string = (char *)"rc,none";
        var_Change( p_intf, "intf-add", HEIMDALL_VAR_ADDCHOICE, val, _("Console") );
    }
    val.psz_string = (char *)"telnet,none";
    var_Change( p_intf, "intf-add", HEIMDALL_VAR_ADDCHOICE, val, _("Telnet") );
    val.psz_string = (char *)"http,none";
    var_Change( p_intf, "intf-add", HEIMDALL_VAR_ADDCHOICE, val, _("Web") );
#ifdef ENABLE_UPNP_SERVER
    val.psz_string = (char *)"upnp,none";
    var_Change( p_intf, "intf-add", HEIMDALL_VAR_ADDCHOICE, val, _("Upnp") );
#endif
    val.psz_string = (char *)"gestures,none";
    var_Change( p_intf, "intf-add", HEIMDALL_VAR_ADDCHOICE, val,
                _("Mouse Gestures") );

    var_AddCallback( p_intf, "intf-add", AddIntfCallback, NULL );

    /* Choose the best module */
    char *module;

    p_intf->p_cfg = NULL;
    free( config_ChainCreate( &module, &p_intf->p_cfg, chain ) );
    p_intf->p_module = module_need( p_intf, "interface", module, true );
    free(module);
    if( p_intf->p_module == NULL )
    {
        msg_Err( p_intf, "no suitable interface module" );
        goto error;
    }

    heimdall_mutex_lock(&priv->lock);
    p_intf->p_next = priv->interfaces;
    priv->interfaces = p_intf;
    heimdall_mutex_unlock(&priv->lock);

    return HEIMDALL_SUCCESS;

error:
    if( p_intf->p_module )
        module_unneed( p_intf, p_intf->p_module );
    config_ChainDestroy( p_intf->p_cfg );
    heimdall_object_delete(p_intf);
    return HEIMDALL_EGENERIC;
}

/**
 * Inserts an item in the playlist.
 *
 * This function is used during initialization. It inserts an item to the
 * beginning of the playlist. That is meant to compensate for the reverse
 * parsing order of the command line.
 */
int intf_InsertItem(libheimdall_int_t *libheimdall, const char *mrl, unsigned optc,
                    const char *const *optv, unsigned flags)
{
    input_item_t *item = input_item_New(mrl, NULL);

    if (unlikely(item == NULL))
        return -1;

    int ret = -1;

    if (input_item_AddOptions(item, optc, optv, flags) == HEIMDALL_SUCCESS)
    {
        heimdall_playlist_t *playlist = libheimdall_GetMainPlaylist(libheimdall);
        if (playlist)
        {
            heimdall_playlist_Lock(playlist);
            ret = heimdall_playlist_InsertOne(playlist, 0, item);
            heimdall_playlist_Unlock(playlist);
        }
    }
    input_item_Release(item);
    return ret;
}

void libheimdall_InternalPlay(libheimdall_int_t *libheimdall)
{
    if (!var_InheritBool(HEIMDALL_OBJECT(libheimdall), "playlist-autostart"))
        return;
    heimdall_playlist_t *playlist = libheimdall_GetMainPlaylist(libheimdall);
    if (!playlist)
        return;
    heimdall_playlist_Lock(playlist);
    if (heimdall_playlist_Count(playlist) > 0)
    {
        if (heimdall_playlist_GetCurrentIndex(playlist) < 0)
            heimdall_playlist_GoTo(playlist, 0);
        heimdall_playlist_Start(playlist);
    }
    heimdall_playlist_Unlock(playlist);
}

static void libheimdall_AutoRun(libheimdall_int_t *libheimdall)
{
    struct heimdall_logger *log = libheimdall->obj.logger;
    module_t **mods;
    ssize_t total = heimdall_module_match("autorun", NULL, false, &mods, NULL);

    for (ssize_t i = 0; i < total; i++) {
        void (*func)(libheimdall_int_t *) = heimdall_module_map(log, mods[i]);

        assert(func != NULL);
        func(libheimdall);
    }

    free(mods);
}

/**
 * Starts an interface plugin.
 */
int libheimdall_InternalAddIntf(libheimdall_int_t *libheimdall, const char *name)
{
    int ret;

    if (name != NULL)
        ret = intf_Create(libheimdall, name);
    else
    {   /* Default interface */
        libheimdall_AutoRun(libheimdall);

        char *intf = var_InheritString(libheimdall, "intf");
        if (intf == NULL) /* "intf" has not been set */
            msg_Info(libheimdall, _("Running heimdall with the default interface. "
                     "Use 'cheimdall' to use heimdall without interface."));

        ret = intf_Create(libheimdall, intf);
        free(intf);
        name = "default";
    }
    if (ret != HEIMDALL_SUCCESS)
        msg_Err(libheimdall, "interface \"%s\" initialization failed", name);
    return ret;
}

/**
 * Stops and destroys all interfaces, then the playlist.
 * @warning FIXME
 * @param libheimdall the LibHeimdall instance
 */
void intf_DestroyAll(libheimdall_int_t *libheimdall)
{
    libheimdall_priv_t *priv = libheimdall_priv(libheimdall);

    heimdall_mutex_lock(&priv->lock);
    intf_thread_t *intf, **pp = &priv->interfaces;

    while ((intf = *pp) != NULL)
    {
        *pp = intf->p_next;
        heimdall_mutex_unlock(&priv->lock);

        module_unneed(intf, intf->p_module);
        config_ChainDestroy(intf->p_cfg);
        var_DelCallback(intf, "intf-add", AddIntfCallback, NULL);
        heimdall_object_delete(intf);

        heimdall_mutex_lock(&priv->lock);
    }
    heimdall_mutex_unlock(&priv->lock);
}

/* Following functions are local */

static int AddIntfCallback( heimdall_object_t *obj, char const *var,
                            heimdall_value_t old, heimdall_value_t cur, void *data )
{
    int ret = intf_Create( heimdall_object_instance(obj), cur.psz_string );
    if( ret )
        msg_Err( obj, "interface \"%s\" initialization failed",
                 cur.psz_string );

    (void) var; (void) old; (void) data;
    return ret;
}
