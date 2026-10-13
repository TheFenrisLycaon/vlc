/*****************************************************************************
 * os2_loop.cpp
 *****************************************************************************
 * Copyright (C) 2003, 2013 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Cyril Deguet      <asmax@via.ecp.fr>
 *          Olivier Teulière <ipkiss@via.ecp.fr>
 *          KO Myung-Hun      <komh@chollian.net>
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

#ifdef OS2_SKINS

#include "os2_factory.hpp"
#include "os2_loop.hpp"
#include "../src/generic_window.hpp"
#include "../events/evt_key.hpp"
#include "../events/evt_leave.hpp"
#include "../events/evt_menu.hpp"
#include "../events/evt_motion.hpp"
#include "../events/evt_mouse.hpp"
#include "../events/evt_refresh.hpp"
#include "../events/evt_scroll.hpp"
#include <heimdall_actions.h>


#define GET_X_MP( mp ) ( SHORT1FROMMP( mp ))
#define GET_Y_MP( mp ) (( rcl.yTop - 1 ) - SHORT2FROMMP( mp )) /* Invert Y */

#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE   0x41F
#endif

OS2Loop::OS2Loop( intf_thread_t *pIntf ): OSLoop( pIntf )
{
    // Initialize the map
    virtKeyToHeimdallKey[VK_F1] = KEY_F1;
    virtKeyToHeimdallKey[VK_F2] = KEY_F2;
    virtKeyToHeimdallKey[VK_F3] = KEY_F3;
    virtKeyToHeimdallKey[VK_F4] = KEY_F4;
    virtKeyToHeimdallKey[VK_F5] = KEY_F5;
    virtKeyToHeimdallKey[VK_F6] = KEY_F6;
    virtKeyToHeimdallKey[VK_F7] = KEY_F7;
    virtKeyToHeimdallKey[VK_F8] = KEY_F8;
    virtKeyToHeimdallKey[VK_F9] = KEY_F9;
    virtKeyToHeimdallKey[VK_F10] = KEY_F10;
    virtKeyToHeimdallKey[VK_F11] = KEY_F11;
    virtKeyToHeimdallKey[VK_F12] = KEY_F12;
    virtKeyToHeimdallKey[VK_ENTER] = KEY_ENTER;
    virtKeyToHeimdallKey[VK_NEWLINE] = KEY_ENTER;
    virtKeyToHeimdallKey[VK_SPACE] = ' ';
    virtKeyToHeimdallKey[VK_ESC] = KEY_ESC;
    virtKeyToHeimdallKey[VK_LEFT] = KEY_LEFT;
    virtKeyToHeimdallKey[VK_RIGHT] = KEY_RIGHT;
    virtKeyToHeimdallKey[VK_UP] = KEY_UP;
    virtKeyToHeimdallKey[VK_DOWN] = KEY_DOWN;
    virtKeyToHeimdallKey[VK_INSERT] = KEY_INSERT;
    virtKeyToHeimdallKey[VK_DELETE] = KEY_DELETE;
    virtKeyToHeimdallKey[VK_HOME] = KEY_HOME;
    virtKeyToHeimdallKey[VK_END] = KEY_END;
    virtKeyToHeimdallKey[VK_PAGEUP] = KEY_PAGEUP;
    virtKeyToHeimdallKey[VK_PAGEDOWN] = KEY_PAGEDOWN;
}


OSLoop *OS2Loop::instance( intf_thread_t *pIntf )
{
    if( pIntf->p_sys->p_osLoop == NULL )
        pIntf->p_sys->p_osLoop = std::make_unique<OS2Loop>(pIntf);
    return pIntf->p_sys->p_osLoop.get();
}


void OS2Loop::destroy( intf_thread_t *pIntf )
{
    assert(pIntf->p_sys->p_osLoop);
    pIntf->p_sys->p_osLoop.reset();
}


void OS2Loop::run()
{
    QMSG qm;

    // Compute windows message list
    while( WinGetMsg( 0, &qm, NULLHANDLE, 0, 0 ))
        WinDispatchMsg( 0, &qm );
}


MRESULT EXPENTRY OS2Loop::processEvent( HWND hwnd, ULONG msg,
                                        MPARAM mp1, MPARAM mp2 )
{
    OS2Factory *pFactory =
        (OS2Factory*)OS2Factory::instance( getIntf() );
    GenericWindow *pWin = pFactory->m_windowMap[hwnd];

    // To invert Y
    RECTL rcl;
    WinQueryWindowRect( hwnd, &rcl );

    GenericWindow &win = *pWin;
    switch( msg )
    {
        case WM_PAINT:
        {
            HPS   hps;
            RECTL rclPaint;

            hps = WinBeginPaint( hwnd, NULLHANDLE, &rclPaint );
            EvtRefresh evt( getIntf(),
                            rclPaint.xLeft,
                            // Find top and invert it
                            ( rcl.yTop - 1 ) - ( rclPaint.yTop - 1 ),
                            rclPaint.xRight - rclPaint.xLeft + 1,
                            rclPaint.yTop - rclPaint.yBottom + 1 );
            win.processEvent( evt );
            WinEndPaint( hps );
            return 0;
        }
        case WM_COMMAND:
        {
            EvtMenu evt( getIntf(), SHORT1FROMMP( mp1 ));
            win.processEvent( evt );
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            pFactory->changeCursor( pFactory->getCursorType());

            // Compute the absolute position of the mouse
            POINTL ptl;
            WinQueryPointerPos( HWND_DESKTOP, &ptl );
            int x = ptl.x;
            int y = ( pFactory->getScreenHeight() - 1 ) - ptl.y;
            EvtMotion evt( getIntf(), x, y );
            win.processEvent( evt );

            return MRFROMLONG( TRUE );
        }
        case WM_MOUSELEAVE:
        {
            EvtLeave evt( getIntf() );
            win.processEvent( evt );
            return MRFROMLONG( TRUE );
        }
        case WM_BUTTON1DOWN:
        {
            WinSetCapture( HWND_DESKTOP, hwnd );
            EvtMouse evt( getIntf(), GET_X_MP( mp1 ),
                          GET_Y_MP( mp1 ), EvtMouse::kLeft,
                          EvtMouse::kDown, getMod( mp2 ) );
            win.processEvent( evt );
            return MRFROMLONG( TRUE );
        }
        case WM_BUTTON2DOWN:
        {
            WinSetCapture( HWND_DESKTOP, hwnd );
            EvtMouse evt( getIntf(), GET_X_MP( mp1 ),
                          GET_Y_MP( mp1 ), EvtMouse::kRight,
                          EvtMouse::kDown, getMod( mp2 ) );
            win.processEvent( evt );
            return MRFROMLONG( TRUE );
        }
        case WM_BUTTON1UP:
        {
            WinSetCapture( HWND_DESKTOP, NULLHANDLE );
            EvtMouse evt( getIntf(), GET_X_MP( mp1 ),
                          GET_Y_MP( mp1 ), EvtMouse::kLeft,
                          EvtMouse::kUp, getMod( mp2 ) );
            win.processEvent( evt );
            return MRFROMLONG( TRUE );
        }
        case WM_BUTTON2UP:
        {
            WinSetCapture( HWND_DESKTOP, NULLHANDLE );
            EvtMouse evt( getIntf(), GET_X_MP( mp1 ),
                          GET_Y_MP( mp1 ), EvtMouse::kRight,
                          EvtMouse::kUp, getMod( mp2 ) );
            win.processEvent( evt );
            return MRFROMLONG( TRUE );
        }
        case WM_BUTTON1DBLCLK:
        {
            WinSetCapture( HWND_DESKTOP, NULLHANDLE );
            EvtMouse evt( getIntf(), GET_X_MP( mp1 ),
                          GET_Y_MP( mp1 ), EvtMouse::kLeft,
                          EvtMouse::kDblClick, getMod( mp2 ) );
            win.processEvent( evt );
            return MRFROMLONG( TRUE );
        }
        case WM_BUTTON2DBLCLK:
        {
            WinSetCapture( HWND_DESKTOP, NULLHANDLE );
            EvtMouse evt( getIntf(), GET_X_MP( mp1 ),
                          GET_Y_MP( mp1 ), EvtMouse::kRight,
                          EvtMouse::kDblClick, getMod( mp2 ) );
            win.processEvent( evt );
            return MRFROMLONG( TRUE );
        }
        case WM_CHAR:
        {
            // The key events are first processed here and not translated
            // into WM_CHAR events because we need to know the status of
            // the modifier keys.

            USHORT fsFlags = SHORT1FROMMP( mp1 );
            USHORT usCh    = SHORT1FROMMP( mp2 );
            USHORT usVk    = SHORT2FROMMP( mp2 );

            // Get Heimdall key code from the virtual key code
            int key = ( fsFlags & KC_VIRTUALKEY ) ?
                            virtKeyToHeimdallKey[ usVk ] : 0;
            if( !key )
            {
                // This appears to be a "normal" (ascii) key
                key = tolower( usCh );
            }

            if( key )
            {
                // Get the modifier
                int mod = 0;
                if( fsFlags & KC_CTRL )
                {
                    mod |= EvtInput::kModCtrl;
                }
                if( fsFlags & KC_SHIFT )
                {
                    mod |= EvtInput::kModShift;
                }
                if( fsFlags & KC_ALT )
                {
                    mod |= EvtInput::kModAlt;
                }

                // Get the state
                EvtKey::ActionType_t state;
                if( fsFlags & KC_KEYUP )
                {
                    state = EvtKey::kUp;
                }
                else
                {
                    state = EvtKey::kDown;
                }

                EvtKey evt( getIntf(), key, state, mod );
                win.processEvent( evt );
            }
            return MRFROMLONG( TRUE );
        }
        default:
            break;
    }
    return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}


int OS2Loop::getMod( MPARAM mp ) const
{
    int mod = EvtInput::kModNone;
    if( SHORT2FROMMP( mp ) & KC_CTRL )
        mod |= EvtInput::kModCtrl;
    if( SHORT2FROMMP( mp ) & KC_SHIFT )
        mod |= EvtInput::kModShift;

    return mod;
}


void OS2Loop::exit()
{
    WinPostQueueMsg( HMQ_CURRENT, WM_QUIT, 0, 0 );
}

#endif
