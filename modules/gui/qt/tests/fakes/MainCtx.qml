/*****************************************************************************
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
pragma Singleton
import QtQuick

// Test stand-in for the C++ MainCtx singleton (maininterface/mainctx.hpp).
// Only the members the tested QML files read. Tests set them in init().
QtObject {
    readonly property int MAININTERFACE_MODE_MAINDISPLAY: 1
    readonly property int MAININTERFACE_MODE_PLAYER: 2

    property int mouseHideTimeout: 100
    property bool hasEmbededVideo: false
    property int initialEffectiveMainInterfaceMode: MAININTERFACE_MODE_MAINDISPLAY

    property QtObject playqueuePanel: QtObject {
        property bool docked: true
        property bool visible: false
    }
}
