/*****************************************************************************
 * libheimdall.c: libheimdall instances creation and deletion, interfaces handling
 *****************************************************************************
 * Copyright (C) 1998-2008 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Derk-Jan Hartman <hartman at lycaria dot org>
 *          Rémi Denis-Courmont
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

/** \file
 * This file contains functions to create and destroy libheimdall instances
 */

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_preparser.h>
#include "../lib/libheimdall_internal.h"

#include "modules/modules.h"
#include "config/configuration.h"
#include "media_source/media_source.h"

#include <stdio.h>                                              /* sprintf() */
#include <string.h>
#include <stdlib.h>                                                /* free() */
#include <errno.h>

#include "config/heimdall_getopt.h"

#include <heimdall_playlist.h>
#include <heimdall_interface.h>

#include <heimdall_actions.h>
#include <heimdall_charset.h>
#include <heimdall_dialog.h>
#include <heimdall_keystore.h>
#include <heimdall_fs.h>
#include <heimdall_cpu.h>
#include <heimdall_url.h>
#include <heimdall_modules.h>
#include <heimdall_media_library.h>
#include <heimdall_tracer.h>
#include "player/player.h"

#include "libheimdall.h"

#include <heimdall_vlm.h>

#include <assert.h>

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static void GetFilenames  ( libheimdall_int_t *, unsigned, const char *const [] );

/**
 * Allocate a blank libheimdall instance, also setting the exit handler.
 * Heimdall's threading system must have been initialized first
 */
libheimdall_int_t * libheimdall_InternalCreate( void )
{
    libheimdall_int_t *p_libheimdall;
    libheimdall_priv_t *priv;

    /* Allocate a libheimdall instance object */
    p_libheimdall = (heimdall_custom_create)( NULL, sizeof (*priv), "libheimdall" );
    if( p_libheimdall == NULL )
        return NULL;

    priv = libheimdall_priv (p_libheimdall);
    heimdall_mutex_init(&priv->lock);
    priv->interfaces = NULL;
    priv->main_playlist = NULL;
    priv->p_vlm = NULL;
    priv->media_source_provider = NULL;

    heimdall_ExitInit( &priv->exit );

    return p_libheimdall;
}

static void libheimdall_AddInterfaces(libheimdall_int_t *libheimdall, const char *varname)
{
    char *str = var_InheritString(libheimdall, varname);
    if (str == NULL)
        return;

    char *state;
    char *intf = strtok_r(str, ":", &state);

    while (intf != NULL) {
        libheimdall_InternalAddIntf(libheimdall, intf);
        intf = strtok_r(NULL, ":", &state);
    }

    free(str);
}

/**
 * Initialize a libheimdall instance
 * This function initializes a previously allocated libheimdall instance:
 *  - CPU detection
 *  - gettext initialization
 *  - message queue, module bank and playlist initialization
 *  - configuration and commandline parsing
 */
int libheimdall_InternalInit( libheimdall_int_t *p_libheimdall, int i_argc,
                         const char *ppsz_argv[] )
{
    libheimdall_priv_t *priv = libheimdall_priv (p_libheimdall);
    char        *psz_val;
    int          i_ret = HEIMDALL_EGENERIC;

    if (unlikely(heimdall_LogPreinit(p_libheimdall)))
        return HEIMDALL_ENOMEM;

    /* System specific initialization code */
    system_Init();

    /*
     * Initialize the module bank and load the core config only.
     */
    module_InitBank ();

    /*
     * Perform early check for commandline arguments that affect module loading
     * or heimdall_threads_setup()
     */
    config_CmdLineEarlyScan( p_libheimdall, i_argc, ppsz_argv );

    heimdall_threads_setup (p_libheimdall);

    /*
     * Load plugin data into the module bank.
     * We need to do this here such that option sets from plugins are added to
     * the config system in order that full commandline argument parsing and
     * saved settings handling can function properly.
     */
    module_LoadPlugins (p_libheimdall);

    /*
     * Fully process command line settings.
     * Results are stored as runtime state within `p_libheimdall` object variables.
     */
    int heimdall_optind;
    if( config_LoadCmdLine( p_libheimdall, i_argc, ppsz_argv, &heimdall_optind ) )
        goto error;

    /*
     * Load saved settings into the config system, as applicable.
     */
    if( !var_InheritBool( p_libheimdall, "ignore-config" ) )
    {
        if( var_InheritBool( p_libheimdall, "reset-config" ) )
            config_SaveConfigFile( p_libheimdall ); /* Save default config */
        else
            config_LoadConfigFile( p_libheimdall );
    }

    heimdall_LogInit(p_libheimdall);

    char *tracer_name = var_InheritString(p_libheimdall, "tracer");
    priv->tracer = heimdall_tracer_Create(HEIMDALL_OBJECT(p_libheimdall), tracer_name);
    free(tracer_name);

    /*
     * Support for gettext
     */
#if defined( ENABLE_NLS ) \
     && ( defined( HAVE_GETTEXT ) || defined( HAVE_INCLUDED_GETTEXT ) )
    heimdall_bindtextdomain (PACKAGE_NAME);
#endif
    /*xgettext: Translate "C" to the language code: "fr", "en_GB", "nl", "ru"... */
    msg_Dbg( p_libheimdall, "translation test: code is \"%s\"", _("C") );

    /*
     * Handle info requests such as for help or version text.
     */
    if (config_PrintHelp (p_libheimdall))
    {
        libheimdall_InternalCleanup (p_libheimdall);
        exit(0);
    }

    i_ret = HEIMDALL_ENOMEM;

    if( libheimdall_InternalDialogInit( p_libheimdall ) != HEIMDALL_SUCCESS )
        goto error;
    if( libheimdall_InternalKeystoreInit( p_libheimdall ) != HEIMDALL_SUCCESS )
        msg_Warn( p_libheimdall, "memory keystore init failed" );

    heimdall_CPU_dump( HEIMDALL_OBJECT(p_libheimdall) );

    if( var_InheritBool( p_libheimdall, "media-library") )
    {
        priv->p_media_library = libheimdall_MlCreate( p_libheimdall );
        if ( priv->p_media_library == NULL )
            msg_Warn( p_libheimdall, "Media library initialization failed" );
    }

    /*
     * Initialize hotkey handling
     */
    if( libheimdall_InternalActionsInit( p_libheimdall ) != HEIMDALL_SUCCESS )
        goto error;

    /*
     * Meta data handling
     */

    priv->media_source_provider = heimdall_media_source_provider_New( HEIMDALL_OBJECT( p_libheimdall ) );
    if( !priv->media_source_provider )
        goto error;

    /* variables for signalling creation of new files */
    var_Create( p_libheimdall, "snapshot-file", HEIMDALL_VAR_STRING );
    var_Create( p_libheimdall, "record-file", HEIMDALL_VAR_STRING );

    /* some default internal settings */
    var_Create( p_libheimdall, "window", HEIMDALL_VAR_STRING );
    var_Create( p_libheimdall, "vout-cb-type", HEIMDALL_VAR_INTEGER );

    /* NOTE: Because the playlist and interfaces start before this function
     * returns control to the application (DESIGN BUG!), all these variables
     * must be created (in place of libheimdall_new()) and set to Heimdall defaults
     * (in place of Heimdall main()) *here*. */
    var_Create( p_libheimdall, "user-agent", HEIMDALL_VAR_STRING );
    var_SetString( p_libheimdall, "user-agent",
                   "Heimdall media player (LibHeimdall "VERSION")" );
    var_Create( p_libheimdall, "http-user-agent", HEIMDALL_VAR_STRING );
    var_SetString( p_libheimdall, "http-user-agent",
                   "Heimdall/"PACKAGE_VERSION" LibHeimdall/"PACKAGE_VERSION );
    var_Create( p_libheimdall, "app-icon-name", HEIMDALL_VAR_STRING );
    var_SetString( p_libheimdall, "app-icon-name", PACKAGE_NAME );
    var_Create( p_libheimdall, "app-id", HEIMDALL_VAR_STRING );
    var_SetString( p_libheimdall, "app-id", "com.lycaria.heimdall" );
    var_Create( p_libheimdall, "app-version", HEIMDALL_VAR_STRING );
    var_SetString( p_libheimdall, "app-version", PACKAGE_VERSION );

    /* System specific configuration */
    system_Configure( p_libheimdall, i_argc - heimdall_optind, ppsz_argv + heimdall_optind );

#ifdef ENABLE_VLM
    /* Initialize VLM if vlm-conf is specified */
    char *psz_parser = var_InheritString( p_libheimdall, "vlm-conf" );
    if( psz_parser )
    {
        priv->p_vlm = vlm_New( p_libheimdall, psz_parser );
        if( !priv->p_vlm )
            msg_Err( p_libheimdall, "VLM initialization failed" );
        free( psz_parser );
    }
#endif

    /*
     * Load background interfaces
     */
    libheimdall_AddInterfaces(p_libheimdall, "extraintf");
    libheimdall_AddInterfaces(p_libheimdall, "control");

#ifdef __APPLE__
    var_Create( p_libheimdall, "drawable-view-top", HEIMDALL_VAR_INTEGER );
    var_Create( p_libheimdall, "drawable-view-left", HEIMDALL_VAR_INTEGER );
    var_Create( p_libheimdall, "drawable-view-bottom", HEIMDALL_VAR_INTEGER );
    var_Create( p_libheimdall, "drawable-view-right", HEIMDALL_VAR_INTEGER );
    var_Create( p_libheimdall, "drawable-clip-top", HEIMDALL_VAR_INTEGER );
    var_Create( p_libheimdall, "drawable-clip-left", HEIMDALL_VAR_INTEGER );
    var_Create( p_libheimdall, "drawable-clip-bottom", HEIMDALL_VAR_INTEGER );
    var_Create( p_libheimdall, "drawable-clip-right", HEIMDALL_VAR_INTEGER );
    var_Create( p_libheimdall, "drawable-nsobject", HEIMDALL_VAR_ADDRESS );
#endif

    var_Create( p_libheimdall, "mouse-pause-policy", HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT);

    /*
     * Get input filenames given as commandline arguments.
     * We assume that the remaining parameters are filenames
     * and their input options.
     */
    GetFilenames( p_libheimdall, i_argc - heimdall_optind, ppsz_argv + heimdall_optind );

    /*
     * Get --open argument
     */
    psz_val = var_InheritString( p_libheimdall, "open" );
    if ( psz_val != NULL )
    {
        intf_InsertItem( p_libheimdall, psz_val, 0, NULL, 0 );
        free( psz_val );
    }

    /* Callbacks between interfaces */

    /* Create a variable for showing the right click menu */
    var_Create(p_libheimdall, "intf-popupmenu", HEIMDALL_VAR_BOOL);

    /* Create a variable for showing the fullscreen interface */
    var_Create(p_libheimdall, "intf-toggle-fscontrol", HEIMDALL_VAR_VOID);

    /* Create a variable for the Boss Key */
    var_Create(p_libheimdall, "intf-boss", HEIMDALL_VAR_VOID);

    /* Create a variable for showing the main interface */
    var_Create(p_libheimdall, "intf-show", HEIMDALL_VAR_VOID);

    return HEIMDALL_SUCCESS;

error:
    libheimdall_InternalCleanup( p_libheimdall );
    return i_ret;
}

/**
 * Cleanup a libheimdall instance. The instance is not completely deallocated
 * \param p_libheimdall the instance to clean
 */
void libheimdall_InternalCleanup( libheimdall_int_t *p_libheimdall )
{
    libheimdall_priv_t *priv = libheimdall_priv (p_libheimdall);

    /* Ask the interfaces to stop and destroy them */
    msg_Dbg( p_libheimdall, "removing all interfaces" );
    intf_DestroyAll( p_libheimdall );

#ifdef ENABLE_VLM
    /* Destroy VLM if created in libheimdall_InternalInit */
    if( priv->p_vlm )
    {
        vlm_Delete( priv->p_vlm );
    }
#endif

#if !defined( _WIN32 ) && !defined( __OS2__ )
    char *pidfile = var_InheritString( p_libheimdall, "pidfile" );
    if( pidfile != NULL )
    {
        msg_Dbg( p_libheimdall, "removing PID file %s", pidfile );
        if( unlink( pidfile ) )
            msg_Warn( p_libheimdall, "cannot remove PID file %s: %s",
                      pidfile, heimdall_strerror_c(errno) );
        free( pidfile );
    }
#endif

    if (priv->main_playlist)
        heimdall_playlist_Delete(priv->main_playlist);

    if ( priv->p_media_library )
        libheimdall_MlRelease( priv->p_media_library );

    if( priv->media_source_provider )
        heimdall_media_source_provider_Delete( priv->media_source_provider );

    libheimdall_InternalDialogClean( p_libheimdall );
    libheimdall_InternalKeystoreClean( p_libheimdall );
    libheimdall_InternalActionsClean( p_libheimdall );

    /* Save the configuration */
    if( !var_InheritBool( p_libheimdall, "ignore-config" ) )
        config_AutoSaveConfigFile( p_libheimdall );

    heimdall_LogDestroy(p_libheimdall->obj.logger);
    if (priv->tracer != NULL)
        heimdall_tracer_Destroy(priv->tracer);
    /* Free module bank. It is refcounted, so we call this each time  */
    module_EndBank (true);
#if defined(_WIN32) || defined(__OS2__)
    system_End( );
#endif
}

/**
 * Destroy libheimdall instance.
 * \param p_libheimdall the instance to destroy
 */
void libheimdall_InternalDestroy( libheimdall_int_t *p_libheimdall )
{
    heimdall_object_delete(p_libheimdall);
}

/*****************************************************************************
 * GetFilenames: parse command line options which are not flags
 *****************************************************************************
 * Parse command line for input files as well as their associated options.
 * An option always follows its associated input and begins with a ":".
 *****************************************************************************/
static void GetFilenames( libheimdall_int_t *p_heimdall, unsigned n,
                          const char *const args[] )
{
    while( n > 0 )
    {
        /* Count the input options */
        unsigned i_options = 0;

        while( args[--n][0] == ':' )
        {
            i_options++;
            if( n == 0 )
            {
                msg_Warn( p_heimdall, "options %s without item", args[n] );
                return; /* syntax!? */
            }
        }

        char *mrl = NULL;
        if( strstr( args[n], "://" ) == NULL )
        {
            mrl = heimdall_path2uri( args[n], NULL );
            if( !mrl )
                continue;
        }

        intf_InsertItem( p_heimdall, (mrl != NULL) ? mrl : args[n], i_options,
                         ( i_options ? &args[n + 1] : NULL ),
                         HEIMDALL_INPUT_OPTION_TRUSTED );
        free( mrl );
    }
}

static void
PlaylistConfigureFromVariables(heimdall_playlist_t *playlist, heimdall_object_t *obj)
{
    enum heimdall_playlist_playback_order order;
    if (var_InheritBool(obj, "random"))
        order = HEIMDALL_PLAYLIST_PLAYBACK_ORDER_RANDOM;
    else
        order = HEIMDALL_PLAYLIST_PLAYBACK_ORDER_NORMAL;

    /* repeat = repeat current; loop = repeat all */
    enum heimdall_playlist_playback_repeat repeat;
    if (var_InheritBool(obj, "repeat"))
        repeat = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_CURRENT;
    else if (var_InheritBool(obj, "loop"))
        repeat = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_ALL;
    else
        repeat = HEIMDALL_PLAYLIST_PLAYBACK_REPEAT_NONE;

    enum heimdall_playlist_media_stopped_action media_stopped_action;
    if (var_InheritBool(obj, "play-and-exit"))
        media_stopped_action = HEIMDALL_PLAYLIST_MEDIA_STOPPED_EXIT;
    else if (var_InheritBool(obj, "play-and-stop"))
        media_stopped_action = HEIMDALL_PLAYLIST_MEDIA_STOPPED_STOP;
    else
        media_stopped_action = HEIMDALL_PLAYLIST_MEDIA_STOPPED_CONTINUE;

    bool start_paused = var_InheritBool(obj, "start-paused");
    bool playlist_cork = var_InheritBool(obj, "playlist-cork");
    bool play_and_pause = var_InheritBool(obj, "play-and-pause");
    unsigned repeat_count = var_InheritInteger(obj, "input-repeat");

    heimdall_playlist_Lock(playlist);
    heimdall_playlist_SetPlaybackOrder(playlist, order);
    heimdall_playlist_SetPlaybackRepeat(playlist, repeat);
    heimdall_playlist_SetMediaStoppedAction(playlist, media_stopped_action);

    heimdall_player_t *player = heimdall_playlist_GetPlayer(playlist);

    /* the playlist and the player share the same lock, and this is not an
     * implementation detail */
    heimdall_player_SetStartPaused(player, start_paused);
    heimdall_player_SetPauseOnCork(player, playlist_cork);
    heimdall_player_SetPlayAndPause(player, play_and_pause);
    heimdall_player_SetRepeatCount(player, repeat_count);

    heimdall_playlist_Unlock(playlist);
}

heimdall_playlist_t *
libheimdall_GetMainPlaylist(libheimdall_int_t *libheimdall)
{
    libheimdall_priv_t *priv = libheimdall_priv(libheimdall);

    heimdall_mutex_lock(&priv->lock);
    heimdall_playlist_t *playlist = priv->main_playlist;
    if (priv->main_playlist == NULL)
    {
        bool auto_preparse = var_InheritBool(libheimdall, "auto-preparse");
        enum heimdall_playlist_preparsing rec = HEIMDALL_PLAYLIST_PREPARSING_DISABLED;
        int max_threads = 1;
        heimdall_tick_t default_timeout = 0;

        if (auto_preparse)
        {
            rec = HEIMDALL_PLAYLIST_PREPARSING_COLLAPSE;

            char *rec_str = var_InheritString(libheimdall, "recursive");
            if (rec_str != NULL)
            {
                if (!strcasecmp(rec_str, "none"))
                    rec = HEIMDALL_PLAYLIST_PREPARSING_ENABLED;
                else if (!strcasecmp(rec_str, "expand"))
                    rec = HEIMDALL_PLAYLIST_PREPARSING_RECURSIVE;
                free(rec_str);
            }
            max_threads = var_InheritInteger(libheimdall, "preparse-threads");
            if (max_threads < 1)
                max_threads = 1;

            default_timeout =
                HEIMDALL_TICK_FROM_MS(var_InheritInteger(libheimdall, "preparse-timeout"));
            if (default_timeout < 0)
                default_timeout = 0;
        }

        playlist = priv->main_playlist = heimdall_playlist_New(HEIMDALL_OBJECT(libheimdall),
                                                          rec, max_threads,
                                                          default_timeout);
        if (playlist)
            PlaylistConfigureFromVariables(playlist, HEIMDALL_OBJECT(libheimdall));
    }
    heimdall_mutex_unlock(&priv->lock);

    return playlist;
}
