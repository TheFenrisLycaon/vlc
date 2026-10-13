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
import QtQuick.Layouts


import Heimdall.MainInterface
import Heimdall.Widgets as Widgets
import Heimdall.Style
import Heimdall.Network

Widgets.PageExt {
    id: root

    title: qsTr("Install services")

    Widgets.ListViewExt {
        id: servicesView

        // required by g_root to indicate view with 'grid' or 'list' mode
        readonly property bool hasGridListMode: false
        readonly property bool isSearchable: true

        property var pagePrefix: []

        anchors.fill: parent


        model: ServicesDiscoveryModel {
            id: discoveryModel

            ctx: MainCtx

            typeFilter: ServicesDiscoveryModel.TYPE_SERVICE_DISCOVERY
            searchPattern: root.search.pattern
            sortOrder: root.sort.order
            sortCriteria: root.sort.criteria

            onLoadingChanged: {
                // Adjust the cursor. Unless the loaded item (view) sets a cursor
                // globally or for itself, this is going to be respected. It should
                // be noted that cursor adjustment is conventionally not delayed,
                // unlike indicators:
                if (loading) {
                    MainCtx.setCursor(servicesView, Qt.BusyCursor)
                } else {
                    MainCtx.unsetCursor(servicesView)
                }
            }

            Component.onCompleted: {
                discoveryModel.loadFromDefaultRepository()
                loadingChanged() // make sure the handler is called
            }
        }

        topMargin: HeimdallStyle.margin_large
        leftMargin: HeimdallStyle.margin_large
        rightMargin: HeimdallStyle.margin_large
        spacing: HeimdallStyle.margin_xsmall

        delegate: Rectangle {
            width: servicesView.width - HeimdallStyle.margin_large * 2
            height: row.implicitHeight + HeimdallStyle.margin_small * 2
            color: servicesView.colorContext.bg.secondary

            onActiveFocusChanged: if (activeFocus) action_btn.forceActiveFocus()

            RowLayout {
                id: row

                spacing: HeimdallStyle.margin_xsmall
                anchors.fill: parent
                anchors.margins: HeimdallStyle.margin_small

                Image {

                    width: HeimdallStyle.icon_large
                    height: HeimdallStyle.icon_large
                    fillMode: Image.PreserveAspectFit
                    source: model.artwork

                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                }

                ColumnLayout {
                    id: content

                    spacing: 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    RowLayout {
                        spacing: 0

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Column {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Widgets.SubtitleLabel {
                                text: model.name
                                width: parent.width
                                color: servicesView.colorContext.fg.primary
                            }

                            Widgets.CaptionLabel {
                                color: servicesView.colorContext.fg.primary
                                textFormat: Text.StyledText
                                text: model.author ? qsTr("by <b>%1</b>").arg(model.author) : qsTr("by <b>Unknown</b>")
                                topPadding: HeimdallStyle.margin_xxxsmall
                                width: parent.width
                            }
                        }

                        Widgets.ButtonExt {
                            id: action_btn

                            focus: true
                            iconTxt: model.state === ServicesDiscoveryModel.STATE_INSTALLED ? HeimdallIcons.del : HeimdallIcons.add
                            busy: model.state === ServicesDiscoveryModel.STATE_INSTALLING || model.state === ServicesDiscoveryModel.STATE_UNINSTALLING
                            text: {
                                switch(model.state) {
                                case ServicesDiscoveryModel.STATE_INSTALLED:
                                    return qsTr("Remove")
                                case ServicesDiscoveryModel.STATE_NOTINSTALLED:
                                    return qsTr("Install")
                                case ServicesDiscoveryModel.STATE_INSTALLING:
                                    return qsTr("Installing")
                                case ServicesDiscoveryModel.STATE_UNINSTALLING:
                                    return qsTr("Uninstalling")
                                }
                            }

                            onClicked: {
                                if (model.state === ServicesDiscoveryModel.STATE_NOTINSTALLED)
                                    discoveryModel.installService(index)
                                else if (model.state === ServicesDiscoveryModel.STATE_INSTALLED)
                                    discoveryModel.removeService(index)
                            }
                        }
                    }

                    Widgets.CaptionLabel {
                        elide: Text.ElideRight
                        text:  model.description || model.summary || qsTr("No information available")
                        color: servicesView.colorContext.fg.secondary
                        topPadding: HeimdallStyle.margin_xsmall
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                    }

                    Widgets.CaptionLabel {
                        text: qsTr("Score: %1/5  Downloads: %2")
                            .arg( (5 * model.score / discoveryModel.maxScore).toFixed(1) )
                            .arg(model.downloads)
                        topPadding: HeimdallStyle.margin_xsmall
                        color: servicesView.colorContext.fg.secondary
                        Layout.fillWidth: true
                    }
                }
            }
        }

        Widgets.BusyIndicatorExt {
            runningDelayed: discoveryModel.loading
            anchors.centerIn: parent
            color: servicesView.colorContext.fg.primary
            z: 1
        }
    }
}
