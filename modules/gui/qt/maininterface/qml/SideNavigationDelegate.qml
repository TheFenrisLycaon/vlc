/*****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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


import Heimdall.MainInterface
import Heimdall.Widgets as Widgets
import Heimdall.Style
import Heimdall.Util

T.ItemDelegate {
    id: control

    // Properties

    property string iconTxt: ""

    property bool showText: true

    property real radius: HeimdallStyle.button_radius

    // Settings

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: HeimdallStyle.margin_xxsmall

    leftInset: implicitLeftInset
    rightInset: implicitRightInset

    topInset: implicitTopInset
    bottomInset: implicitBottomInset

    property real implicitLeftInset: HeimdallStyle.margin_xxsmall
    property real implicitRightInset: HeimdallStyle.margin_xxsmall

    property real implicitTopInset: HeimdallStyle.margin_xxxsmall
    property real implicitBottomInset: HeimdallStyle.margin_xxxsmall

    // Accessible

    Accessible.onPressAction: control.clicked()
    AccessibleCompat.id: {
        if (model.depth !== 0 || !model.uri || model.uri.length === 0) return "";
        const navBarIds = {
            "home": "homeNavBar",
            "video": "videoNavBar",
            "music": "musicNavBar",
            "network": "browseNavBar",
            "discover": "discoverNavBar"
        }
        const id = navBarIds[model.uri[0]]
        if (id !== undefined)
            return id
        else
            return ""
    }

    // Tooltip

    T.ToolTip.visible: (showText === false && T.ToolTip.text && (hovered || visualFocus))

    T.ToolTip.delay: HeimdallStyle.delayToolTipAppear

    T.ToolTip.text: text

    // Childs

    ColorContext {
        id: theme
        colorSet: ColorContext.TabButton

        focused: control.visualFocus
        hovered: control.hovered
        pressed: control.down
        enabled: control.enabled
    }

    background: Widgets.AnimatedBackground {
        enabled: theme.initialized
        color: theme.bg.primary
        border.color: visualFocus ? theme.visualFocus : "transparent"

        radius: control.radius

        Widgets.CurrentIndicator {
            anchors {
                left: parent.left
                leftMargin: HeimdallStyle.margin_xxxsmall
                verticalCenter: parent.verticalCenter
            }
            implicitHeight: parent.height / 2
            visible: control.checked

            radius: control.radius / 2
        }
    }

    contentItem: RowLayout {
        spacing: HeimdallStyle.margin_xsmall

        Item {
            Layout.preferredWidth: HeimdallStyle.icon_banner
            Layout.fillHeight: true

            Widgets.IconLabel {
                id: iconLabel

                visible: text.length > 0

                anchors.centerIn: parent

                text: control.iconTxt

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                color: (control.highlighted || control.checked) ? theme.accent : theme.fg.primary

                font.pixelSize: HeimdallStyle.icon_banner
            }
        }

        T.Label {
            id: label

            Layout.fillWidth: true
            Layout.fillHeight: true

            text: control.text

            verticalAlignment: Text.AlignVCenter

            color: control.checked ? theme.fg.secondary : theme.fg.primary

            elide: Text.ElideRight

            visible: control.showText

            font.pixelSize: HeimdallStyle.fontSize_normal

            font.weight: control.checked ? Font.DemiBold : Font.Normal

            //button text is already exposed
            Accessible.ignored: true

            Behavior on color {
                enabled: theme.initialized

                ColorAnimation {
                    duration: HeimdallStyle.duration_short
                }
            }
        }
    }
}
