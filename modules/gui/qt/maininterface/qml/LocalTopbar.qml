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

    Accessible.name: qsTr("Local top bar")
    AccessibleCompat.id: "localTopbar"

    property bool useAcrylic: true

    // For now, used for d&d functionality
    // Not strictly necessary to set
    property Item plListView

    property bool _showCSD: MainCtx.clientSideDecoration
        && !(MainCtx.intfMainWindow.visibility === Window.FullScreen)
        && !MainCtx.hasToolbarMenu

    //whether the navigation pannel is actually visible or not
    property alias navigationVisible: navigationVisibilty.checked
    property alias playqueueVisible: playlistBtn.checked

    property alias navigationButton: navigationVisibilty

    signal toggleNavigationVisibility()
    signal togglePlayqueueVisibility()

    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                             implicitContentWidth + leftPadding + rightPadding + (_showCSD ? csdButtons.implicitWidth : 0))

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.Window
    }

    background:  Widgets.AcrylicBackground {
        enabled: root.useAcrylic
        tintColor: theme.bg.primary
    }

    contentItem: Item {

        implicitHeight: HeimdallStyle.globalToolbar_height
        implicitWidth: navigationButtons.implicitWidth
                     + playqueueButtonsTop.implicitWidth
                     + HeimdallStyle.margin_xsmall * 2

        Item {
            id: navigationButtons

            implicitWidth: navigationButtonsRow.implicitWidth

            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
                leftMargin: HeimdallStyle.margin_xsmall
            }

            Navigation.parentItem: root

            Row {
                id: navigationButtonsRow

                anchors.verticalCenter: parent.verticalCenter
                spacing: HeimdallStyle.margin_normal

                Navigation.parentItem: root
                Navigation.rightItem: playqueueButtonsTop

                Widgets.IconToolButton {
                    id: navigationVisibilty

                    focus: true

                    text: HeimdallIcons.panel_left
                    description: qsTr("Show navigation menu")

                    AccessibleCompat.id: "sidebarToggleButton"

                    font.pixelSize: HeimdallStyle.icon_banner
                    height: HeimdallStyle.bannerButton_height
                    width: HeimdallStyle.bannerButton_width

                    onClicked: root.toggleNavigationVisibility()

                    Navigation.parentItem: navigationButtons
                    Navigation.rightItem: historyBack
                }

                Widgets.IconToolButton {
                    id: historyBack

                    font.pixelSize: HeimdallStyle.icon_banner
                    text: HeimdallIcons.back
                    description: qsTr("Previous")

                    AccessibleCompat.id: "historyBackButton"

                    height: HeimdallStyle.bannerButton_height
                    width: HeimdallStyle.bannerButton_width

                    onClicked: History.previous()
                    enabled: !History.previousEmpty

                    onEnabledChanged: {
                        if (historyBack.activeFocus)
                            navigationVisibilty.focus = true
                    }

                    Navigation.parentItem: navigationButtons
                    Navigation.leftItem: navigationVisibilty
                    Navigation.rightItem: playlistBtn
                }

                Widgets.BannerCone {
                    id: logo

                    sourceSize.width: HeimdallStyle.bannerButton_height
                    sourceSize.height: HeimdallStyle.bannerButton_height
                    color: theme.accent
                }
            }
        }

        Item {
            id: playqueueButtonsTop

            implicitWidth: playqueueButtonsTopRow.implicitWidth

            anchors {
                right: parent.right
                top: parent.top
                bottom: parent.bottom
                //put the buttons below the CSD if we have enough space
                rightMargin: HeimdallStyle.margin_xsmall
                    + ((root._showCSD && root.topPadding < csdButtons.height)
                        ? Math.max(csdButtons.width - root.rightPadding, 0)
                        : 0)
            }

            Navigation.parentItem: root

            Row {
                id: playqueueButtonsTopRow

                anchors.verticalCenter: parent.verticalCenter
                spacing: HeimdallStyle.margin_normal

                Navigation.parentItem: root
                Navigation.leftItem: navigationButtons

                PlaylistPane.PlaylistToggleButton {
                    id: playlistBtn

                    plListView: root.plListView

                    Navigation.parentItem: playqueueButtonsTop
                    Navigation.rightItem: menuBtn
                    Navigation.leftItem: historyBack

                    onClicked:  root.togglePlayqueueVisibility()
                }

                Widgets.IconToolButton {
                    id: menuBtn

                    visible: !MainCtx.hasToolbarMenu
                    font.pixelSize: HeimdallStyle.icon_banner
                    text: HeimdallIcons.more
                    description: qsTr("Menu")
                    width: HeimdallStyle.bannerButton_width
                    height: HeimdallStyle.bannerButton_height
                    checked: contextMenu.shown

                    Navigation.parentItem: playqueueButtonsTop
                    Navigation.leftItem: playlistBtn

                    onClicked: contextMenu.popup(this.mapToGlobal(0, height))

                    Menus.QmlGlobalMenu {
                        id: contextMenu
                        ctx: MainCtx
                    }
                }
            }
        }

        Loader {
            z: -1

            anchors.fill: parent
            active: root._showCSD
            sourceComponent: Widgets.CSDTitlebarTapNDrapHandler {
            }
        }
    }

    Loader {
        id: csdButtons

        //csd button are
        parent:  root
        anchors {
            right: parent.right
            top: parent.top
        }

        height: HeimdallStyle.globalToolbar_height
        active: root._showCSD
        source: HeimdallStyle.palette.hasCSDImage
                  ? "qrc:///qt/qml/Heimdall/Widgets/CSDThemeButtonSet.qml"
                  : "qrc:///qt/qml/Heimdall/Widgets/CSDWindowButtonSet.qml"
    }
}
