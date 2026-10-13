/*****************************************************************************
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
import QtTest
import Heimdall.MainInterface
import Heimdall.Player

// Safety net: when the play queue panel shows in the player.
TestCase {
    id: root
    name: "PlayerPlaylistVisibilityFSM"

    Component {
        id: fsmComponent
        PlayerPlaylistVisibilityFSM { }
    }

    function init() {
        MainCtx.playqueuePanel.docked = true
        MainCtx.playqueuePanel.visible = false
        MainCtx.hasEmbededVideo = false
        MainCtx.initialEffectiveMainInterfaceMode = MainCtx.MAININTERFACE_MODE_MAINDISPLAY
    }

    function makeFsm() {
        const fsm = createTemporaryObject(fsmComponent, root)
        verify(fsm)
        return fsm
    }

    function test_floatingButtonFlipsTheWindow() {
        MainCtx.playqueuePanel.docked = false
        const fsm = makeFsm()
        verify(!fsm.isPlaylistVisible)

        fsm.togglePlaylistVisibility()
        verify(MainCtx.playqueuePanel.visible)
        fsm.togglePlaylistVisibility()
        verify(!MainCtx.playqueuePanel.visible)
        verify(!fsm.isPlaylistVisible) // a floating queue is never docked-visible
    }

    function test_dockedStartsLikeThePanelWas() {
        const hidden = makeFsm()
        verify(!hidden.isPlaylistVisible)

        MainCtx.playqueuePanel.visible = true
        const shown = makeFsm()
        verify(shown.isPlaylistVisible)
    }

    function test_dockedStartsHiddenOverVideo() {
        MainCtx.playqueuePanel.visible = true
        MainCtx.hasEmbededVideo = true
        const fsm = makeFsm()
        verify(!fsm.isPlaylistVisible)
        verify(MainCtx.playqueuePanel.visible) // remembered for when the video ends
    }

    function test_dockedStartsHiddenWhenNotComingFromTheLibrary() {
        MainCtx.playqueuePanel.visible = true
        MainCtx.initialEffectiveMainInterfaceMode = MainCtx.MAININTERFACE_MODE_PLAYER
        const fsm = makeFsm()
        verify(!fsm.isPlaylistVisible)
    }

    function test_dockedButtonShowsAndHides() {
        const fsm = makeFsm()
        fsm.togglePlaylistVisibility()
        verify(fsm.isPlaylistVisible)
        verify(MainCtx.playqueuePanel.visible)

        fsm.togglePlaylistVisibility()
        verify(!fsm.isPlaylistVisible)
        verify(!MainCtx.playqueuePanel.visible)
    }

    function test_videoHidesThePanelAndBringsItBack() {
        MainCtx.playqueuePanel.visible = true
        const fsm = makeFsm()
        verify(fsm.isPlaylistVisible)

        MainCtx.hasEmbededVideo = true
        fsm.updateVideoEmbed()
        verify(!fsm.isPlaylistVisible)

        MainCtx.hasEmbededVideo = false
        fsm.updateVideoEmbed()
        verify(fsm.isPlaylistVisible)
    }

    function test_buttonOverVideoShowsThePanel() {
        MainCtx.hasEmbededVideo = true
        const fsm = makeFsm()
        verify(!fsm.isPlaylistVisible)
        fsm.togglePlaylistVisibility()
        verify(fsm.isPlaylistVisible)
        verify(MainCtx.playqueuePanel.visible)
    }

    function test_followsOutsideChanges() {
        const fsm = makeFsm()
        MainCtx.playqueuePanel.visible = true
        fsm.updatePlaylistVisible()
        verify(fsm.isPlaylistVisible)

        MainCtx.playqueuePanel.visible = false
        fsm.updatePlaylistVisible()
        verify(!fsm.isPlaylistVisible)
    }

    function test_undockingAndDocking() {
        MainCtx.playqueuePanel.visible = true
        const fsm = makeFsm()
        verify(fsm.isPlaylistVisible)

        MainCtx.playqueuePanel.docked = false
        fsm.updatePlaylistDocked()
        verify(!fsm.isPlaylistVisible)

        MainCtx.playqueuePanel.docked = true
        fsm.updatePlaylistDocked()
        verify(fsm.isPlaylistVisible) // the panel was still marked visible
    }
}
