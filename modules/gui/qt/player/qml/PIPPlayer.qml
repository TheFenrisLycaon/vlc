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

import Heimdall.MainInterface
import Heimdall.Style
import Heimdall.Widgets as Widgets
import Heimdall.Playlist
import Heimdall.Player
import Heimdall.Util

T.Control {
    id: root
    width: Math.round(HeimdallStyle.dp(320, HeimdallStyle.scale))
    height: Math.round(HeimdallStyle.dp(180, HeimdallStyle.scale))

    objectName: "pip window"

    property real dragXMin: 0
    property real dragXMax: 0
    property real dragYMin: undefined
    property real dragYMax: undefined

    property int textStyle: Text.Outline

    // `hideControls` being `false` does not necessarily mean that the buttons are shown:
    property bool hideControls: MainCtx.usingTouch // By default controls are hidden with touch

    Accessible.role: Accessible.Graphic
    Accessible.focusable: false
    Accessible.name: qsTr("video content")

    Drag.active: dragHandler.active

    Drag.onActiveChanged: {
        root.anchors.left = undefined
        root.anchors.right = undefined
        root.anchors.top = undefined
        root.anchors.bottom = undefined
        root.anchors.verticalCenter = undefined
        root.anchors.horizontalCenter = undefined
    }

    DoubleClickIgnoringItem {
        anchors.fill: parent

        TapHandler {
            gesturePolicy: TapHandler.WithinBounds

            onDoubleTapped: (eventPoint, button) => {
                // Docs are lying, there is no `deviceType`: QTBUG-135104
                if (eventPoint.device.type !== PointerDevice.TouchScreen)
                    MainCtx.playerView = true
            }

            onTapped: (eventPoint, button) => {
                // Docs are lying, there is no `deviceType`: QTBUG-135104
                if (eventPoint.device.type === PointerDevice.TouchScreen) {
                    // Breaking the initial binding is intentional here:
                    root.hideControls = !root.hideControls
                } else {
                    MainPlaylistController.togglePlayPause()
                }
            }
        }

        DragHandler {
            id: dragHandler

            target: root

            cursorShape: Qt.DragMoveCursor

            dragThreshold: 0

            grabPermissions: PointerHandler.CanTakeOverFromAnything

            xAxis.minimum: root.dragXMin
            xAxis.maximum: root.dragXMax
            yAxis.minimum: root.dragYMin
            yAxis.maximum: root.dragYMax
        }

        HoverHandler {
            id: hoverHandler

            grabPermissions: PointerHandler.CanTakeOverFromAnything
            cursorShape: Qt.ArrowCursor

            Component.onCompleted: {
                if (typeof blocking === 'boolean')
                    blocking = true // Qt 6.3 feature
            }
        }
    }

    background: VideoSurface {
        id: videoSurface
        videoSurfaceProvider: MainCtx.videoSurfaceProvider

        color: (hoverHandler.hovered ||
                playButton?.hovered ||
                closeButton?.hovered ||
                fullscreenButton?.hovered) ? "#10000000" : "transparent"

        // It is acceptable for effects to exceed the boundaries of the
        // parent. They are an exception to the "hypothetical clip test".
        Widgets.DefaultShadow {

        }
    }

    contentItem: Item {
        visible: !root.hideControls &&
                 (hoverHandler.hovered ||
                 playButton.hovered ||
                 closeButton.hovered ||
                 fullscreenButton.hovered)

        // Raise the content item so that the handlers of the control do
        // not handle events that are to be handled by the handlers/items
        // of the content item. Raising the content item should be fine
        // because content item is supposed to be the foreground item.
        z: 1

        Widgets.IconButton {
            id: playButton

            anchors.centerIn: parent

            font.pixelSize: HeimdallStyle.icon_large

            description: qsTr("play/pause")

            AccessibleCompat.id: "pipPlayButton"

            text: (Player.playingState !== Player.PLAYING_STATE_PAUSED
                   && Player.playingState !== Player.PLAYING_STATE_STOPPED)
                  ? HeimdallIcons.pause_filled
                  : HeimdallIcons.play_filled

            textStyle: root.textStyle

            onClicked: MainPlaylistController.togglePlayPause()
        }

        Widgets.IconButton {
            id: closeButton

            anchors {
                top: parent.top
                topMargin: HeimdallStyle.margin_small
                right: parent.right
                rightMargin: HeimdallStyle.margin_small
            }

            font.pixelSize: HeimdallStyle.icon_PIP
            description: qsTr("close video")

            AccessibleCompat.id: "pipCloseButton"

            text: HeimdallIcons.close

            textStyle: root.textStyle

            onClicked: MainPlaylistController.stop()
        }

        Widgets.IconButton {
            id: fullscreenButton

            anchors {
                top: parent.top
                topMargin: HeimdallStyle.margin_small
                left: parent.left
                leftMargin: HeimdallStyle.margin_small
            }

            font.pixelSize: HeimdallStyle.icon_PIP

            description: qsTr("maximize player")

            AccessibleCompat.id: "pipFullscreenButton"

            text: HeimdallIcons.fullscreen

            textStyle: root.textStyle

            onClicked: MainCtx.playerView = true
        }
    }
}
