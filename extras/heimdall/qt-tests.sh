#!/bin/sh
# Build and run the Qt interface tests inside Heimdall's own CI image.
#
# Usage: extras/heimdall/qt-tests.sh [meson test name...]
#   extras/heimdall/qt-tests.sh                     # every test in the "qt" suite
#   extras/heimdall/qt-tests.sh tst_SliderBar       # one test
#
# The build folder lives in the Docker volume "heimdall-build", so only the
# first run is slow. The volume is mounted as the image user's home so that
# user owns it. Remove it with `docker volume rm heimdall-build`.
set -e

IMAGE=registry.videolan.org/vlc-debian-unstable:20260611225331 # HEIMDALL_DEBIAN_IMAGE in extras/ci/gitlab-ci.yml

SRC=$(cd "$(dirname "$0")/../.." && (pwd -W 2>/dev/null || pwd))

# Qt C++ compiles take ~1 GB each and ninja runs one per CPU (+2), which
# crashed an 8 GB Docker VM with 16 CPUs. Override with HEIMDALL_CPUS=0-15.
CPUS=${HEIMDALL_CPUS:-0-7}

# MSYS_NO_PATHCONV stops Git Bash on Windows from rewriting the container paths.
MSYS_NO_PATHCONV=1 docker run --rm --cpuset-cpus "$CPUS" \
    -v "$SRC:/heimdall" -v heimdall-build:/home/videolan -w /heimdall \
    "$IMAGE" sh -ec '
        [ -f ~/build/build.ninja ] || meson setup ~/build /heimdall -Dqt=enabled
        # Full build: several tests start libheimdall and need the plugins.
        ninja -C ~/build
        if [ $# -eq 0 ]; then set -- --suite qt; fi
        meson test -C ~/build --no-rebuild --print-errorlogs "$@"
    ' sh "$@"
