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
import QtQuick.Window
import QtQuick.Layouts
import QtQml.Models

import Heimdall.MainInterface
import Heimdall.MediaLibrary

import Heimdall.Widgets as Widgets
import Heimdall.Util
import Heimdall.Style

FocusScope {
    id: root

    property var model

    property int index
    property Item view // can not use `ItemView` because of `ExpandGridView`

    property var headerFocusScope
    property var enqueueActionBtn
    property var playActionBtn

    property bool forcePlayActionBtnFocusOnce: false

    signal retract()

    implicitWidth: layout.implicitWidth

    implicitHeight: {
        const verticalMargins = layout.anchors.topMargin + layout.anchors.bottomMargin
        if (tracks.contentHeight < artAndControl.height)
            return artAndControl.height + verticalMargins
        return Math.min(tracks.contentHeight
                        , tracks.listView.headerItem.height + tracks.rowHeight * 6) // show a maximum of 6 rows
                + verticalMargins
    }

    // components should shrink with change of height, but it doesn't happen fast enough
    // causing expand and shrink animation bit laggy, so clip the delegate to fix it
    clip: true

    function setCurrentItemFocus(reason) {
        root.playActionBtn.forceActiveFocus(reason);
        if (HeimdallStyle.isScreenSmall)
            root.forcePlayActionBtnFocusOnce = true;
    }

    function _getStringTrack() {
        const count = model?.nb_tracks ?? 0;

        return qsTr("%1 track", "%1 tracks", count).arg(count);
    }

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View
    }

    Rectangle {
        id: background

        anchors.fill: parent
        color: theme.bg.secondary

        Rectangle {
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
            color: theme.border
            height: HeimdallStyle.expandDelegate_border
        }

        Rectangle {
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            color: theme.border
            height: HeimdallStyle.expandDelegate_border
        }
    }

    RowLayout {
        id: layout

        anchors.fill: parent
        anchors.leftMargin: HeimdallStyle.margin_normal
        anchors.topMargin: HeimdallStyle.margin_large
        anchors.rightMargin: HeimdallStyle.margin_small
        anchors.bottomMargin: HeimdallStyle.margin_xxsmall
        spacing: HeimdallStyle.margin_large

        Component {
            id: cover

            Widgets.ImageExt {
                id: expand_cover_id

                property int cover_height: parent.cover_height
                property int cover_width: parent.cover_width

                height: cover_height
                width: cover_width
                radius: HeimdallStyle.expandCover_music_radius
                source: (root.model && root.model.cover && root.model.cover !== "")
                    ?  root.model.cover
                    : HeimdallStyle.noArtAlbumCover
                sourceSize: Qt.size(width * eDPR, height * eDPR)
                backgroundColor: theme.bg.primary
                textureProviderItem: root.view?.itemAtIndex(root.index)?.artworkTextureProvider ?? null


                readonly property real eDPR: MainCtx.effectiveDevicePixelRatio(Window.window)

                Widgets.DefaultShadow {
                    visible: (parent.status === Image.Ready)
                }
            }
        }

        Component {
            id: buttons

            Widgets.NavigableRow {
                id: actionButtons

                property alias enqueueActionBtn: _enqueueActionBtn
                property alias playActionBtn: _playActionBtn

                focus: true
                width: HeimdallStyle.expandCover_music_width

                spacing: HeimdallStyle.margin_small

                Layout.alignment: Qt.AlignCenter

                Widgets.ActionButtonPrimary {
                    id: _playActionBtn

                    iconTxt: HeimdallIcons.play
                    text: qsTr("Play")
                    onClicked: MediaLib.addAndPlay( root.model.id )

                    onActiveFocusChanged: {
                        // root.setCurrentItemFocus sets active focus to playActionBtn, but it gets stolen
                        // by the delegate of the first track at initial load when playActionBtn is in the
                        // header of tracks
                        if (HeimdallStyle.isScreenSmall && root.forcePlayActionBtnFocusOnce) {
                            root.forcePlayActionBtnFocusOnce = false
                            root.playActionBtn.forceActiveFocus(Qt.TabFocusReason)
                        }
                    }
                }

                Widgets.ButtonExt {
                    id: _enqueueActionBtn

                    iconTxt: HeimdallIcons.enqueue
                    text: qsTr("Enqueue")
                    onClicked: MediaLib.addToPlaylist( root.model.id )
                }


                Navigation.parentItem: root
                Navigation.rightItem: HeimdallStyle.isScreenSmall ? root.headerFocusScope : tracks
                Navigation.upItem: HeimdallStyle.isScreenSmall ? root.headerFocusScope : null

                Navigation.downAction: function () {
                    if (!HeimdallStyle.isScreenSmall)
                        return

                    if (tracks.count > 0) {
                        tracks.setCurrentItemFocus(Qt.TabFocusReason)
                    } else {
                        root.Navigation.downAction()
                    }
                }
            }
        }

        Component {
            id: header_common

            FocusScope {
                id: headerFocusScope

                property int bottomPadding: parent.bottomPadding

                width: parent.width
                height: implicitHeight
                implicitHeight: col.implicitHeight

                focus: true

                Navigation.parentItem: root
                Navigation.leftItem: root.enqueueActionBtn
                Navigation.downAction: function () {
                    if (HeimdallStyle.isScreenSmall) {
                        root.enqueueActionBtn.forceActiveFocus(Qt.TabFocusReason);
                        return
                    }

                    if (tracks.count > 0) {
                        tracks.setCurrentItemFocus(Qt.TabFocusReason)
                    } else {
                        root.Navigation.downAction()
                    }
                }

                Column {
                    id: col

                    anchors.fill: parent
                    bottomPadding: headerFocusScope.bottomPadding

                    RowLayout {
                        width: parent.width

                        /* The title of the albums */
                        Widgets.SubtitleLabel {
                            id: expand_infos_title_id

                            text: root.model?.title || qsTr("Unknown title")

                            color: theme.fg.primary

                            Layout.fillWidth: true
                        }

                        Widgets.IconToolButton {
                            text: HeimdallIcons.close
                            focus: true

                            description: qsTr("Close Panel")

                            Navigation.parentItem: headerFocusScope
                            Layout.rightMargin: HeimdallStyle.margin_small

                            onClicked: root.retract()
                        }
                    }

                    Widgets.CaptionLabel {
                        id: expand_infos_subtitle_id

                        color: theme.fg.secondary

                        width: parent.width

                        text: qsTr("%1 - %2 - %3 - %4")
                            .arg(root.model?.main_artist || qsTr("Unknown artist"))
                            .arg(root.model?.release_year || "")
                            .arg(_getStringTrack())
                            .arg(root.model?.duration?.formatHMS() ?? 0)
                    }
                }
            }
        }

        Component {
            id: header_small

            RowLayout {
                id: row

                width: parent.width
                implicitHeight: col.implicitHeight

                Loader {
                    sourceComponent: cover
                    property int cover_height: HeimdallStyle.cover_small
                    property int cover_width: HeimdallStyle.cover_small

                    Layout.bottomMargin: HeimdallStyle.margin_large
                    Layout.rightMargin: HeimdallStyle.margin_xxsmall
                }

                Column {
                    id: col

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.bottomMargin: HeimdallStyle.margin_large

                    Loader {
                        sourceComponent: header_common
                        width: parent.width
                        property int bottomPadding: HeimdallStyle.margin_xsmall

                        onLoaded: {
                            root.headerFocusScope = item
                        }
                    }

                    Loader {
                        sourceComponent: buttons

                        onLoaded: {
                            root.enqueueActionBtn = item.enqueueActionBtn
                            root.playActionBtn = item.playActionBtn
                        }
                    }
                }
            }
        }

        FocusScope {
            id: artAndControl

            visible: !HeimdallStyle.isScreenSmall
            focus: !HeimdallStyle.isScreenSmall

            implicitHeight: artAndControlLayout.implicitHeight
            implicitWidth: artAndControlLayout.implicitWidth
            Layout.alignment: Qt.AlignTop

            Column {
                id: artAndControlLayout

                spacing: HeimdallStyle.margin_normal
                bottomPadding: HeimdallStyle.margin_large

                /* A bigger cover for the album */
                Loader {
                    sourceComponent: !HeimdallStyle.isScreenSmall ? cover : null
                    property int cover_height: HeimdallStyle.expandCover_music_height
                    property int cover_width: HeimdallStyle.expandCover_music_width
                }

                Loader {
                    sourceComponent: !HeimdallStyle.isScreenSmall ? buttons : null

                    onLoaded: {
                        root.playActionBtn = item.playActionBtn
                        root.enqueueActionBtn = item.enqueueActionBtn
                    }
                }
            }
        }

        /* The list of the tracks available */
        MusicTrackListDisplay {
            id: tracks

            readonly property var _titleModel: [{
                weight: 1,

                model: {
                    criteria: "title",

                    visible: true,

                    text: HeimdallStyle.isScreenSmall ? qsTr("Tracks") : qsTr("Title"),

                    showSection: "",

                    subCriterias: ["track_number", "duration"],

                    colDelegate: tableColumns.titleTextDelegate,
                    headerDelegate: tableColumns.titleTextHeaderDelegate
                }
            }]

            readonly property var _allModel: [
            {
                size: .2,

                model: {
                    criteria: "track_number",

                    visible: true,

                    text: qsTr("#"),

                    showSection: "",

                    hCenterText: true
                }
            },
                ..._titleModel,
            {
                size: 1,

                model: {
                    criteria: "duration",

                    visible: true,

                    text: qsTr("Duration"),

                    showSection: "",

                    colDelegate: tableColumns.timeColDelegate,
                    headerDelegate: tableColumns.timeHeaderDelegate
                }
            }]

            fadingEdge.backgroundColor: background.color

            preferredHeader: Loader {
                sourceComponent: HeimdallStyle.isScreenSmall
                                 ? header_small
                                 : header_common
                width: tracks.width

                property int bottomPadding: HeimdallStyle.margin_large //used only by header_common
            }

            clip: true // content may overflow if not enough space is provided
            headerPositioning: ListView.InlineHeader
            section.property: ""

            Layout.fillWidth: true
            Layout.fillHeight: true

            rowHeight: HeimdallStyle.tableRow_height

            parentId: root.model?.id
            onParentIdChanged: {
                currentIndex = 0
            }

            sortModel: HeimdallStyle.isScreenSmall
                       ? _titleModel // use criterias text with small screens
                       : _allModel

            Navigation.parentItem: root
            Navigation.leftItem: HeimdallStyle.isScreenSmall ? null : root.enqueueActionBtn
            Navigation.upItem: headerItem


            MLTableColumns {
                id: tableColumns

                showCriterias: HeimdallStyle.isScreenSmall
            }
        }
    }


    Keys.priority:  Keys.AfterItem
    Keys.onPressed: (event) =>  root.Navigation.defaultKeyAction(event)
}
