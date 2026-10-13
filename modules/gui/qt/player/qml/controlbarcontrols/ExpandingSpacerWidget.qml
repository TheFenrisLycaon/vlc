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
import QtQuick.Templates as T
import QtQuick.Layouts


import Heimdall.Widgets as Widgets
import Heimdall.Style


Item {
    enabled: false

    implicitWidth: HeimdallStyle.widthExtendedSpacer
    implicitHeight: HeimdallStyle.icon_toolbar

    property bool paintOnly: false
    property alias spacetextExt: spacetext

    Layout.minimumWidth: 1

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.ButtonStandard
    }

    T.Label {
        id: spacetext
        anchors.centerIn: parent

        text: HeimdallIcons.space
        color: theme.fg.secondary
        visible: paintOnly

        font.pixelSize: HeimdallStyle.icon_toolbar
        font.family: HeimdallIcons.fontFamily

        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        Accessible.ignored: true
    }
}
