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
import QtQml.Models

import Heimdall.MediaLibrary

import Heimdall.Widgets as Widgets
import Heimdall.MainInterface
import Heimdall.Util
import Heimdall.Style

Widgets.TableViewExt {
    id: root

    Accessible.name: qsTr("Playlist media")
    AccessibleCompat.id: "playlistMedia"

    //---------------------------------------------------------------------------------------------
    // Properties
    //---------------------------------------------------------------------------------------------

    property bool isMusic

    //---------------------------------------------------------------------------------------------
    // Private

    property Item _item: null

    property bool _before: true

    property var _modelSmall: [{
        weight: 1,

        model: {
            criteria: "title",

            subCriterias: [ "duration" ],

            text: qsTr("Title"),

            isSortable: false,

            headerDelegate: table.titleHeaderDelegate,
            colDelegate   : table.titleDelegate,

            placeHolder: HeimdallStyle.noArtAlbumCover
        }
    }]

    property var _modelMedium: [{
        weight: 1,

        model: {
            criteria: "title",

            text: qsTr("Title"),

            isSortable: false,

            headerDelegate: table.titleHeaderDelegate,
            colDelegate   : table.titleDelegate,

            placeHolder: HeimdallStyle.noArtAlbumCover
        }
    }, {
        size: 1,

        model: {
            criteria: "duration",

            text: qsTr("Duration"),

            isSortable: false,

            headerDelegate: table.timeHeaderDelegate,
            colDelegate   : table.timeColDelegate
        }
    }]

    //---------------------------------------------------------------------------------------------
    // Settings
    //---------------------------------------------------------------------------------------------

    rowHeight: HeimdallStyle.tableCoverRow_height

    sortModel: (_availableRowWidth < HeimdallStyle.colWidth(4)) ? _modelSmall
                                                           : _modelMedium


    acceptDropFunc: function(index, drop) {
        // FIXME: The DnD API seems quite poorly designed in this file.
        //        Why does it ask for both index and "before"
        //        When index + 1 is essentially the same as
        //        before being false?
        //        What is "delegate" and why is it passed in applyDrop()?
        //        We have to come up with a shim function here...
        return applyDrop(drop, index - 1, null, false)
    }

    //---------------------------------------------------------------------------------------------
    // Events
    //---------------------------------------------------------------------------------------------

    onActionForSelection: (selection) => model.addAndPlay( selection )
    onItemDoubleClicked: (index, model) => MediaLib.addAndPlay(model.id)

    //---------------------------------------------------------------------------------------------
    // Functions
    //---------------------------------------------------------------------------------------------
    // Drop interface

    isDropAcceptableFunc: function(drop, index) {
        if (drop.source === dragItem) {
            return Helpers.itemsMovable(selectionModel.sortedSelectedIndexesFlat, index)
        } else if (Helpers.isValidInstanceOf(drop.source, Widgets.DragItem)) {
            return true
        } else if (drop.hasUrls) {
            return true
        } else {
            return false
        }
    }

    function applyDrop(drop, index, delegate, before) {
        if (isDropAcceptableFunc(drop, index + (before ? 0 : 1)) === false) {
            drop.accepted = false
            return Promise.resolve()
        }

        const item = drop.source;

        const destinationIndex = before ? index : (index + 1)

        // NOTE: Move implementation.
        if (dragItem === item) {
            model.move(selectionModel.selectedRows(), destinationIndex)
            root.forceActiveFocus()
        // NOTE: Dropping medialibrary content into the playlist.
        } else if (Helpers.isValidInstanceOf(item, Widgets.DragItem)) {
            return item.getSelectedInputItem()
                        .then(inputItems => {
                            model.insert(inputItems, destinationIndex)
                        })
                        .then(() => { root.forceActiveFocus(); })
        } else if (drop.hasUrls) {
            const urlList = []
            for (let url in drop.urls)
                urlList.push(drop.urls[url])

            model.insert(urlList, destinationIndex)

            root.forceActiveFocus()
        }

        return Promise.resolve()
    }

    //---------------------------------------------------------------------------------------------
    // Childs
    //---------------------------------------------------------------------------------------------

    MLTableColumns {
        id: table

        titleCover_width: isMusic ? HeimdallStyle.trackListAlbumCover_width
                                  : HeimdallStyle.listAlbumCover_width
        titleCover_height: isMusic ? HeimdallStyle.trackListAlbumCover_heigth
                                   : HeimdallStyle.listAlbumCover_height
        titleCover_radius: isMusic ? HeimdallStyle.trackListAlbumCover_radius
                                   : HeimdallStyle.listAlbumCover_radius

        showCriterias: (root.sortModel === root._modelSmall)

        criteriaCover: "thumbnail"

        //-----------------------------------------------------------------------------------------
        // TableColumns implementation

        function titlecoverLabels(model) {
            return [
                (model) ? model.resolution_name : "",
                (model) ? model.channel         : ""
            ].filter(function(a) { return a !== "" })
        }
    }

    Rectangle {
        id: line

        anchors.left : parent.left
        anchors.right: parent.right

        height: HeimdallStyle.dp(1, HeimdallStyle.scale)

        visible: root._item !== null

        color: root.colorContext.accent
    }
}
