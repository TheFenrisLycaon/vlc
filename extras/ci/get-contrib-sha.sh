#!/usr/bin/env bash
# Copyright (C) Marvin Scholz
# Copyright (C) 2026 TheFenrisLycaon
#
# License: see COPYING
#
# Get latest SHA that changed contribs (needed for CI)
set -e

# Array of paths that should trigger contrib rebuilds
HEIMDALL_CONTRIB_REBUILD_PATHS=("extras/tools" "contrib" "extras/ci")
# Revision from which to start look for changes (backwards in time)
HEIMDALL_START_REVISION="HEAD"

# Print error message and terminate script with status 1
# Arguments:
#   Message to print
abort_err()
{
    echo "ERROR: $1" >&2
    exit 1
}

command -v "git" >/dev/null 2>&1 || abort_err "Git was not found!"

# Heimdall source root directory
HEIMDALL_SRC_ROOT_DIR=$(git rev-parse --show-toplevel)

[ -n "${HEIMDALL_SRC_ROOT_DIR}" ] || abort_err "This script must be run in the Heimdall Git repo and git must be available"
[ -f "${HEIMDALL_SRC_ROOT_DIR}/src/libheimdall.h" ] || abort_err "This script must be run in the Heimdall Git repository"

case $1 in
    win32*|win64*|uwp*)
        HEIMDALL_CONTRIB_REBUILD_PATHS+=( "extras/package/win32" )
        ;;
    ios*|tvos*)
        HEIMDALL_CONTRIB_REBUILD_PATHS+=( "extras/package/apple" )
        ;;
    macos*)
        HEIMDALL_CONTRIB_REBUILD_PATHS+=( "extras/package/macosx" )
        ;;
    raspbian*)
        HEIMDALL_CONTRIB_REBUILD_PATHS+=( "extras/package/raspberry" )
        ;;
    snap*)
        HEIMDALL_CONTRIB_REBUILD_PATHS+=( "extras/package/snap" )
        ;;
    wasm*)
        HEIMDALL_CONTRIB_REBUILD_PATHS+=( "extras/package/wasm-emscripten" )
        ;;
    debian*|android*)
        ;;
    *)
        HEIMDALL_CONTRIB_REBUILD_PATHS+=( "extras/package" )
esac

HEIMDALL_LAST_CONTRIB_SHA=$(
    cd "${HEIMDALL_SRC_ROOT_DIR}" && git rev-list -1 "${HEIMDALL_START_REVISION}" -- "${HEIMDALL_CONTRIB_REBUILD_PATHS[@]}"
)

[ -n "${HEIMDALL_LAST_CONTRIB_SHA}" ] || abort_err "Failed to determine last contrib SHA using Git!"

echo "${HEIMDALL_LAST_CONTRIB_SHA}"
