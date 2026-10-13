#!/bin/bash
# Copyright (C) 2012-2017 VLC authors and VideoLAN
# Copyright (C) 2012-2014 Felix Paul Kühne <fkuehne at videolan dot org>
# Copyright (C) 2026 TheFenrisLycaon
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.

set -e
# set -o xtrace

info()
{
    green='\x1B[1;32m'
    normal='\x1B[0m'
    echo -e "[${green}codesign${normal}] $1"
}

usage()
{
cat << EOF
usage: $0 [options]

Sign Heimdall.app in the current directory

OPTIONS:
   -h            Show this help
   -i            Identity to use
   -g            Developer ID certificate mode (validates with Gatekeeper)
   -r            Enable runtime hardening
EOF

}

while getopts "hi:gr" OPTION
do
     case $OPTION in
         h)
             usage
             exit 1
         ;;
         i)
             IDENTITY=$OPTARG
         ;;
         g)
             GK="yes"
         ;;
         r)
             RUNTIME="yes"
         ;;
         *)
             usage
             exit 1
         ;;
     esac
done
shift $(($OPTIND - 1))

if [ "x$1" != "x" ]; then
    usage
    exit 1
fi

HEIMDALLCACHEGEN=""
if [ -e "./bin/heimdall-cache-gen" ]; then
    HEIMDALLCACHEGEN="./bin/heimdall-cache-gen"
fi
if [ -e "./heimdall-cache-gen" ]; then
    HEIMDALLCACHEGEN="./heimdall-cache-gen"
fi

$HEIMDALLCACHEGEN --help 1>/dev/null 2>&1 && returncode=$? || returncode=$?
if [ $returncode -ne 0 ]; then
    info "WARN: Cannot execute heimdall-cache-gen with path '$HEIMDALLCACHEGEN' (wrong arch?)"
    HEIMDALLCACHEGEN=""
fi

if [ -z "$HEIMDALLCACHEGEN" ]; then
    info "WARN: Cannot find heimdall-cache-gen, cache will be removed for signing"
fi

SCRIPTDIR=$(dirname "$0")
if [ ! -z "$RUNTIME" ]; then
    RUNTIME_FLAGS="--options runtime --entitlements $SCRIPTDIR/heimdall-hardening.entitlements"
fi

# Call with $1 = file or folder
sign()
{
    # info "Signing file $1 with identifier $IDENTIFIER"

    codesign --force --verbose $RUNTIME_FLAGS -s "$IDENTITY" "$1"
}


info "Cleaning frameworks"
find Heimdall.app/Contents/Frameworks -type f -name ".DS_Store" -exec rm '{}' \;
find Heimdall.app/Contents/Frameworks -type f -name "*.textile" -exec rm '{}' \;
find Heimdall.app/Contents/Frameworks -type f -name "*.txt" -exec rm '{}' \;

info "Signing frameworks"

sign "Heimdall.app/Contents/Frameworks/Sparkle.framework/Versions/B/Autoupdate"
sign "Heimdall.app/Contents/Frameworks/Sparkle.framework/Versions/B/Updater.app"
sign "Heimdall.app/Contents/Frameworks/Sparkle.framework/"

if [ -e "Heimdall.app/Contents/Frameworks/Breakpad.framework" ]; then
    sign "Heimdall.app/Contents/Frameworks/Breakpad.framework/Resources/breakpadUtilities.dylib"
    sign "Heimdall.app/Contents/Frameworks/Breakpad.framework/Resources/Inspector"
    sign "Heimdall.app/Contents/Frameworks/Breakpad.framework/Resources/crash_report_sender.app"
    sign "Heimdall.app/Contents/Frameworks/Breakpad.framework/Versions/A"
fi

info "Signing the modules"

for i in $(find Heimdall.app/Contents/Frameworks/plugins -type f \( -name "*.dylib" -o -name "*.jar" \)  -exec echo {} \;)
do
    sign "$i"
done

if [ ! -z "$HEIMDALLCACHEGEN" ]; then
    $HEIMDALLCACHEGEN Heimdall.app/Contents/Frameworks/plugins
    sign "Heimdall.app/Contents/Frameworks/plugins/plugins.dat"
else
    rm "Heimdall.app/Contents/Frameworks/plugins/plugins.dat" || true
fi

info "Signing the libraries"

for i in $(find Heimdall.app/Contents/Frameworks -type f -name "*.dylib" -d 1 -exec echo {} \;)
do
    sign "$i"
done

for i in $(find Heimdall.app/Contents/Frameworks/lua -type f -exec echo {} \;)
do
    sign "$i"
done

info "Signing the executables"
sign "Heimdall.app/Contents/MacOS/heimdall-preparser"
sign "Heimdall.app"


info "all items signed, validating..."

info "Validating frameworks"
if [ -e "Heimdall.app/Contents/Frameworks/Breakpad.framework" ]; then
    codesign --verify -vv Heimdall.app/Contents/Frameworks/Breakpad.framework
fi

codesign --verify -vv Heimdall.app/Contents/Frameworks/Sparkle.framework

info "Validating autoupdate app"
codesign --verify -vv Heimdall.app/Contents/Frameworks/Sparkle.framework/Versions/Current/Updater.app

info "Validating complete bundle"
codesign --verify --deep --strict --verbose=4 Heimdall.app

if [ ! -z "$GK" ]; then
    spctl -a -t exec -vv Heimdall.app
fi


info "Validation complete"
