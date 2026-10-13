# Heimdall: rename VLC to Heimdall everywhere (hard fork)

## Context

You asked to rename the project from VLC to Heimdall and change every reference. You picked **everything**: names people see, plus code names, APIs, files and folders. You know this makes a hard fork: VLC updates can't be merged any more, fixes can't easily go back upstream, and outside plugins and Lua extensions written for VLC stop working.

Size: "vlc" appears on about 354,000 lines in 4,050 files (226,000 of those lines are translations in `po/`), and 878 file paths contain it. The main kinds are `libvlc*` (84k), `vlc_*` (70k), `VLC_*` macros (33k) and "VLC media player" (4k).

Done by hand, that can't be reviewed. So the rename is **one script that can be run again**. You review the script and its skip list, not 354,000 lines. The build and the full test suite prove it worked.

**Order:** this comes after the Qt safety-net tests (the branch already in progress). Those tests are the check that the UI still behaves the same after the rename. The rename branch starts from the finished tests branch.

## Name mapping (case is kept)

| Before | After | Example |
|---|---|---|
| `vlc` | `heimdall` | `vlc_player_Start` → `heimdall_player_Start`, `libvlc` → `libheimdall`, `vlcrc` → `heimdallrc` |
| `Vlc` | `Heimdall` | `GstVlcVideoPool` → `GstHeimdallVideoPool` |
| `VLC` inside an all-caps name | `HEIMDALL` | `VLC_TICK_FROM_MS` → `HEIMDALL_TICK_FROM_MS`, `PKG_ENABLE_MODULES_VLC` → `..._HEIMDALL` |
| `VLC` anywhere else (prose, CamelCase) | `Heimdall` | "VLC media player" → "Heimdall media player", `VLCStyle` → `HeimdallStyle`, `QVLCFrame` → `QHeimdallFrame` |
| `videolan` | `lycaria` | `org.videolan.*` → `com.lycaria.*` (asked 2026-10-13) |
| `VideoLAN`, `Videolan`, `VideoLan` | `Lycaria` | "the VideoLAN team" → "the Lycaria team" |
| `VIDEOLAN` | `LYCARIA` | |

Names that follow from this:
- **Programs:** `heimdall`, `cheimdall`, `rheimdall`, `qheimdall`, `nheimdall`.
- **Libraries:** `libheimdall`, `libheimdallcore`. The plugin folder is `lib/heimdall/plugins`.
- **Headers:** `include/heimdall/`, `heimdall_*.h`.
- **Settings:** `~/.config/heimdall/heimdallrc`, and the Qt settings file `heimdall-qt-interface`.
- **Lua scripts:** the `vlc.` API becomes `heimdall.`.
- **Desktop files:** `heimdall.desktop`. The D-Bus name becomes `org.mpris.MediaPlayer2.heimdall`.
- **Apple and Android ids:** `org.videolan.vlc` becomes `com.lycaria.heimdall` (picked 2026-10-13). The script runs this rule before the general `vlc` → `heimdall` rule, so it never produces `org.videolan.heimdall`.

## What is NOT renamed (skip list)

- **Copyright and license text.** The GPL and LGPL require keeping it. That means `COPYING`, `COPYING.LIB`, `AUTHORS`, `THANKS`, and every line with `Copyright` or `VLC authors and VideoLAN`.
- **Upstream history:** `NEWS`, `ChangeLog*`, and the git history itself.
- **Every URL** (`http(s)://…`, `git@…`). Renaming them would break downloads (contribs, prebuilt packages), links, and the XSPF playlist namespace `http://www.videolan.org/vlc/playlist/ns/0/`, which other players read.
- **`VideoLAN` inside copyright lines and URLs.** It's renamed to Lycaria everywhere else (see the mapping). "VLC authors and VideoLAN" in copyright notices stays, and so do `videolan.org` addresses, the `registry.videolan.org` CI images and `code.videolan.org` links, which would break if renamed. "Based on VLC by VideoLAN" in the credits also stays, because it names the original.
- **Third-party code:** patches in `contrib/src/*/*.patch`, which patch other projects. A one-line note per skipped folder goes in the script.
- **Binary files.** Only text files are edited.

## Authorship (asked 2026-10-12: "change the authors to TheFenrisLycaon")

Existing copyright lines stay. The people who wrote the code still own it, and the GPL and LGPL require keeping their notices. Removing them would break the license the fork depends on. Instead:
- **New Heimdall files:** `Copyright (C) <year> TheFenrisLycaon` only.
- **Files the fork changes:** add `Copyright (C) <year> TheFenrisLycaon` as a new line under the existing ones. The rename script adds it to every file it edits.
- **Branding:** About, credits, `--version`, package author and maintainer fields, desktop and AppStream metadata, and app ids show TheFenrisLycaon as Heimdall's author, with "Based on VLC by VideoLAN and the VLC authors".
- **`AUTHORS`:** add a "Heimdall" section at the top naming TheFenrisLycaon. Keep the VLC list below it.

## Steps

1. **Branch:** `heimdall/rename` from the finished `heimdall/qt-safety-net-tests`.
2. **Script `extras/heimdall/rename.py`** (Python standard library only):
   - Lists files with `git ls-files` and drops binaries and the skip list.
   - Edits text with the case-keeping rules above. Inside each line it doesn't touch URL and copyright spans.
   - Renames paths with `git mv`, deepest first, so git keeps each file's history.
   - Prints a leftovers report: every remaining "vlc", grouped by why it was kept.
   - Running it twice changes nothing.
3. **Fix hidden length bugs.** Some code depends on the name being 3 letters long. One example: `qt.cpp:968` has `configDir.chop( 4 ); /* the "\vlc" dir */`. Search for `chop(`, `strncmp`/`memcmp` with a fixed length near "vlc", `sizeof("…vlc…")`, and similar. Fix them by hand in a separate commit.
4. **Fix outside-facing names by hand**, after reviewing each one:
   - HTTP `User-Agent` strings (`lib/core.c`, `modules/access/http.c`)
   - the `vlc://` special links (`INPUT_ITEM_URI_NOP` and others)
   - the update-check URL (`UPDATE_VLC_STATUS_URL`): point it nowhere, because it would offer upstream VLC updates
5. **Commits:** (a) the script, (b) the result of running it, as one commit "Rename VLC to Heimdall (generated by extras/heimdall/rename.py)", (c) the hand fixes.
6. **Vault notes:** in the Heimdall notes, change "VLC" to "Heimdall" where it means our app. Keep it where it means the original VLC, the Juxtopposed video or the Figma file.
7. **Push** the branch to the fork and send you the compare link.

## Verification

1. **Clean build in Docker:** wipe `heimdall-build`, then run `meson setup` and `ninja` in VLC's CI image (Linux). It must build with no new warnings about missing files or headers.
2. **Full test suite:** `meson test` (every suite, not just `qt`). Same pass and fail list as `master` before the rename, with names changed.
3. **The Qt safety-net tests** pass unchanged in behavior.
4. **Smoke run:** run `heimdall --version` and `heimdall -I dummy --play-and-exit` on a test sample in the container. Check the version text says Heimdall, and that settings land in `~/.config/heimdall/`.
5. **Windows cross-build** with the `vlc-debian-llvm-ucrt` CI image (the image name stays, it's an upstream URL), to catch Windows-only code. macOS, iOS and Android code is renamed by the same rules but **can't be built here**. I'll flag that as untested in the summary.
6. **Leftovers report:** every remaining "vlc" is in the skip list.

## Risks you're accepting

- No more upstream merges. Security fixes from VLC have to be copied over by hand.
- Outside VLC plugins, Lua extensions and skins stop working.
- Settings saved by an installed VLC aren't carried over (new config folder).
- Apple and Android code is renamed but not built.
