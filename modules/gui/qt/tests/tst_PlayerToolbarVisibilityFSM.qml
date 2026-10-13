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
import VLC.MainInterface
import VLC.Player

// Safety net: when the player's top and bottom bars show and hide.
TestCase {
    id: root
    name: "PlayerToolbarVisibilityFSM"

    readonly property int hideTimeout: 300

    Component {
        id: fsmComponent
        PlayerToolbarVisibilityFSM { }
    }

    SignalSpy {
        id: forceUnlockSpy
        signalName: "forceUnlock"
    }

    function init() {
        MainCtx.mouseHideTimeout = root.hideTimeout
        MainCtx.hasEmbededVideo = true
        Player.isInteractive = false
        forceUnlockSpy.clear()
    }

    function makeFsm() {
        const fsm = createTemporaryObject(fsmComponent, root)
        verify(fsm)
        forceUnlockSpy.target = fsm
        return fsm
    }

    function test_videoShowsBarsThenHidesThem() {
        const fsm = makeFsm()
        verify(fsm.isVisible)
        tryVerify(() => !fsm.isVisible, hideTimeout * 5)
        compare(forceUnlockSpy.count, 1)
    }

    function test_mouseMoveKeepsBarsShown() {
        const fsm = makeFsm()
        wait(hideTimeout * 2 / 3)
        fsm.mouseMove()
        wait(hideTimeout * 2 / 3)
        verify(fsm.isVisible) // past the first timeout, but the move restarted it
        tryVerify(() => !fsm.isVisible, hideTimeout * 5)
    }

    function test_mouseMoveShowsHiddenBars() {
        const fsm = makeFsm()
        tryVerify(() => !fsm.isVisible, hideTimeout * 5)
        fsm.mouseMove()
        verify(fsm.isVisible)
        compare(fsm.timeoutDuration, hideTimeout)
    }

    function test_keyboardShowsBarsForTwiceAsLong() {
        const fsm = makeFsm()
        tryVerify(() => !fsm.isVisible, hideTimeout * 5)
        fsm.keyboardMove()
        verify(fsm.isVisible)
        compare(fsm.timeoutDuration, hideTimeout * 2)

        fsm.mouseMove() // the mouse brings the normal timeout back
        compare(fsm.timeoutDuration, hideTimeout)
    }

    function test_askShowToggles() {
        const fsm = makeFsm()
        fsm.askShow()
        verify(!fsm.isVisible)
        fsm.askShow()
        verify(fsm.isVisible)
    }

    function test_locksKeepBarsShownUntilAllAreReleased() {
        const fsm = makeFsm()
        fsm.lock()
        fsm.lock()
        compare(fsm.lockCount, 2)

        wait(hideTimeout * 2)
        verify(fsm.isVisible)

        fsm.unlock()
        compare(fsm.lockCount, 1)
        wait(hideTimeout * 2)
        verify(fsm.isVisible)

        fsm.unlock()
        compare(fsm.lockCount, 0)
        tryVerify(() => !fsm.isVisible, hideTimeout * 5)
    }

    function test_audioNeverHidesBars() {
        MainCtx.hasEmbededVideo = false
        const fsm = makeFsm()
        wait(hideTimeout * 2)
        verify(fsm.isVisible)
        fsm.askShow()
        verify(fsm.isVisible)
    }

    function test_videoStartingLaterStartsTheTimer() {
        MainCtx.hasEmbededVideo = false
        const fsm = makeFsm()
        MainCtx.hasEmbededVideo = true
        fsm.updateVideoEmbed()
        tryVerify(() => !fsm.isVisible, hideTimeout * 5)
    }

    // Interactive media (DVD menus and the like) takes the mouse and keys.
    function test_interactiveMediaStartsHiddenAndIgnoresInput() {
        Player.isInteractive = true
        const fsm = makeFsm()
        verify(!fsm.isVisible)

        fsm.mouseMove()
        fsm.keyboardMove()
        verify(!fsm.isVisible)

        fsm.askShow()
        verify(fsm.isVisible)
        wait(hideTimeout * 2)
        verify(fsm.isVisible) // no auto-hide while interactive
    }
}
