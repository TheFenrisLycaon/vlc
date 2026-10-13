import QtQuick

import Heimdall.MainInterface
import Heimdall.Style
import Heimdall.Util
import Heimdall.Menus

NavigableRow {
    id: root

    spacing: HeimdallStyle.margin_normal
    enabled: gridListBtn.visible || sortControl.visible || searchBox.visible

    property SortMenu sortMenu: null
    required property SortCtx sort
    required property SearchCtx search


    //TODO: visible value of root.sort.available and MainCtx.hasGridListMode is initialised correctly but on first load still shows up
    IconToolButton {
        id: gridListBtn

        visible: MainCtx.hasGridListMode

        enabled: visible
        width: HeimdallStyle.bannerButton_width
        height: HeimdallStyle.bannerButton_height
        font.pixelSize: HeimdallStyle.icon_banner
        text: MainCtx.gridView ? HeimdallIcons.list : HeimdallIcons.grid
        description: qsTr("List/Grid")
        onClicked: MainCtx.gridView = !MainCtx.gridView
    }

    SortControl {
        id: sortControl

        visible: root.sort.available
        enabled: visible
        width: HeimdallStyle.bannerButton_width
        height: HeimdallStyle.bannerButton_height
        font.pixelSize: HeimdallStyle.icon_banner
        description: qsTr("Sort")

        model: root.sort.model

        menu: root.sortMenu

        sortKey:  root.sort.criteria
        sortOrder: root.sort.order

        onSortSelected: (key) => {
            root.sort.criteria = key
        }
        onSortOrderSelected: (type) => {
            root.sort.order = type
        }
    }

    SearchBox {
        id: searchBox

        //TODO: initialise visible value with MainCtx
        visible: root.search.available
        height: HeimdallStyle.bannerButton_height
        buttonWidth: HeimdallStyle.bannerButton_width

        Binding {
            target: root.search
            property: "pattern"
            value: searchBox.searchPattern
        }

        Connections {
            target: root.search
            function onAskShow() {
                searchBox.expandAndFocus()
            }
        }
    }
}
