/*****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Benjamin Arnaud <bunjee@omega.gg>
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
import QtQuick.Layouts


import Heimdall.MainInterface
import Heimdall.Style
import Heimdall.Widgets as Widgets
import Heimdall.Util

RowLayout {
    id: root

    // Properties

    default property alias content: content.data

    property int preferredWidth: HeimdallStyle.dp(512, HeimdallStyle.scale)

    //FIXME make TrackMenuController a proper type (see TrackMenu)
    required property QtObject trackMenuController

    // Settings

    spacing: 0

    focus: true

    Navigation.leftItem: button

    // Children

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.Window
    }

    Item {
        Layout.preferredWidth: HeimdallStyle.dp(72, HeimdallStyle.scale)
        Layout.fillHeight: true

        Layout.topMargin: HeimdallStyle.margin_large

        Layout.alignment: Qt.AlignLeft | Qt.AlignTop

        Widgets.IconTrackButton {
            id: button

            AccessibleCompat.id: "tracksBackButton"

            anchors.horizontalCenter: parent.horizontalCenter

            description: qsTr("Back")
            text: HeimdallIcons.back

            Navigation.parentItem: root
            Navigation.rightItem: content

            onClicked: trackMenuController.requestBack()
        }
    }

    Rectangle {
        Layout.preferredWidth: HeimdallStyle.margin_xxxsmall
        Layout.fillHeight: true

        color: theme.border
    }

    FocusScope {
        id: content

        Layout.fillWidth: true
        Layout.fillHeight: true

        Layout.margins: HeimdallStyle.margin_large
    }
}
