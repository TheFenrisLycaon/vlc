/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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
import QtQml.Models
import QtQuick.Window


import Heimdall.MainInterface
import Heimdall.Style
import Heimdall.Playlist
import Heimdall.Widgets as Widgets
import Heimdall.Menus as Menus
import Heimdall.Util

T.ToolBar {
    id: root

    Accessible.name: qsTr("Menu")
    AccessibleCompat.id: "menuTopbar"

    // For now, used for d&d functionality
    // Not strictly necessary to set
    property PlaylistPane plListView: null

    property bool _showCSD: MainCtx.clientSideDecoration && !(MainCtx.intfMainWindow.visibility === Window.FullScreen)

    hoverEnabled: true

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    ColorContext {
        id: theme
        colorSet: ColorContext.Window
    }

    background: Widgets.AcrylicBackground {
        tintColor: theme.bg.secondary
        alternativeColor: theme.bg.primary
    }

    contentItem:  Item {
        implicitHeight: menubar.implicitHeight
            + HeimdallStyle.applicationVerticalMargin
            + HeimdallStyle.margin_xxsmall

        //drag and dbl click the titlebar in CSD mode
        Loader {
            z:-1
            anchors.fill: parent
            active: root._showCSD
            sourceComponent: Widgets.CSDTitlebarTapNDrapHandler {
            }
        }

        Menus.Menubar {
            id: menubar

            anchors {
                left: parent.left
                leftMargin: HeimdallStyle.applicationHorizontalMargin

                right: parent.right
                rightMargin: root._showCSD && !(csdButtons.height < HeimdallStyle.applicationVerticalMargin) ? csdButtons.width
                            : HeimdallStyle.applicationHorizontalMargin

                top: parent.top
                topMargin: HeimdallStyle.applicationVerticalMargin
            }
            visible: MainCtx.hasToolbarMenu
            enabled: visible

            Navigation.parentItem: root
        }

        Loader {
            id: csdButtons
            anchors.right: parent.right
            height: Math.min(parent.implicitHeight, HeimdallStyle.globalToolbar_height)
            active: root._showCSD
            source: HeimdallStyle.palette.hasCSDImage
                      ? "qrc:///qt/qml/Heimdall/Widgets/CSDThemeButtonSet.qml"
                      : "qrc:///qt/qml/Heimdall/Widgets/CSDWindowButtonSet.qml"
        }

        Keys.priority: Keys.AfterItem
        Keys.onPressed: (event) => root.Navigation.defaultKeyAction(event)
    }
}
