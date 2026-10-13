/*****************************************************************************
 * skin_common.hpp
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef SKIN_COMMON_HPP
#define SKIN_COMMON_HPP

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_configuration.h>
#include <heimdall_interface.h>
#include <heimdall_playlist.h>
#include <heimdall_player.h>
#include <heimdall_fs.h>

#include <string>
#include <memory>

class AsyncQueue;
class Logger;
class Dialogs;
class Interpreter;
class OSFactory;
class OSLoop;
class VarManager;
class HeimdallProc;
class VoutManager;
class ArtManager;
class Theme;
class ThemeRepository;

#ifndef M_PI
#   define M_PI 3.14159265358979323846
#endif

#ifdef _MSC_VER
// turn off 'warning C4355: 'this' : used in base member initializer list'
#pragma warning ( disable:4355 )
// turn off 'identifier was truncated to '255' characters in the debug info'
#pragma warning ( disable:4786 )
#endif

//---------------------------------------------------------------------------
// intf_sys_t: description and status of skin interface
//---------------------------------------------------------------------------
struct intf_sys_t
{
    // "Singleton" objects: MUST be initialized to NULL !
    /// Logger
    Logger *p_logger;
    /// Asynchronous command queue
    AsyncQueue *p_queue;
    /// Dialog provider
    Dialogs *p_dialogs;
    /// Script interpreter
    Interpreter *p_interpreter;
    /// Factory for OS specific classes
    OSFactory *p_osFactory;
    /// Main OS specific message loop
    std::unique_ptr<OSLoop> p_osLoop;
    /// Variable manager
    VarManager *p_varManager;
    /// Heimdall state handler
    HeimdallProc *p_heimdallProc;
    /// Vout manager
    VoutManager *p_voutManager;
    /// Art manager
    ArtManager *p_artManager;
    /// Theme repository
    ThemeRepository *p_repository;

    /// Current theme
    std::unique_ptr<Theme> p_theme;

    /// synchronisation at start of interface
    heimdall_thread_t thread;
    heimdall_sem_t    init_wait;
    bool         b_error;
};


/// Base class for all skin classes
class SkinObject
{
public:
    SkinObject( intf_thread_t *pIntf ): m_pIntf( pIntf ) { }
    virtual ~SkinObject() { }

    /// Getter (public because it is used in C callbacks in the win32
    /// interface)
    intf_thread_t *getIntf() const { return m_pIntf; }
    heimdall_playlist_t *getPL() const
        { return heimdall_intf_GetMainPlaylist( m_pIntf ); }

private:
    intf_thread_t *m_pIntf;
};


#endif
