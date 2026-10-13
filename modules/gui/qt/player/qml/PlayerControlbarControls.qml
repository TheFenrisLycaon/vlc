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

pragma Singleton

import QtQml


import Heimdall.MainInterface
import Heimdall.PlayerControls
import Heimdall.Widgets as Widgets
import Heimdall.Style

QtObject {
    readonly property string controlPath : "qrc:///qt/qml/Heimdall/PlayerControls/"

    readonly property var controlList: [
        { id: ControlListModel.PLAY_BUTTON, file: "PlayButton.qml", label: HeimdallIcons.play_filled, text: qsTr("Play") },
        { id: ControlListModel.STOP_BUTTON, file: "StopButton.qml", label: HeimdallIcons.stop, text: qsTr("Stop") },
        { id: ControlListModel.OPEN_BUTTON, file: "OpenButton.qml", label: HeimdallIcons.eject, text: qsTr("Open") },
        { id: ControlListModel.PREVIOUS_BUTTON, file: "PreviousButton.qml", label: HeimdallIcons.previous, text: qsTr("Previous") },
        { id: ControlListModel.NEXT_BUTTON, file: "NextButton.qml", label: HeimdallIcons.next, text: qsTr("Next") },
        { id: ControlListModel.SLOWER_BUTTON, file: "SlowerButton.qml", label: HeimdallIcons.slower, text: qsTr("Slower") },
        { id: ControlListModel.FASTER_BUTTON, file: "FasterButton.qml", label: HeimdallIcons.faster, text: qsTr("Faster") },
        { id: ControlListModel.FULLSCREEN_BUTTON, file: "FullscreenButton.qml", label: HeimdallIcons.fullscreen, text: qsTr("Fullscreen") },
        { id: ControlListModel.EXTENDED_BUTTON, file: "ExtendedSettingsButton.qml", label: HeimdallIcons.effect_filter, text: qsTr("Extended panel") },
        { id: ControlListModel.PLAYLIST_BUTTON, file: "PlaylistButton.qml", label: HeimdallIcons.playlist, text: qsTr("Play Queue") },
        { id: ControlListModel.SNAPSHOT_BUTTON, file: "SnapshotButton.qml", label: HeimdallIcons.snapshot, text: qsTr("Snapshot") },
        { id: ControlListModel.RECORD_BUTTON, file: "RecordButton.qml", label: HeimdallIcons.record, text: qsTr("Record") },
        { id: ControlListModel.ATOB_BUTTON, file: "AtoBButton.qml", label: HeimdallIcons.atob, text: qsTr("A-B Loop") },
        { id: ControlListModel.FRAME_PREV_BUTTON, file: "FrameNextButton.qml", label: HeimdallIcons.next_frame, text: qsTr("Next Frame") },
        { id: ControlListModel.FRAME_NEXT_BUTTON, file: "FramePrevButton.qml", label: HeimdallIcons.previous_frame, text: qsTr("Previous Frame") },
        { id: ControlListModel.REVERSE_BUTTON, file: "ReverseButton.qml", label: HeimdallIcons.play_reverse, text: qsTr("Trickplay Reverse") },
        { id: ControlListModel.SKIP_BACK_BUTTON, file: "SkipBackButton.qml", label: HeimdallIcons.skip_back, text: qsTr("Step backward") },
        { id: ControlListModel.SKIP_FW_BUTTON, file: "SkipForwardButton.qml", label: HeimdallIcons.skip_for, text: qsTr("Step forward") },
        { id: ControlListModel.QUIT_BUTTON, file: "QuitButton.qml", label: HeimdallIcons.clear, text: qsTr("Quit") },
        { id: ControlListModel.RANDOM_BUTTON, file: "RandomButton.qml", label: HeimdallIcons.shuffle, text: qsTr("Random") },
        { id: ControlListModel.LOOP_BUTTON, file: "LoopButton.qml", label: HeimdallIcons.repeat_all, text: qsTr("Loop") },
        { id: ControlListModel.INFO_BUTTON, file: "InfoButton.qml", label: HeimdallIcons.info, text: qsTr("Information") },
        { id: ControlListModel.LANG_BUTTON, file: "LangButton.qml", label: HeimdallIcons.audiosub, text: qsTr("Open subtitles") },
        { id: ControlListModel.BOOKMARK_BUTTON, file: "BookmarkButton.qml", label: HeimdallIcons.bookmark, text: qsTr("Bookmark Button") },
        { id: ControlListModel.CHAPTER_PREVIOUS_BUTTON, file: "ChapterPreviousButton.qml", label: HeimdallIcons.dvd_prev, text: qsTr("Previous chapter") },
        { id: ControlListModel.CHAPTER_NEXT_BUTTON, file: "ChapterNextButton.qml", label: HeimdallIcons.dvd_next, text: qsTr("Next chapter") },
        { id: ControlListModel.VOLUME, file: "VolumeWidget.qml", label: HeimdallIcons.volume_high, text: qsTr("Volume Widget") },
        { id: ControlListModel.NAVIGATION_BOX, file: "NavigationBoxButton.qml", label: HeimdallIcons.ic_fluent_arrow_move, text: qsTr("Navigation Box") },
        { id: ControlListModel.NAVIGATION_BUTTONS, file: "NavigationWidget.qml", label: HeimdallIcons.dvd_menu, text: qsTr("Navigation") },
        { id: ControlListModel.DVD_MENUS_BUTTON, file: "DvdMenuButton.qml", label: HeimdallIcons.dvd_menu, text: qsTr("DVD menus") },
        { id: ControlListModel.PROGRAM_BUTTON, file: "ProgramButton.qml", label: HeimdallIcons.tv, text: qsTr("Program Button") },
        { id: ControlListModel.TELETEXT_BUTTONS, file: "TeletextButton.qml", label: HeimdallIcons.tvtelx, text: qsTr("Teletext") },
        { id: ControlListModel.RENDERER_BUTTON, file: "RendererButton.qml", label: HeimdallIcons.renderer, text: qsTr("Renderer Button") },
        { id: ControlListModel.ASPECT_RATIO_COMBOBOX, file: "AspectRatioWidget.qml", label: HeimdallIcons.aspect_ratio, text: qsTr("Aspect Ratio") },
        { id: ControlListModel.WIDGET_SPACER, file: "SpacerWidget.qml", label: HeimdallIcons.space, text: qsTr("Spacer") },
        { id: ControlListModel.WIDGET_SPACER_EXTEND, file: "ExpandingSpacerWidget.qml", label: HeimdallIcons.space, text: qsTr("Expanding Spacer") },
        { id: ControlListModel.PLAYER_SWITCH_BUTTON, file: "PlayerSwitchButton.qml", label: HeimdallIcons.fullscreen, text: qsTr("Switch Player") },
        { id: ControlListModel.ARTWORK_INFO, file: "ArtworkInfoWidget.qml", label: HeimdallIcons.info, text: qsTr("Artwork Info") },
        { id: ControlListModel.PLAYBACK_SPEED_BUTTON, file: "PlaybackSpeedButton.qml", label: "1x", text: qsTr("Playback Speed") },
        { id: ControlListModel.HIGH_RESOLUTION_TIME_WIDGET, file: "HighResolutionTimeWidget.qml", label: HeimdallIcons.time, text: qsTr("High Resolution Time") }
    ]

    function control(id) {
        const entry = controlList.find( function(e) { return ( e.id === id ) } )

        if (entry === undefined) {
            console.warn("control delegate id " + id +  " doesn't exist")
            return { source: controlPath + "Fallback.qml" }
        }

        entry.source = controlPath + entry.file

        return entry
    }
}
