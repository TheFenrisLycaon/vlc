#!/usr/bin/env sh

SCRIPTDIR="$(cd "$(dirname "$0")" || exit 1; pwd)"
HEIMDALL_SRC_DIR="$(cd "${SCRIPTDIR}/../../../" || exit 1; pwd)"
PROJECTDIR="$(pwd)/xcodeproj/"

export HEIMDALL_SRC_DIR
mkdir -p "${PROJECTDIR}" && cd "${PROJECTDIR}" || exit 1

xcodegen generate --project . --spec "${HEIMDALL_SRC_DIR}/extras/package/apple/xcodegen.yml" --project-root .
