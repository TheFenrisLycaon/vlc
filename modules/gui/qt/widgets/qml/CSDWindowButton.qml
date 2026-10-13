/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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
import Heimdall.MainInterface
import Heimdall.Style


T.Button {
    id: control

    property color color
    property color hoverColor
    property string iconTxt: ""
    property bool showHovered: false
    property bool isThemeDark: false
    property bool externalPressed: false

    readonly property bool _paintHovered: control.hovered || showHovered

    padding: 0
    width: HeimdallStyle.dp(40, HeimdallStyle.scale)

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    focusPolicy: Qt.NoFocus

    background: Rectangle {
        height: control.height
        width: control.width
        color: {
            if (control.pressed || (control.externalPressed && control._paintHovered))
                return control.isThemeDark ? Qt.lighter(control.hoverColor, 1.2)
                                           : Qt.darker(control.hoverColor, 1.2)

            if (control._paintHovered)
                return control.hoverColor

            return "transparent"
        }
    }

    contentItem: Item {
        Widgets.IconLabel {
            id: icon
            anchors.centerIn: parent
            text: control.iconTxt

            font.family:{
                if (MainCtx.osName === MainCtx.Windows)
                {
                    if(MainCtx.osVersion === 10)
                        return "Segoe MDL2 Assets"

                    else if(MainCtx.osVersion >= 11)
                        return "Segoe Fluent Icons"
                }
     
                return HeimdallIcons.fontFamily
            }

            font.pixelSize: HeimdallStyle.icon_CSD
            color: control.color
        }
    }
}
