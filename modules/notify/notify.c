/*****************************************************************************
 * notify.c : libnotify notification plugin
 *****************************************************************************
 * Copyright (C) 2006-2009 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Christophe Mutricy <xtophe -at- lycaria -dot- org>
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

#define HEIMDALL_MODULE_LICENSE HEIMDALL_LICENSE_GPL_2_PLUS
#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_plugin.h>
#include <heimdall_interface.h>
#include <heimdall_player.h>
#include <heimdall_playlist.h>
#include <heimdall_url.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libnotify/notify.h>

typedef struct GtkIconTheme GtkIconTheme;
enum GtkIconLookupFlags { dummy = 0x7fffffff };

HEIMDALL_WEAK GtkIconTheme *gtk_icon_theme_get_default(void);
HEIMDALL_WEAK GdkPixbuf *gtk_icon_theme_load_icon(GtkIconTheme *,
    const char *icon_name, int size, enum GtkIconLookupFlags, GError **);

#ifndef NOTIFY_CHECK_VERSION
# define NOTIFY_CHECK_VERSION(x,y,z) 0
#endif

/*****************************************************************************
 * Module descriptor
 ****************************************************************************/
static int  Open    ( heimdall_object_t * );
static void Close   ( heimdall_object_t * );

#define APPLICATION_NAME "Heimdall media player"

#define TIMEOUT_TEXT N_("Timeout (ms)")
#define TIMEOUT_LONGTEXT N_("How long the notification will be displayed.")

heimdall_module_begin ()
    set_subcategory( SUBCAT_INTERFACE_CONTROL )
    set_shortname( N_( "Notify" ) )
    set_description( N_("LibNotify Notification Plugin") )

    add_integer( "notify-timeout", 4000,
                 TIMEOUT_TEXT, TIMEOUT_LONGTEXT )

    set_capability( "interface", 0 )
    set_callbacks( Open, Close )
heimdall_module_end ()


/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static void on_current_media_changed(heimdall_player_t *player,
                                     input_item_t *new_media, void *data);
static int Notify( heimdall_object_t *, const char *, GdkPixbuf *, intf_thread_t * );
#define MAX_LENGTH 256

struct intf_sys_t
{
    NotifyNotification *notification;
    heimdall_mutex_t     lock;
    bool            b_has_actions;
    heimdall_playlist_t  *playlist;
    struct heimdall_player_listener_id *player_listener;
};

static void foreach_g_free(void *data, void *userdata)
{
    g_free(data);
    HEIMDALL_UNUSED(userdata);
}

/*****************************************************************************
 * Open: initialize and create stuff
 *****************************************************************************/
static int Open( heimdall_object_t *p_this )
{
    intf_thread_t   *p_intf = (intf_thread_t *)p_this;
    intf_sys_t      *p_sys  = malloc( sizeof( *p_sys ) );

    if( !p_sys )
        return HEIMDALL_ENOMEM;

    p_sys->playlist = heimdall_intf_GetMainPlaylist(p_intf);
    heimdall_player_t *player = heimdall_playlist_GetPlayer(p_sys->playlist);
    static const struct heimdall_player_cbs player_cbs =
    {
        .on_current_media_changed = on_current_media_changed
    };
    heimdall_player_Lock(player);
    p_sys->player_listener = heimdall_player_AddListener(player, &player_cbs, p_intf);
    heimdall_player_Unlock(player);
    if (!p_sys->player_listener)
    {
        free(p_sys);
        return HEIMDALL_EGENERIC;
    }

    if( !notify_init( APPLICATION_NAME ) )
    {
        heimdall_player_Lock(player);
        heimdall_player_RemoveListener(player, p_sys->player_listener);
        heimdall_player_Unlock(player);
        free( p_sys );
        msg_Err( p_intf, "can't find notification daemon" );
        return HEIMDALL_EGENERIC;
    }
    p_intf->p_sys = p_sys;

    heimdall_mutex_init( &p_sys->lock );
    p_sys->notification = NULL;
    p_sys->b_has_actions = false;

    GList *p_caps = notify_get_server_caps ();
    if( p_caps )
    {
        for( GList *c = p_caps; c != NULL; c = c->next )
        {
            if( !strcmp( (char*)c->data, "actions" ) )
            {
                p_sys->b_has_actions = true;
                break;
            }
        }
        g_list_foreach( p_caps, foreach_g_free, NULL );
        g_list_free( p_caps );
    }

    return HEIMDALL_SUCCESS;
}

/*****************************************************************************
 * Close: destroy interface stuff
 *****************************************************************************/
static void Close( heimdall_object_t *p_this )
{
    intf_thread_t   *p_intf = ( intf_thread_t* ) p_this;
    intf_sys_t      *p_sys  = p_intf->p_sys;

    heimdall_player_t *player = heimdall_playlist_GetPlayer(p_sys->playlist);
    heimdall_player_Lock(player);
    heimdall_player_RemoveListener(player, p_sys->player_listener);
    heimdall_player_Unlock(player);

    if( p_sys->notification )
    {
        GError *p_error = NULL;
        notify_notification_close( p_sys->notification, &p_error );
        g_object_unref( p_sys->notification );
    }

    free( p_sys );
    notify_uninit();
}

static void on_current_media_changed(heimdall_player_t *player,
                                     input_item_t *p_input_item, void *data)
{
    (void) player;
    intf_thread_t *p_intf = data;
    intf_sys_t *p_sys = p_intf->p_sys;
    char           psz_tmp[MAX_LENGTH];
    char           psz_notify[MAX_LENGTH];
    char           *psz_title;
    char           *psz_artist;
    char           *psz_album;
    char           *psz_arturl;

    if (!p_input_item)
        return;

    /* Checking for click on directories */
    if(p_input_item->i_type == ITEM_TYPE_DIRECTORY || p_input_item->i_type == ITEM_TYPE_PLAYLIST
        || p_input_item->i_type == ITEM_TYPE_NODE || p_input_item->i_type== ITEM_TYPE_UNKNOWN
        || p_input_item->i_type == ITEM_TYPE_CARD){
        return;
    }

    psz_title = input_item_GetTitleFbName( p_input_item );
    /* We need at least a title */
    if( EMPTY_STR( psz_title ) )
    {
        free( psz_title );
        return;
    }

    psz_artist = input_item_GetArtist( p_input_item );
    psz_album = input_item_GetAlbum( p_input_item );

    if( !EMPTY_STR( psz_artist ) )
    {
        if( !EMPTY_STR( psz_album ) )
            snprintf( psz_tmp, MAX_LENGTH, "<b>%s</b>\n%s\n[%s]",
                      psz_title, psz_artist, psz_album );
        else
            snprintf( psz_tmp, MAX_LENGTH, "<b>%s</b>\n%s",
                      psz_title, psz_artist );
    }
    else
        snprintf( psz_tmp, MAX_LENGTH, "<b>%s</b>", psz_title );

    free( psz_title );
    free( psz_artist );
    free( psz_album );

    GdkPixbuf *pix = NULL;
    psz_arturl = input_item_GetArtURL( p_input_item );

    if( psz_arturl )
    {
        char *psz = heimdall_uri2path( psz_arturl );
        free( psz_arturl );
        psz_arturl = psz;
    }

    if( psz_arturl )
    { /* scale the art to show it in notify popup */
        GError *p_error = NULL;
        pix = gdk_pixbuf_new_from_file_at_scale( psz_arturl,
                                                 72, 72, TRUE, &p_error );
        free( psz_arturl );
    }
    else
    /* else we show state-of-the art logo */
    if( gtk_icon_theme_get_default != NULL
     && gtk_icon_theme_load_icon != NULL )
    {
        /* First try to get an icon from the current theme. */
        GtkIconTheme* p_theme = gtk_icon_theme_get_default();
        pix = gtk_icon_theme_load_icon( p_theme, "heimdall", 72, 0, NULL);
    }
    else
    {   /* Load icon from share/ */
        GError *p_error = NULL;
        char *psz_pixbuf = config_GetSysPath(HEIMDALL_SYSDATA_DIR,
                                     "icons/hicolor/48x48/"PACKAGE_NAME".png");
        if (psz_pixbuf != NULL)
        {
            pix = gdk_pixbuf_new_from_file( psz_pixbuf, &p_error );
            free( psz_pixbuf );
        }
    }

    /* we need to replace '&' with '&amp;' because '&' is a keyword of
     * notification-daemon parser */
    const int i_len = strlen( psz_tmp );
    int i_notify = 0;
    for( int i = 0; i < i_len && i_notify < ( MAX_LENGTH - 5 ); i++ )
    { /* we use MAX_LENGTH - 5 because if the last char of psz_tmp is '&'
       * we will need 5 more characters: 'amp;\0' .
       * however that's unlikely to happen because the last char is '\0' */
        if( psz_tmp[i] != '&' )
        {
            psz_notify[i_notify] = psz_tmp[i];
        }
        else
        {
            strcpy( &psz_notify[i_notify], "&amp;" );
            i_notify += 4;
        }
        i_notify++;
    }
    psz_notify[i_notify] = '\0';

    heimdall_mutex_lock( &p_sys->lock );

    Notify( HEIMDALL_OBJECT(p_intf), psz_notify, pix, p_intf );

    heimdall_mutex_unlock( &p_sys->lock );
}

/* libnotify callback, called when the "Next" button is pressed */
static void Next( NotifyNotification *notification, gchar *psz, gpointer p )
{
    intf_thread_t *p_intf = (intf_thread_t *)p;
    intf_sys_t *p_sys = p_intf->p_sys;

    HEIMDALL_UNUSED(psz);
    notify_notification_close( notification, NULL );
    heimdall_playlist_Lock(  p_sys->playlist );
    heimdall_playlist_Next( p_sys->playlist );
    heimdall_playlist_Unlock(  p_sys->playlist );
}

/* libnotify callback, called when the "Previous" button is pressed */
static void Prev( NotifyNotification *notification, gchar *psz, gpointer p )
{
    intf_thread_t *p_intf = (intf_thread_t *)p;
    intf_sys_t *p_sys = p_intf->p_sys;

    HEIMDALL_UNUSED(psz);
    notify_notification_close( notification, NULL );
    heimdall_playlist_Lock(  p_sys->playlist );
    heimdall_playlist_Prev( p_sys->playlist );
    heimdall_playlist_Unlock(  p_sys->playlist );
}

static int Notify( heimdall_object_t *p_this, const char *psz_temp, GdkPixbuf *pix,
                   intf_thread_t *p_intf )
{
    intf_sys_t *p_sys = p_intf->p_sys;

    NotifyNotification * notification;

    /* Close previous notification if still active */
    if( p_sys->notification )
    {
        GError *p_error = NULL;
        notify_notification_close( p_sys->notification, &p_error );
        g_object_unref( p_sys->notification );
    }

    notification = notify_notification_new( _("Now Playing"),
                                            psz_temp, NULL
#if NOTIFY_CHECK_VERSION (0, 7, 0)
                                          );
#else
                                          , NULL );
#endif
    notify_notification_set_timeout( notification,
                                var_InheritInteger(p_this, "notify-timeout") );
    notify_notification_set_urgency( notification, NOTIFY_URGENCY_LOW );
    if( pix )
    {
        notify_notification_set_icon_from_pixbuf( notification, pix );
        g_object_unref( pix );
    }

    /* Adds previous and next buttons in the notification if actions are supported. */
    if( p_sys->b_has_actions )
    {
      notify_notification_add_action( notification, "previous", _("Previous"), Prev,
                                      (gpointer*)p_intf, NULL );
      notify_notification_add_action( notification, "next", _("Next"), Next,
                                      (gpointer*)p_intf, NULL );
    }

    notify_notification_show( notification, NULL);

    /* Stores the notification to be able to close it */
    p_sys->notification = notification;
    return HEIMDALL_SUCCESS;
}
