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
import QtQml.Models

import Heimdall.MediaLibrary

import Heimdall.Util
import Heimdall.Widgets as Widgets
import Heimdall.MainInterface
import Heimdall.Style

Widgets.TableViewExt {
    id: listView_id

    //---------------------------------------------------------------------------------------------
    // Properties
    //---------------------------------------------------------------------------------------------

    // NOTE: This is useful for groups because our main criteria is 'name' instead of 'title'.
    property string mainCriteria: "title"

    property var coverLabels

    property bool showGroupCountColumn: false

    property real coverHeight: HeimdallStyle.listAlbumCover_height
    property real coverWidth: HeimdallStyle.listAlbumCover_width
    property real coverRadius: HeimdallStyle.listAlbumCover_radius

    //---------------------------------------------------------------------------------------------
    // Private

    property var _modelSmall: [{
        weight: 1,

        model: ({
            criteria: mainCriteria,

            subCriterias: showGroupCountColumn ? [ "nb_videos", "duration" ]
                                               : [ "duration" ],

            showSection: "title",

            text: qsTr("Title"),

            placeHolder: HeimdallStyle.noArtVideoCover,

            headerDelegate: tableColumns.titleHeaderDelegate,
            colDelegate   : tableColumns.titleDelegate
        })
    }]

    property var _modelMedium: (function(){
        const medium = [{
            weight: 1,

            model: ({
                criteria: mainCriteria,

                showSection: "title",

                text: qsTr("Title"),

                headerDelegate: tableColumns.titleHeaderDelegate,
                colDelegate   : tableColumns.titleDelegate,

                placeHolder: HeimdallStyle.noArtVideoCover
            })
        }]

        if (showGroupCountColumn) {
            medium.push({
                size: 0.5,
                model: ({
                    criteria: "nb_videos",
                    text: qsTr("Videos"),
                    isSortable: false
                })
            })
        }

        medium.push({
            size: showGroupCountColumn ? 0.5 : 1,

            model: ({
                criteria: "duration",

                text: qsTr("Duration"),

                showSection: "",
                showContextButton: true,

                headerDelegate: tableColumns.timeHeaderDelegate,
                colDelegate   : tableColumns.timeColDelegate
            })
        })

        return medium
    })()

    // Settings

    sortModel: (_availableRowWidth < HeimdallStyle.colWidth(4)) ? _modelSmall
                                                           : _modelMedium

    section.property: "title_first_symbol"

    rowHeight: HeimdallStyle.tableCoverRow_height

    //---------------------------------------------------------------------------------------------
    // Childs
    //---------------------------------------------------------------------------------------------

    MLTableColumns {
        id: tableColumns

        showCriterias: (listView_id.sortModel === listView_id._modelSmall)

        criteriaCover: "thumbnail"

        titleCover_height: listView_id.coverHeight
        titleCover_width: listView_id.coverWidth
        titleCover_radius: listView_id.coverRadius

        titlecoverLabels: listView_id.coverLabels
    }
}
