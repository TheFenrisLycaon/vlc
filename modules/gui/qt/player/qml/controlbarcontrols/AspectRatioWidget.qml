/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
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
import QtQuick


import Heimdall.Widgets as Widgets
import Heimdall.Style
import Heimdall.Player

Widgets.ComboBoxExt {
    id: combo
    property bool paintOnly: false

    signal requestLockUnlockAutoHide(bool lock)

    function forceUnlock () {
        combo.popup.close()
    }

    width: HeimdallStyle.combobox_width_normal
    height: HeimdallStyle.combobox_height_normal
    textRole: "display"
    model: Player.aspectRatio
    currentIndex: -1
    onCurrentIndexChanged: model.toggleIndex(currentIndex)
    Accessible.name: qsTr("Aspect ratio")

    Connections {
        target: combo.popup
        function onOpened() {
            combo.requestLockUnlockAutoHide(true)
        }
        function onClosed() {
            combo.requestLockUnlockAutoHide(false)
        }
    }
}
