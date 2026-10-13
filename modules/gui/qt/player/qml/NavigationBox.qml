/*****************************************************************************
 * Copyright (C) 2023 VLC authors and VideoLAN
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
import Heimdall.Style
import Heimdall.Widgets as Widgets
import Heimdall.Player
import Heimdall.Util

T.Control {
    id: navigationBox

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    Accessible.name: qsTr("DVD navigation")
    AccessibleCompat.id: "navigationBox"

    padding: HeimdallStyle.focus_border

    property real dragXMin: 0
    property real dragXMax: 0
    property real dragYMin: 0
    property real dragYMax: 0

    property bool show: false

    Drag.active: mouseArea.drag.active

    function toggleVisibility() {
        show = !show
    }

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.ButtonStandard
    }

    Connections {
        target: MainCtx
        function onNavBoxToggled() {
            navigationBox.toggleVisibility()
        }
    }

    contentItem: GridLayout {
        columns: 3
        rows: 2
        columnSpacing: HeimdallStyle.margin_xxsmall
        rowSpacing: HeimdallStyle.margin_xxsmall

        Widgets.ActionButtonOverlay {
            id: closeButton
            color: theme.fg.secondary
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.column: 2
            Layout.row: 0
            iconTxt: HeimdallIcons.window_close
            iconSize: HeimdallStyle.icon_normal

            Accessible.name: qsTr("Close navigation")
            AccessibleCompat.id: "navCloseButton"

            onClicked: {
                navigationBox.toggleVisibility()
            }
        }

        Widgets.ActionButtonOverlay {
            id: upButton
            color: theme.fg.secondary
            Layout.column: 1
            Layout.row: 0
            iconTxt: HeimdallIcons.ic_fluent_chevron_up_24
            iconSize: HeimdallStyle.icon_large

            Accessible.name: qsTr("Navigate up")
            AccessibleCompat.id: "navUpButton"

            onClicked: Player.navigateUp()
        }

        Widgets.ActionButtonOverlay {
            id: leftButton
            color: theme.fg.secondary
            Layout.column: 0
            Layout.row: 1
            iconTxt: HeimdallIcons.ic_fluent_chevron_left_24
            iconSize: HeimdallStyle.icon_large

            Accessible.name: qsTr("Navigate left")
            AccessibleCompat.id: "navLeftButton"

            onClicked: Player.navigateLeft()
        }

        Widgets.ActionButtonOverlay {
            id: selectButton
            color: theme.fg.secondary
            Layout.column: 1
            Layout.row: 1
            iconTxt: HeimdallIcons.ok
            font.pixelSize: HeimdallStyle.fontSize_large
            iconSize: HeimdallStyle.icon_normal

            Accessible.name: qsTr("Select")
            AccessibleCompat.id: "navOkButton"

            onClicked: Player.navigateActivate()
        }

        Widgets.ActionButtonOverlay {
            id: rightButton
            color: theme.fg.secondary
            Layout.column: 2
            Layout.row: 1
            iconTxt: HeimdallIcons.ic_fluent_chevron_right_24
            iconSize: HeimdallStyle.icon_large

            Accessible.name: qsTr("Navigate right")
            AccessibleCompat.id: "navRightButton"

            onClicked: Player.navigateRight()
        }

        Widgets.ActionButtonOverlay {
            id: downButton
            color: theme.fg.secondary
            Layout.column: 1
            Layout.row: 2
            iconTxt: HeimdallIcons.ic_fluent_chevron_down_24
            iconSize: HeimdallStyle.icon_large

            Accessible.name: qsTr("Navigate down")
            AccessibleCompat.id: "navDownButton"

            onClicked: Player.navigateDown()
        }
    }

    background: Rectangle {
        id: navBoxBackgound
        color: "black"
        opacity: 0.4
        radius: HeimdallStyle.navBoxButton_radius
        border.color: theme.bg.secondary
        border.width: HeimdallStyle.border
        border.pixelAligned: (radius < Number.EPSILON)

        MouseArea {
            id: mouseArea

            anchors.fill: parent

            cursorShape: (mouseArea.drag.active || mouseArea.pressed) ? Qt.DragMoveCursor : Qt.OpenHandCursor

            drag.target: navigationBox

            drag.minimumX: navigationBox.dragXMin
            drag.minimumY: navigationBox.dragYMin
            drag.maximumX: navigationBox.dragXMax
            drag.maximumY: navigationBox.dragYMax

            drag.smoothed: false

            hoverEnabled: true

            drag.onActiveChanged: {
                if (drag.active) {
                    drag.target.Drag.start()
                } else {
                    drag.target.Drag.drop()
                }
            }
        }
    }
}
