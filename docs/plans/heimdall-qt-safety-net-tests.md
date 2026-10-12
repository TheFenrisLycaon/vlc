# Heimdall: safety-net tests for VLC's Qt player UI

## Context

Heimdall will rebuild VLC's player screens (`modules/gui/qt/`). Before we change anything, we want tests that pin down how the player works **today**, so we notice when the redesign breaks behavior by accident. You chose: **safety net only** (today's behavior, no Heimdall specs yet) and **run in Docker with VLC's own CI image**.

Today the Qt UI has 6 small C++ tests and 1 QML test (`tests/tst_FSM.qml`). Nothing covers the control bar layouts, the theme, the bar auto-hide rules, the seek bar, the play button or the volume.

**Rule for every test:** check *behavior*, not *looks*. Heimdall will change colors, sizes and positions on purpose, so pixel or layout tests would just fail by design. Each test must still pass after a correct redesign.

This plan will be copied to `docs/plans/heimdall-qt-safety-net-tests.md` in the repo when work starts.

## Step 1: Docker test runner (one script)

New file `extras/heimdall/qt-tests.sh`. It:
- Runs `registry.videolan.org/vlc-debian-unstable:20260611225331` (the `VLC_DEBIAN_IMAGE` from `extras/ci/gitlab-ci.yml`), with the repo mounted and the build folder in a named Docker volume (`heimdall-build`), so rebuilds are fast and Windows file sharing doesn't slow the build.
- Inside the container, does what the `debian-meson` CI job does: builds `extras/tools`, runs `meson setup build -Dqt=enabled`, builds only what the tests need, then runs `meson test -C build --suite qt --print-errorlogs`.
- Takes an optional test name: `qt-tests.sh tst_PlayerToolbarVisibilityFSM`.

First check: confirm the image has Qt 6 with the Test and QuickTest modules. If not, add them with `apt-get` inside the script and note it.

Needs Docker Desktop running. The image is several GB, and the first build takes a while.

## Step 2: C++ tests for the layout and theme logic (QTest)

Same pattern as `tests/test_navigation_model.cpp`: a QTest class, `vlc_stub_modules.cpp` when core symbols are needed, registered in `meson.build` (`vlc_tests += {...}`, suite `qt`) **and** `Makefile.am` (`check_PROGRAMS` / `TESTS`), to stay in step with upstream.

1. **`tests/test_control_list_model.cpp`** for `player/control_list_model.*`:
   - Pin every `ControlType` number (play = 0, `SPLITTER` = 0x20, `WIDGET_SPACER` = 0x40, …). Saved layouts store these numbers, so a renumbering must fail this test.
   - Insert, move and remove keep the order, and the model passes `QAbstractItemModelTester`.
2. **`tests/test_controlbar_profile_model.cpp`** for `dialogs/toolbar/controlbar_profile_model.*` and `controlbar_profile.*`:
   - The built-in profiles exist with their exact names and layouts: Empty, Default, Minimalist, One-liner, Simplest, Classic.
   - `save()` then `reload()` with a temp `QSettings` file gives back the same profiles and the same selected profile.
   - Reading a hand-written `$0|1,0,2|…|…` string works. A broken string is skipped without a crash.
   - `newProfile`, `cloneSelectedProfile` and `deleteSelectedProfile` behave as today.
3. **`tests/test_player_controlbar_model.cpp`** for `player/player_controlbar_model.*`: the left, center and right lists, and pinned `PlayerIdentifier` values (Videoplayer = 0, Audioplayer, Miniplayer).
4. **`tests/test_color_scheme_model.cpp`** for `util/color_scheme_model.*`: the scheme list (System, Day, Night) and how the chosen scheme is set and reported.
5. **`tests/test_system_palette.cpp`** for `style/systempalette.*`: for the Day and Night palettes, check that every color the screens ask for exists (no magenta fallback), and record today's key colors (accent, window background, text) so changes are deliberate. `SystemPalette` reads `MainCtx` for the accent option. If that can't be stubbed cleanly, drop this test and say so in the PR, rather than adding big fakes.

## Step 3: QML tests for player behavior (QtQuickTest)

Use the existing runner `tests/qml_test.cpp` and add each test file to the `qml_tests` dict in `meson.build` (and to `Makefile.am`).

**Runner change:** in `Setup::qmlEngineAvailable`, register small QML fake singletons for `MainCtx` (in `VLC.MainInterface`), `Player` (in `VLC.Player`) and `MainPlaylistController` (in `VLC.Playlist`). The fakes have the properties the tested files read (`mouseHideTimeout`, `hasEmbededVideo`, `playqueuePanel`, `isInteractive`, `position`, `length`, `time`, `playingState`, `volume`, `maxVolume`…), and they record calls like `togglePlayPause()`. Also load the asset bundles the tested files live in (`player_assets`, `style_assets`, `widgets_assets`, `playlist_assets`), next to the current `util_assets`. The fakes live in `tests/fakes/`.

6. **`tests/tst_PlayerToolbarVisibilityFSM.qml`**:
   - Bars show on mouse move.
   - They hide after `MainCtx.mouseHideTimeout`.
   - The longer timeout (×2) applies where the code uses it.
   - A lock (`lockCount`) keeps them shown.
   - Audio mode keeps them shown.
   - `Player.isInteractive` keeps them shown.
   - Fake time comes from `TestCase.wait`, with a tiny timeout set on the fake `MainCtx`.
7. **`tests/tst_PlayerPlaylistVisibilityFSM.qml`**: docked vs floating, the toggle flips `MainCtx.playqueuePanel.visible`, and the start state follows today's rules.
8. **`tests/tst_SliderBar.qml`** (the seek bar):
   - Clicking at 25% of the width sets `Player.position` to about 0.25.
   - Positions are clamped to 0 to 1.
   - The left and right keys call `jumpBwd` and `jumpFwd`.
   - The hover tooltip shows the time for the hovered spot.
   - Snapping to chapters is tested only if the fake chapters stay small.
9. **`tests/tst_PlayButton.qml`**: a click calls `MainPlaylistController.togglePlayPause()` once, and the shown state follows `Player.playingState`.
10. **`tests/tst_VolumeWidget.qml`**: dragging, the wheel and the keys change `Player.volume` within 0 to `MainCtx.maxVolume`, and mute toggles.

Order: 6 and 7 first (they only need `VLC.Util`, `VLC.MainInterface` and `VLC.Player`), then 8 to 10. If a component pulls in too much of the app to load in a test, don't fake the whole app. Skip that test and list it in the PR with the reason.

## Not in this round (and why)

- **Screenshot tests.** The redesign changes the look on purpose, so they would fail by design.
- **`PlayerController` against a real core player.** It needs a full `qt_intf_t`. Too big for a safety net. The core's own player tests in `test/src/player/` already cover the core side.
- **Menus and the old Qt Widgets dialogs.** Heimdall replaces them. Testing them now is wasted work.
- **Heimdall spec tests.** You chose safety net only.

## Files

- New: `extras/heimdall/qt-tests.sh`, `modules/gui/qt/tests/test_*.cpp` (5), `modules/gui/qt/tests/tst_*.qml` (5), `modules/gui/qt/tests/fakes/*.qml`.
- Changed: `modules/gui/qt/tests/qml_test.cpp` (fake singletons, extra asset bundles), `modules/gui/qt/meson.build` (`vlc_tests`, `qml_tests`), `modules/gui/qt/Makefile.am` (`check_PROGRAMS`, `TESTS`, QML test list).
- Doc: `docs/plans/heimdall-qt-safety-net-tests.md` (this plan).

## Verification

1. Start Docker Desktop. Run `extras/heimdall/qt-tests.sh`. The 7 old tests and every new test pass.
2. Check that each test can fail. Make one temporary wrong change per area and confirm the right test goes red, then revert it:
   - swap two `ControlType` numbers
   - change a default profile layout
   - set the hide timeout to 0 in `PlayerToolbarVisibilityFSM.qml`
   - change the seek click math in `SliderBar.qml`
   - remove the `togglePlayPause()` call in `PlayButton.qml`
3. Run it twice to confirm the second build is quick (the cached volume works).
4. Report the pass and fail list and any skipped tests with reasons.
