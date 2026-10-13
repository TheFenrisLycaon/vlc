/*****************************************************************************
 * skin_main.cpp
 *****************************************************************************
 * Copyright (C) 2003 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Cyril Deguet     <asmax@via.ecp.fr>
 *          Olivier Teulière <ipkiss@via.ecp.fr>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define HEIMDALL_MODULE_LICENSE HEIMDALL_LICENSE_GPL_2_PLUS

#include <atomic>
#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_plugin.h>
#include <heimdall_playlist.h>
#include <heimdall_threads.h>
#include <heimdall_window.h>
#include <heimdall_cxx_helpers.hpp>

#include "dialogs.hpp"
#include "os_factory.hpp"
#include "os_loop.hpp"
#include "var_manager.hpp"
#include "heimdallproc.hpp"
#include "theme_loader.hpp"
#include "theme.hpp"
#include "theme_repository.hpp"
#include "vout_window.hpp"
#include "vout_manager.hpp"
#include "art_manager.hpp"
#include "../parser/interpreter.hpp"
#include "../commands/async_queue.hpp"
#include "../commands/cmd_quit.hpp"
#include "../commands/cmd_dialogs.hpp"
#include "../commands/cmd_minimize.hpp"
#include "../commands/cmd_playlist.hpp"
#include "../commands/cmd_callbacks.hpp"
#include "../commands/cmd_show_window.hpp"
#include "../commands/cmd_resize.hpp"
#include "../commands/cmd_on_top.hpp"

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------
static int  Open  ( heimdall_object_t * );
static void Close ( heimdall_object_t * );
static void *Run  ( void * );

static std::atomic<intf_thread_t *> skin_load_intf;
static heimdall::threads::mutex skin_load_lock;
static heimdall::threads::condition_variable skin_load_wait;
static uintptr_t skin_load_rc = 0;

//---------------------------------------------------------------------------
// Open: initialize interface
//---------------------------------------------------------------------------
static int Open( heimdall_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;

    // Allocate instance and initialize some members
    p_intf->p_sys = (intf_sys_t *) calloc( 1, sizeof( intf_sys_t ) );
    if( p_intf->p_sys == NULL )
        return HEIMDALL_ENOMEM;

    // Initialize "singleton" objects
    p_intf->p_sys->p_logger = NULL;
    p_intf->p_sys->p_queue = NULL;
    p_intf->p_sys->p_dialogs = NULL;
    p_intf->p_sys->p_interpreter = NULL;
    p_intf->p_sys->p_osFactory = NULL;
    p_intf->p_sys->p_varManager = NULL;
    p_intf->p_sys->p_voutManager = NULL;
    p_intf->p_sys->p_heimdallProc = NULL;
    p_intf->p_sys->p_repository = NULL;

    // No theme yet
    p_intf->p_sys->p_theme = NULL;

    heimdall_sem_init( &p_intf->p_sys->init_wait, 0 );
    p_intf->p_sys->b_error = false;

    if( heimdall_clone( &p_intf->p_sys->thread, Run, p_intf ) )
    {
        free( p_intf->p_sys );
        return HEIMDALL_EGENERIC;
    }

    heimdall_sem_wait( &p_intf->p_sys->init_wait );

    if( p_intf->p_sys->b_error )
    {
        heimdall_join( p_intf->p_sys->thread, NULL );

        free( p_intf->p_sys );
        return HEIMDALL_EGENERIC;
    }

    heimdall::threads::mutex_locker guard {skin_load_lock};
    skin_load_intf = p_intf;
    return HEIMDALL_SUCCESS;
}

//---------------------------------------------------------------------------
// Close: destroy interface
//---------------------------------------------------------------------------
static void Close( heimdall_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;

    msg_Dbg( p_intf, "closing skins2 module" );

    // ensure the playlist is stopped
    heimdall_playlist_t *playlist = heimdall_intf_GetMainPlaylist( p_intf );
    heimdall_playlist_Lock( playlist );
    heimdall_playlist_Stop ( playlist );
    heimdall_playlist_Unlock( playlist );

    {
        heimdall::threads::mutex_locker guard {skin_load_lock};
        while (skin_load_rc != 0)
            skin_load_wait.wait(skin_load_lock);

        /* We don't need the skin_load_lock anymore since the interface
         * will signaled that it doesn't exist anymore below. */
        skin_load_intf = NULL;
    }

    AsyncQueue *pQueue = p_intf->p_sys->p_queue;
    if( pQueue )
    {
        CmdGeneric *pCmd = new CmdExitLoop( p_intf );
        if( pCmd )
            pQueue->push( CmdGenericPtr( pCmd ) );
    }
    else
    {
        msg_Err( p_intf, "thread found already stopped (weird!)" );
    }

    heimdall_join( p_intf->p_sys->thread, NULL );

    // Destroy structure
    free( p_intf->p_sys );
}


//---------------------------------------------------------------------------
// Run: main loop
//---------------------------------------------------------------------------
static void *Run( void * p_obj )
{
    heimdall_thread_set_name("heimdall-skins2");

    intf_thread_t *p_intf = (intf_thread_t *)p_obj;

    bool b_error = false;
    char *skin_last = NULL;
    OSLoop *loop = NULL;
    std::unique_ptr<ThemeLoader> pLoader;

    // Initialize singletons
    if( OSFactory::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "cannot initialize OSFactory" );
        b_error = true;
        goto end;
    }
    if( AsyncQueue::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "cannot initialize AsyncQueue" );
        b_error = true;
        goto end;
    }
    if( Interpreter::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "cannot instantiate Interpreter" );
        b_error = true;
        goto end;
    }
    if( VarManager::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "cannot instantiate VarManager" );
        b_error = true;
        goto end;
    }
    if( HeimdallProc::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "cannot initialize HeimdallProc" );
        b_error = true;
        goto end;
    }
    if( VoutManager::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "cannot instantiate VoutManager" );
        b_error = true;
        goto end;
    }
    if( ArtManager::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "cannot instantiate ArtManager" );
        b_error = true;
        goto end;
    }
    if( ThemeRepository::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "cannot instantiate ThemeRepository" );
        b_error = true;
        goto end;
    }
    if( Dialogs::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "cannot instantiate dialogs provider" );
        b_error = true;
        goto end;
    }

    // Load a theme
    skin_last = config_GetPsz( "skins2-last" );
    pLoader = std::make_unique<ThemeLoader>(p_intf);

    if( !skin_last || !pLoader->load( skin_last ) )
    {
        // No skins (not even the default one). let's quit
        CmdQuit *pCmd = new CmdQuit( p_intf );
        AsyncQueue *pQueue = AsyncQueue::instance( p_intf );
        pQueue->push( CmdGenericPtr( pCmd ) );
        msg_Err( p_intf, "no skins found : exiting");
    }

    pLoader.reset();
    free( skin_last );

    // Get the instance of OSLoop
    loop = OSFactory::instance( p_intf )->getOSLoop();

    // Signal the main thread this thread is now ready
    p_intf->p_sys->b_error = false;
    heimdall_sem_post( &p_intf->p_sys->init_wait );

    // Enter the main event loop
    loop->run();

    // Destroy OSLoop
    OSFactory::instance( p_intf )->destroyOSLoop();

    // save and delete the theme
    if( p_intf->p_sys->p_theme )
    {
        p_intf->p_sys->p_theme->saveConfig();
        p_intf->p_sys->p_theme.reset();
        msg_Dbg( p_intf, "current theme deleted" );
    }

    // save config file
    config_SaveConfigFile( p_intf );

end:
    // Destroy "singleton" objects
    Dialogs::destroy( p_intf );
    ThemeRepository::destroy( p_intf );
    ArtManager::destroy( p_intf );
    VoutManager::destroy( p_intf );
    HeimdallProc::destroy( p_intf );
    VarManager::destroy( p_intf );
    Interpreter::destroy( p_intf );
    AsyncQueue::destroy( p_intf );
    OSFactory::destroy( p_intf );

    if( b_error )
    {
        p_intf->p_sys->b_error = true;
        heimdall_sem_post( &p_intf->p_sys->init_wait );
    }

    return NULL;
}

static int  WindowOpen( heimdall_window_t * );
static void WindowClose( heimdall_window_t * );

typedef struct
{
    intf_thread_t*     pIntf;
    heimdall_window_cfg_t  cfg;
} vout_window_skins_t;

static void WindowOpenLocal( intf_thread_t* pIntf, heimdall_object_t *pObj )
{
    heimdall_window_t* pWnd = (heimdall_window_t*)pObj;
    vout_window_skins_t* sys = (vout_window_skins_t *)pWnd->sys;
    int width = sys->cfg.width;
    int height = sys->cfg.height;

    VoutManager::instance( pIntf )->acceptWnd( pWnd, width, height );
}

static void WindowCloseLocal( intf_thread_t* pIntf, heimdall_object_t *pObj )
{
    heimdall_window_t* pWnd = (heimdall_window_t*)pObj;
    VoutManager::instance( pIntf )->releaseWnd( pWnd );
}

static void WindowSetFullscreen( heimdall_window_t *pWnd, const char * );

static int WindowEnable( heimdall_window_t *pWnd, const heimdall_window_cfg_t *cfg )
{
    vout_window_skins_t* sys = static_cast<vout_window_skins_t*>(pWnd->sys);

    /* Protect from races against the main interface closing. */
    heimdall::threads::mutex_locker guard {skin_load_lock};

    /* If the interface has already been closed, we cannot enable the window
     * anymore and we need to report that. */
    if (skin_load_intf == NULL)
        return HEIMDALL_EGENERIC;
    intf_thread_t *pIntf = skin_load_intf;

    sys->cfg = *cfg;

    // force execution in the skins2 thread context
    CmdExecuteBlock* cmd = new CmdExecuteBlock( pIntf, HEIMDALL_OBJECT( pWnd ),
                                                WindowOpenLocal );
    CmdExecuteBlock::executeWait( CmdGenericPtr( cmd ) );

    if( pWnd->type == HEIMDALL_WINDOW_TYPE_DUMMY )
    {
        msg_Dbg( pIntf, "Vout window creation failed" );
        return HEIMDALL_EGENERIC;
    }

    if (cfg->is_fullscreen)
        WindowSetFullscreen( pWnd, NULL );

    /* Prevent the interface from closing before we get disabled. */
    skin_load_rc++;
    return HEIMDALL_SUCCESS;
}

static void WindowDisable( heimdall_window_t *pWnd )
{
    /* Protect from races against the main interface closing. */
    heimdall::threads::mutex_locker guard {skin_load_lock};
    intf_thread_t *pIntf = skin_load_intf;

    // force execution in the skins2 thread context
    CmdExecuteBlock* cmd = new CmdExecuteBlock( pIntf, HEIMDALL_OBJECT( pWnd ),
                                                WindowCloseLocal );
    CmdExecuteBlock::executeWait( CmdGenericPtr( cmd ) );

    pWnd->type = HEIMDALL_WINDOW_TYPE_DUMMY;

    /* Now the interface can be closed. */
    skin_load_rc--;
    skin_load_wait.signal();
}

static void WindowResize( heimdall_window_t *pWnd,
                          unsigned i_width, unsigned i_height )
{
    vout_window_skins_t* sys = (vout_window_skins_t *)pWnd->sys;
    intf_thread_t *pIntf = sys->pIntf;

    if( i_width == 0 || i_height == 0 )
        return;

    // Post a vout resize command
    AsyncQueue *pQueue = AsyncQueue::instance( pIntf );
    CmdResizeVout *pCmd = new CmdResizeVout( pIntf, pWnd,
                                             (int)i_width, (int)i_height );
    pQueue->push( CmdGenericPtr( pCmd ) );
}

static void WindowSetState( heimdall_window_t *pWnd, unsigned i_arg )
{
    vout_window_skins_t* sys = (vout_window_skins_t *)pWnd->sys;
    intf_thread_t *pIntf = sys->pIntf;
    AsyncQueue *pQueue = AsyncQueue::instance( pIntf );
    bool on_top = i_arg & HEIMDALL_WINDOW_STATE_ABOVE;

    // Post a SetOnTop command
    CmdSetOnTop* pCmd = new CmdSetOnTop( pIntf, on_top );
    pQueue->push( CmdGenericPtr( pCmd ) );
}

static void WindowUnsetFullscreen( heimdall_window_t *pWnd )
{
    vout_window_skins_t* sys = (vout_window_skins_t *)pWnd->sys;
    intf_thread_t *pIntf = sys->pIntf;
    AsyncQueue *pQueue = AsyncQueue::instance( pIntf );
    CmdSetFullscreen* pCmd = new CmdSetFullscreen( pIntf, pWnd, false );

    pQueue->push( CmdGenericPtr( pCmd ) );
}

static void WindowSetFullscreen( heimdall_window_t *pWnd, const char * )
{
    vout_window_skins_t* sys = (vout_window_skins_t *)pWnd->sys;
    intf_thread_t *pIntf = sys->pIntf;
    AsyncQueue *pQueue = AsyncQueue::instance( pIntf );
    // Post a set fullscreen command
    CmdSetFullscreen* pCmd = new CmdSetFullscreen( pIntf, pWnd, true );

    pQueue->push( CmdGenericPtr( pCmd ) );
}

static const auto window_ops = []{
    struct heimdall_window_operations ops {};
    ops.enable = WindowEnable;
    ops.disable = WindowDisable;
    ops.resize = WindowResize;
    ops.destroy = WindowClose;
    ops.set_state = WindowSetState;
    ops.unset_fullscreen = WindowUnsetFullscreen;
    ops.set_fullscreen = WindowSetFullscreen;
    return ops;
}();

static int WindowOpen( heimdall_window_t *pWnd )
{
    if( var_InheritBool( pWnd, "video-wallpaper" )
     || !var_InheritBool( pWnd, "embedded-video" ) )
        return HEIMDALL_EGENERIC;

    vout_window_skins_t* sys;

    /* Prevent the interface from closing behind our back. */
    heimdall::threads::mutex_locker guard {skin_load_lock};
    if (skin_load_intf == NULL)
        return HEIMDALL_EGENERIC;

    intf_thread_t *pIntf = skin_load_intf;

    if( !var_InheritBool( pIntf, "skinned-video") )
        return HEIMDALL_EGENERIC;

    sys = new (std::nothrow) vout_window_skins_t;
    if( !sys )
        return HEIMDALL_ENOMEM;

    pWnd->sys = sys;
    sys->pIntf = pIntf;
    pWnd->ops = &window_ops;
    pWnd->type = HEIMDALL_WINDOW_TYPE_DUMMY;
    return HEIMDALL_SUCCESS;
}

static void WindowClose( heimdall_window_t *pWnd )
{
    vout_window_skins_t* sys = (vout_window_skins_t *)pWnd->sys;

    delete sys;
}

//---------------------------------------------------------------------------
// Module descriptor
//---------------------------------------------------------------------------
#define SKINS2_LAST      N_("Skin to use")
#define SKINS2_LAST_LONG N_("Path to the skin to use.")
#define SKINS2_CONFIG      N_("Config of last used skin")
#define SKINS2_CONFIG_LONG N_("Windows configuration of the last skin used. " \
        "This option is updated automatically, do not touch it." )
#define SKINS2_SYSTRAY      N_("Systray icon")
#define SKINS2_SYSTRAY_LONG N_("Show a systray icon for Heimdall")
#define SKINS2_TASKBAR      N_("Show Heimdall on the taskbar")
#define SKINS2_TRANSPARENCY      N_("Enable transparency effects")
#define SKINS2_TRANSPARENCY_LONG N_("You can disable all transparency effects"\
    " if you want. This is mainly useful when moving windows does not behave" \
    " correctly.")
#define SKINS2_PLAYLIST N_("Use a skinned playlist")
#define SKINS2_VIDEO N_("Display video in a skinned window if any")
#define SKINS2_VIDEO_LONG N_( \
    "When set to 'no', this parameter is intended to give old skins a chance" \
    " to play back video even though no video tag is implemented")

heimdall_module_begin ()
    set_subcategory( SUBCAT_INTERFACE_MAIN )
    add_loadfile("skins2-last", "", SKINS2_LAST, SKINS2_LAST_LONG)
    add_string( "skins2-config", "", SKINS2_CONFIG, SKINS2_CONFIG_LONG )
        change_private ()
#ifdef _WIN32
    add_bool( "skins2-systray", true, SKINS2_SYSTRAY,
              SKINS2_SYSTRAY_LONG )
    add_bool( "skins2-taskbar", true, SKINS2_TASKBAR,
              nullptr )
#endif
    add_bool( "skins2-transparency", false, SKINS2_TRANSPARENCY,
              SKINS2_TRANSPARENCY_LONG )

    add_bool( "skinned-playlist", true, SKINS2_PLAYLIST,
              nullptr )
    add_bool( "skinned-video", true, SKINS2_VIDEO,
              SKINS2_VIDEO_LONG )
    set_shortname( N_("Skins"))
    set_description( N_("Skinnable Interface") )
    set_capability( "interface", 30 )
    set_callbacks( Open, Close )
    add_shortcut( "skins" )

    add_submodule ()
        set_capability( "vout window", 51 )
        set_callback( WindowOpen )

heimdall_module_end ()
