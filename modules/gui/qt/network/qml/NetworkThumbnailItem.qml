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
import QtQml.Models

import Heimdall.Widgets as Widgets
import Heimdall.Style
import Heimdall.Network
import Heimdall.Util

Widgets.TableRowDelegate {
    id: root

    readonly property string artworkSource: rowModel?.artwork ?? ""

    readonly property bool _showPlayCover: (currentlyFocused || containsMouse)
                                           && !!rowModel
                                           && (rowModel.type !== NetworkMediaModel.TYPE_NODE)
                                           && (rowModel.type !== NetworkMediaModel.TYPE_DIRECTORY)
                                           && (rowModel.type !== NetworkMediaModel.TYPE_PLAYLIST)

    property alias artworkTextureProvider: cover.textureProvider

    signal playClicked(int index)

    // Functions

    function getCriterias(colModel, rowModel) {
        if (colModel === null || rowModel === null)
            return ""

        const criterias = colModel.subCriterias

        if (criterias === undefined || criterias.length === 0)
            return ""

        let string = ""

        for (let i = 0; i < criterias.length; i++) {
            const criteria = criterias[i]

            const value = rowModel[criteria]

            if (value.toString() === "heimdall://nop")
                continue

            if (i) string += " • "

            string += value
        }

        return string
    }

    Row {
        anchors.fill: parent
        spacing: HeimdallStyle.margin_normal

        Widgets.MediaCover {
            id: cover

            anchors.verticalCenter: parent.verticalCenter

            width: HeimdallStyle.listAlbumCover_width
            height: HeimdallStyle.listAlbumCover_height

            pictureWidth: 0 // preserve aspect ratio
            pictureHeight: height

            fillMode: Image.PreserveAspectFit

            source: {
                if (root.rowModel?.artwork && root.rowModel.artwork.length > 0)
                    return HeimdallAccessImage.uri(root.rowModel.artwork)

                return ""
            }

            fallbackImageSource: {
                if (!root.rowModel)
                    return ""

                let img = SVGColorImage.colorize(root.rowModel.artworkFallback)
                                       .color1(root.colorContext.fg.primary)
                                       .accent(root.colorContext.accent)

                if (GraphicsInfo.shaderType !== GraphicsInfo.RhiShader)
                    img = img.background(cover.color)

                return img.uri()
            }

            color: root.colorContext.bg.secondary

            playCoverShowPlay: root._showPlayCover
            playIconSize: HeimdallStyle.play_cover_small

            onPlayIconClicked: () => {
                root.playClicked(root.index)
            }

            Widgets.DefaultShadow {

            }
        }

        Column {
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            anchors.topMargin: HeimdallStyle.margin_xxsmall
            anchors.bottomMargin: HeimdallStyle.margin_xxsmall

            width: Math.max(0, parent.width - x)

            Widgets.TextAutoScroller {
                id: itemText

                anchors.left: parent.left
                anchors.right: parent.right

                height: (itemCriterias.visible) ? Math.round(parent.height / 2)
                                                : parent.height

                visible: (listLabel.text)

                clip: scrolling

                label: listLabel

                forceScroll: root.currentlyFocused

                Widgets.ListLabel {
                    id: listLabel

                    anchors.verticalCenter: parent.verticalCenter

                    text: root.rowModel?.[root.colModel.title] ?? ""

                    color: root.selected
                        ? root.colorContext.fg.highlight
                        : root.colorContext.fg.primary
                }
            }

            Widgets.MenuCaption {
                id: itemCriterias

                anchors.left: parent.left
                anchors.right: parent.right

                height: itemText.height

                visible: (text)

                color: root.selected
                    ? root.colorContext.fg.highlight
                    : root.colorContext.fg.secondary

                text: root.getCriterias(root.colModel, root.rowModel)
            }
        }
    }
}
