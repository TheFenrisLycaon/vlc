#!/usr/bin/env bash
# Copyright (C) Marvin Scholz
# Copyright (C) 2026 TheFenrisLycaon
#
# Script to help build Heimdall or libHeimdall for Apple OSes
# Supported OSes: iOS, tvOS, macOS, xrOS, watchOS
#
# Currently this script builds a full static library,
# with all modules and contribs combined into one .a
# file.
#
# Settings that need to be changed from time to time,
# like the target OS versions or contrib/configure options
# can be found in the build.conf file in the same folder.

# TODO:
# - Add packaging support and right options to build a macOS app
# - Support shared build where you get a dylib for libheimdall,
#   libheimdallcore and dylibs for the individual modules.
# - Support a mixed shared build where you only have a
#   libheimdall.dylib that includes libheimdallcore and the modules
#   statically.
# Proposed interface for this:
#   --buildmode=<fullstatic, pseudoshared, shared>
#        fullstatic: One resulting static library with libheimdall and modules
#        pseudoshared: Shared library with all modules statically linked
#        shared: Shared libraries and modules

# Dir of this script
readonly HEIMDALL_SCRIPT_DIR="$(cd "${BASH_SOURCE%/*}"; pwd)"

# Include heimdall env script
. "$HEIMDALL_SCRIPT_DIR/../macosx/env.build.sh" "none"

##########################################################
#                    Global variables                    #
##########################################################

# Name of this script
readonly HEIMDALL_SCRIPT_NAME=$(basename "$0")
# Heimdall source dir root
readonly HEIMDALL_SRC_DIR=$(heimdallGetRootDir)
# Heimdall build dir
readonly HEIMDALL_BUILD_DIR=$(pwd)
# Whether verbose output is enabled or not
HEIMDALL_SCRIPT_VERBOSE=0
# Path to build configuration file
: "${HEIMDALL_BUILD_CONF_PATH:="$HEIMDALL_SCRIPT_DIR/build.conf"}"
# Architecture of the host (OS that the result will run on)
HEIMDALL_HOST_ARCH="x86_64"
# Host platform information
HEIMDALL_HOST_PLATFORM=
HEIMDALL_HOST_TRIPLET=
HEIMDALL_BUILD_TRIPLET=
# Set to "1" when building for simulator
HEIMDALL_HOST_PLATFORM_SIMULATOR=
# The host OS name (without the simulator suffix)
# as used by the Apple tools for flags like the
# min version or clangs target option
HEIMDALL_HOST_OS=
# Lowest OS version (iOS, tvOS or macOS) to target
# Do NOT edit this to set a specific target, instead
# edit the HEIMDALL_DEPLOYMENT_TARGET_* variables above.
HEIMDALL_DEPLOYMENT_TARGET=
# Flags for linker and compiler that set the min target OS
# Those will be set by the set_deployment_target function
HEIMDALL_DEPLOYMENT_TARGET_LDFLAG=
HEIMDALL_DEPLOYMENT_TARGET_CFLAG=
# SDK name (optionally with version) to build with
# We default to macOS builds, so this is set to macosx
HEIMDALL_APPLE_SDK_NAME="macosx"
# SDK path
# Set in the validate_sdk_name function
HEIMDALL_APPLE_SDK_PATH=
# SDK version
# Set in the validate_sdk_name function
HEIMDALL_APPLE_SDK_VERSION=
# XCode version used to build
HEIMDALL_APPLE_XCODE_VERSION=
# Indicated if prebuilt contribs package
# should be created
HEIMDALL_MAKE_PREBUILT_CONTRIBS=0
# Indicates that prebuit contribs should be
# used instead of building the contribs from source
HEIMDALL_USE_PREBUILT_CONTRIBS=0
# User-provided URL from where to fetch contribs, empty
# for the default chosen by contrib system
HEIMDALL_PREBUILT_CONTRIBS_URL=${HEIMDALL_PREBUILT_CONTRIBS_URL:-""}
# The number of cores to compile on
CORE_COUNT=$(sysctl -n machdep.cpu.core_count || nproc || echo 0)
let HEIMDALL_USE_NUMBER_OF_CORES=$CORE_COUNT+1
HEIMDALL_REQUESTED_CORE_COUNT=0
# whether to disable debug mode (the default) or not
HEIMDALL_DISABLE_DEBUG=0
# whether to compile with bitcode or not
HEIMDALL_USE_BITCODE=0
HEIMDALL_BITCODE_FLAG="-fembed-bitcode"
# whether to build static or dynamic plugins
HEIMDALL_BUILD_DYNAMIC=0
# Add extra checks when compiling
HEIMDALL_BUILD_EXTRA_CHECKS=0
# --with-contrib option forwarded to configure
HEIMDALL_WITH_CONTRIB=

# Tools to be used
HEIMDALL_HOST_CC="$(xcrun --find clang)"
HEIMDALL_HOST_CPP="$(xcrun --find clang) -E"
HEIMDALL_HOST_CXX="$(xcrun --find clang++)"
HEIMDALL_HOST_OBJC="$(xcrun --find clang)"
HEIMDALL_HOST_OBJCXX="$(xcrun --find clang++)"
HEIMDALL_HOST_LD="$(xcrun --find ld)"
HEIMDALL_HOST_AR="$(xcrun --find ar)"
HEIMDALL_HOST_STRIP="$(xcrun --find strip)"
HEIMDALL_HOST_RANLIB="$(xcrun --find ranlib)"
HEIMDALL_HOST_NM="$(xcrun --find nm)"

##########################################################
#                    Helper functions                    #
##########################################################

# Print command line usage
usage()
{
    echo "Usage: $HEIMDALL_SCRIPT_NAME [options]"
    echo " --arch=ARCH      Architecture to build for"
    echo "                   (i386|x86_64|armv7|arm64)"
    echo " --sdk=SDK        Name of the SDK to build with (see 'xcodebuild -showsdks')"
    echo " --config=FILE    Path to build configuration file"
    echo "                   (default: ./build.conf)"
    echo " --enable-bitcode        Enable bitcode for compilation, same as with =full"
    echo " --enable-bitcode=marker Enable bitcode marker for compilation"
    echo " --disable-debug  Disable libheimdall debug mode (for release)"
    echo " --verbose        Print verbose output and disable multi-core use"
    echo " --help           Print this help"
    echo ""
    echo "Advanced options:"
    echo " --package-contribs        Create a prebuilt contrib package"
    echo " --with-prebuilt-contribs  Use prebuilt contribs instead of building"
    echo "                           them from source"
    echo " --with-contrib[=DIR]      Use contribs from DIR (default: build and auto-detect)"
    echo " --without-contrib         Disable contribs entirely (--with-contrib=no)"
    echo " --enable-shared           Build dynamic libraries and plugins"
    echo " --enable-extra-checks     Add extra checks when compiling"
    echo "Environment variables:"
    echo " HEIMDALL_PREBUILT_CONTRIBS_URL  URL to fetch the prebuilt contrib archive"
    echo "                            from when --with-prebuilt-contribs is used"
}

# Print error message and terminate script with status 1
# Arguments:
#   Message to print
abort_err()
{
    echo "ERROR: $1" >&2
    exit 1
}

# Print message if verbose, else silent
# Globals:
#   HEIMDALL_SCRIPT_VERBOSE
# Arguments:
#   Message to print
verbose_msg()
{
    if [ "$HEIMDALL_SCRIPT_VERBOSE" -gt "0" ]; then
        echo "$1"
    fi
}

# Check if tool exists, if not error out
# Arguments:
#   Tool name to check for
check_tool()
{
    command -v "$1" >/dev/null 2>&1 || {
        abort_err "This script requires '$1' but it was not found"
    }
}

# Validate and set the build config file path
# Globals:
#   HEIMDALL_BUILD_CONF_PATH
# Arguments:
#   Optional config file path
validate_build_conf()
{
    local conf_path="$1"

    # Expand relative paths to absolute paths
    conf_path="$(cd "$(dirname "$conf_path")" 2>/dev/null && pwd)/$(basename "$conf_path")"

    if [ ! -f "$conf_path" ] || [ ! -r "$conf_path" ]; then
        abort_err "Build config file not found or not readable: $conf_path"
    fi

    HEIMDALL_BUILD_CONF_PATH="$conf_path"
    verbose_msg "Using build config: $conf_path"
}

# Set the HEIMDALL_DEPLOYMENT_TARGET* flag options correctly
# Globals:
#   HEIMDALL_DEPLOYMENT_TARGET
#   HEIMDALL_DEPLOYMENT_TARGET_LDFLAG
#   HEIMDALL_DEPLOYMENT_TARGET_CFLAG
# Arguments:
#   Deployment target version
set_deployment_target()
{
    HEIMDALL_DEPLOYMENT_TARGET="$1"
    HEIMDALL_DEPLOYMENT_TARGET_CFLAG="-m$HEIMDALL_HOST_OS"

    if [ -n "$HEIMDALL_HOST_PLATFORM_SIMULATOR" ]; then
        HEIMDALL_DEPLOYMENT_TARGET_CFLAG="${HEIMDALL_DEPLOYMENT_TARGET_CFLAG}-simulator"
    fi

    # TODO: To support setting the min version for xrOS we need to use a target triplet
    if [ "$HEIMDALL_HOST_OS" != "xros" ]; then
        HEIMDALL_DEPLOYMENT_TARGET_CFLAG="${HEIMDALL_DEPLOYMENT_TARGET_CFLAG}-version-min=${HEIMDALL_DEPLOYMENT_TARGET}"
        HEIMDALL_DEPLOYMENT_TARGET_LDFLAG="${HEIMDALL_DEPLOYMENT_TARGET_CFLAG}"
    else
        HEIMDALL_DEPLOYMENT_TARGET_CFLAG="--target=${HEIMDALL_HOST_ARCH}-apple-xros${HEIMDALL_DEPLOYMENT_TARGET}"
        if [ -n "$HEIMDALL_HOST_PLATFORM_SIMULATOR" ]; then
            HEIMDALL_DEPLOYMENT_TARGET_CFLAG="${HEIMDALL_DEPLOYMENT_TARGET_CFLAG}-simulator"
        fi
        HEIMDALL_DEPLOYMENT_TARGET_LDFLAG=""
    fi
}

# Validates the architecture and sets HEIMDALL_HOST_ARCH
# This MUST set the arch to what the compiler accepts
# for the -arch argument!
# Globals:
#   HEIMDALL_HOST_ARCH
# Arguments:
#   Architecture string
validate_architecture()
{
    case "$1" in
    i386|x86_64|armv7|arm64|armv7k|arm64_32)
        HEIMDALL_HOST_ARCH="$1"
        ;;
    aarch64)
        HEIMDALL_HOST_ARCH="arm64"
        ;;
    *)
        abort_err "Invalid architecture '$1'"
        ;;
    esac
}

# Set the HEIMDALL_HOST_TRIPLET based on the architecture
# by querying the compiler for it, as the HEIMDALL_HOST_ARCH
# can not be used in the triplet directly, like in
# case of arm64.
# Globals:
#   CC
#   HEIMDALL_HOST_TRIPLET
# Arguments:
#   Architecture string
set_host_triplet()
{
    local triplet_arch=$(${HEIMDALL_HOST_CC:-cc} -arch "$1" -dumpmachine | cut -d- -f 1)
    # We can not directly use the compiler value here as when building for
    # x86_64 iOS Simulator the triplet will match the build machine triplet
    # exactly, which will cause autoconf to assume we are not cross-compiling.
    # Therefore we construct a triplet here without a version number, which
    # will not match the autoconf "guessed" host machine triplet.
    HEIMDALL_HOST_TRIPLET="${triplet_arch}-apple-darwin"
    HEIMDALL_HOST_TRIPLET="${HEIMDALL_HOST_TRIPLET/arm64-/aarch64-}"
}

# Set the HEIMDALL_BUILD_TRIPLET based on the architecture
# that we run on.
# Globals:
#   HEIMDALL_BUILD_TRIPLET
# Arguments:
#   None
set_build_triplet()
{
    if [ "$(uname -s)" = "Darwin" ]; then
        local build_arch="$(uname -m | cut -d. -f1)"
        HEIMDALL_BUILD_TRIPLET="$(${HEIMDALL_HOST_CC} -arch "${build_arch}" -dumpmachine)"
        HEIMDALL_BUILD_TRIPLET="${HEIMDALL_BUILD_TRIPLET/arm64/aarch64}"
    fi
}

# Take SDK name, verify it exists and populate
# HEIMDALL_HOST_*, HEIMDALL_APPLE_SDK_PATH variables based
# on the SDK and calls the set_deployment_target
# function with the rigth target version
# Globals:
#   HEIMDALL_DEPLOYMENT_TARGET_IOS
#   HEIMDALL_DEPLOYMENT_TARGET_TVOS
#   HEIMDALL_DEPLOYMENT_TARGET_MACOSX
# Arguments:
#   SDK name
validate_sdk_name()
{
    xcrun --sdk "$1" --show-sdk-path >/dev/null 2>&1 || {
        abort_err "Failed to find SDK '$1'"
    }

    HEIMDALL_APPLE_SDK_PATH="$(xcrun --sdk "$1" --show-sdk-path)"
    HEIMDALL_APPLE_SDK_VERSION="$(xcrun --sdk "$1" --show-sdk-version)"
    HEIMDALL_APPLE_XCODE_VERSION="$(xcodebuild -version)"
    if [ ! -d "$HEIMDALL_APPLE_SDK_PATH" ]; then
        abort_err "SDK at '$HEIMDALL_APPLE_SDK_PATH' does not exist"
    fi

    case "$1" in
        iphoneos*)
            HEIMDALL_HOST_PLATFORM="iOS"
            HEIMDALL_HOST_OS="ios"
            set_deployment_target "$HEIMDALL_DEPLOYMENT_TARGET_IOS"
            ;;
        iphonesimulator*)
            HEIMDALL_HOST_PLATFORM="iOS-Simulator"
            HEIMDALL_HOST_PLATFORM_SIMULATOR="yes"
            HEIMDALL_HOST_OS="ios"
            set_deployment_target "$HEIMDALL_DEPLOYMENT_TARGET_IOS_SIMULATOR"
            ;;
        appletvos*)
            HEIMDALL_HOST_PLATFORM="tvOS"
            HEIMDALL_HOST_OS="tvos"
            set_deployment_target "$HEIMDALL_DEPLOYMENT_TARGET_TVOS"
            ;;
        appletvsimulator*)
            HEIMDALL_HOST_PLATFORM="tvOS-Simulator"
            HEIMDALL_HOST_PLATFORM_SIMULATOR="yes"
            HEIMDALL_HOST_OS="tvos"
            set_deployment_target "$HEIMDALL_DEPLOYMENT_TARGET_TVOS_SIMULATOR"
            ;;
        macosx*)
            HEIMDALL_HOST_PLATFORM="macOS"
            HEIMDALL_HOST_OS="macosx"
            set_deployment_target "$HEIMDALL_DEPLOYMENT_TARGET_MACOSX"
            ;;
        xros*)
            HEIMDALL_HOST_PLATFORM="xrOS"
            HEIMDALL_HOST_OS="xros"
            set_deployment_target "$HEIMDALL_DEPLOYMENT_TARGET_XROS"
            ;;
        xrsimulator*)
            HEIMDALL_HOST_PLATFORM="xr-Simulator"
            HEIMDALL_HOST_PLATFORM_SIMULATOR="yes"
            HEIMDALL_HOST_OS="xros"
            set_deployment_target "$HEIMDALL_DEPLOYMENT_TARGET_XROS"
            ;;
        watchos*)
            HEIMDALL_HOST_PLATFORM="watchOS"
            HEIMDALL_HOST_OS="watchos"
            set_deployment_target "$HEIMDALL_DEPLOYMENT_TARGET_WATCHOS"
            ;;
        watchsimulator*)
            HEIMDALL_HOST_PLATFORM="watchSimulator"
            HEIMDALL_HOST_PLATFORM_SIMULATOR="yes"
            HEIMDALL_HOST_OS="watchos"
            set_deployment_target "$HEIMDALL_DEPLOYMENT_TARGET_WATCHOS"
            ;;
        *)
            abort_err "Unhandled SDK name '$1'"
            ;;
    esac
}

# Set env variables used to define compilers and flags
# Arguments:
#   Additional flags for use with C-like compilers
# Globals:
#   HEIMDALL_DEPLOYMENT_TARGET_CFLAG
#   HEIMDALL_DEPLOYMENT_TARGET_LDFLAG
#   HEIMDALL_APPLE_SDK_PATH
#   HEIMDALL_HOST_ARCH
set_host_envvars()
{
    # Flags to be used for C-like compilers (C, C++, Obj-C)
    local clike_flags="$HEIMDALL_DEPLOYMENT_TARGET_CFLAG -arch $HEIMDALL_HOST_ARCH -isysroot $HEIMDALL_APPLE_SDK_PATH $1"
    local bitcode_flag=""
    if [ "$HEIMDALL_USE_BITCODE" -gt "0" ]; then
        clike_flags+=" $HEIMDALL_BITCODE_FLAG"
        bitcode_flag=" $HEIMDALL_BITCODE_FLAG"
    fi

    export CPPFLAGS="-arch $HEIMDALL_HOST_ARCH -isysroot $HEIMDALL_APPLE_SDK_PATH"

    export CFLAGS="$clike_flags"
    export CXXFLAGS="$clike_flags"
    export OBJCFLAGS="$clike_flags"
    export OBJCXXFLAGS="$clike_flags"

    # Vanilla clang doesn't use HEIMDALL_DEPLOYMENT_TAGET_LDFLAGS but only the CFLAGS variant
    export LDFLAGS="$HEIMDALL_DEPLOYMENT_TARGET_LDFLAG $HEIMDALL_DEPLOYMENT_TARGET_CFLAG -arch $HEIMDALL_HOST_ARCH -isysroot $HEIMDALL_APPLE_SDK_PATH ${bitcode_flag}"
}

hostenv()
{
    CC="${HEIMDALL_HOST_CC}" \
    CPP="${HEIMDALL_HOST_CPP}" \
    CXX="${HEIMDALL_HOST_CXX}" \
    OBJC="${HEIMDALL_HOST_OBJC}" \
    OBJCXX="${HEIMDALL_HOST_OBJCXX}" \
    LD="${HEIMDALL_HOST_LD}" \
    AR="${HEIMDALL_HOST_AR}" \
    STRIP="${HEIMDALL_HOST_STRIP}" \
    RANLIB="${HEIMDALL_HOST_RANLIB}" \
    NM="${HEIMDALL_HOST_NM}" \
    "$@"
}

ac_var_to_export_ac_var()
{
    for ac_var in "$@"; do
        echo "export $ac_var"
    done
}

# Write config.mak for contribs
# Globals:
#   HEIMDALL_DEPLOYMENT_TARGET_CFLAG
#   HEIMDALL_DEPLOYMENT_TARGET_LDFLAG
#   HEIMDALL_APPLE_SDK_PATH
#   HEIMDALL_HOST_ARCH
write_config_mak()
{
    # Flags to be used for C-like compilers (C, C++, Obj-C)
    local clike_flags="\$(HEIMDALL_DEPLOYMENT_TARGET_CFLAG) -arch $HEIMDALL_HOST_ARCH -isysroot $HEIMDALL_APPLE_SDK_PATH $1"

    local heimdall_cppflags="-arch $HEIMDALL_HOST_ARCH -isysroot $HEIMDALL_APPLE_SDK_PATH"
    local heimdall_cflags="$clike_flags"
    local heimdall_cxxflags="$clike_flags"
    local heimdall_objcflags="$clike_flags"
    local heimdall_objcxxflags="$clike_flags"

    # Vanilla clang doesn't use HEIMDALL_DEPLOYMENT_TAGET_LDFLAGS but only the CFLAGS variant
    local heimdall_ldflags="\$(HEIMDALL_DEPLOYMENT_TARGET_LDFLAG) \$(HEIMDALL_DEPLOYMENT_TARGET_CFLAG) -arch $HEIMDALL_HOST_ARCH -isysroot $HEIMDALL_APPLE_SDK_PATH"

    echo "Creating makefile..."
    test -e config.mak && unlink config.mak
    exec 3>config.mak || return $?

    printf '# This file was automatically generated!\n\n' >&3
    printf '%s := %s\n' "HEIMDALL_DEPLOYMENT_TARGET" "${HEIMDALL_DEPLOYMENT_TARGET}" >&3
    printf '%s := %s\n' "HEIMDALL_DEPLOYMENT_TARGET_CFLAG" "${HEIMDALL_DEPLOYMENT_TARGET_CFLAG}" >&3
    printf '%s := %s\n' "HEIMDALL_DEPLOYMENT_TARGET_LDFLAG" "${HEIMDALL_DEPLOYMENT_TARGET_LDFLAG}" >&3

    printf '%s := %s\n' "CPPFLAGS" "${heimdall_cppflags}" >&3
    printf '%s := %s\n' "CFLAGS" "${heimdall_cflags}" >&3
    printf '%s := %s\n' "CXXFLAGS" "${heimdall_cxxflags}" >&3
    printf '%s := %s\n' "OBJCFLAGS" "${heimdall_objcflags}" >&3
    printf '%s := %s\n' "OBJCXXFLAGS" "${heimdall_objcxxflags}" >&3
    printf '%s := %s\n' "LDFLAGS" "${heimdall_ldflags}" >&3
    printf '%s := %s\n' "CC" "${HEIMDALL_HOST_CC}" >&3
    printf '%s := %s\n' "CPP" "${HEIMDALL_HOST_CPP}" >&3
    printf '%s := %s\n' "CXX" "${HEIMDALL_HOST_CXX}" >&3
    printf '%s := %s\n' "OBJC" "${HEIMDALL_HOST_OBJC}" >&3
    printf '%s := %s\n' "OBJCXX" "${HEIMDALL_HOST_OBJCXX}" >&3
    printf '%s := %s\n' "LD" "${HEIMDALL_HOST_LD}" >&3
    printf '%s := %s\n' "AR" "${HEIMDALL_HOST_AR}" >&3
    printf '%s := %s\n' "STRIP" "${HEIMDALL_HOST_STRIP}" >&3
    printf '%s := %s\n' "RANLIB" "${HEIMDALL_HOST_RANLIB}" >&3
    printf '%s := %s\n' "NM" "${HEIMDALL_HOST_NM}" >&3

    # Add the ac_cv_ var exports in the config.mak for the contribs
    echo "Appending ac_cv_ vars to config.mak"
    heimdallSetSymbolEnvironment ac_var_to_export_ac_var >&3
}

# Generate the source file with the needed array for
# the static Heimdall module list. This has to be compiled
# and linked into the static library
# Arguments:
#   Path of the output file
#   Array with module entry symbol names
gen_heimdall_static_module_list()
{
    local output="$1"
    shift
    local symbol_array=( "$@" )
    touch "$output" || abort_err "Failure creating static module list file"

    local array_list
    local declarations_list

    for symbol in "${symbol_array[@]}"; do
        declarations_list+="HEIMDALL_ENTRY_FUNC(${symbol});\\n"
        array_list+="    ${symbol},\\n"
    done

    printf "\
#include <stddef.h>\\n\
#include <heimdall_plugin.h>\\n\
%b\\n\
const heimdall_plugin_cb heimdall_static_modules[] = {\\n
%b
    NULL\\n
};" \
    "$declarations_list" "$array_list" >> "$output" \
      || abort_err "Failure writing static module list file"
}

##########################################################
#                  Main script logic                     #
##########################################################

# Parse arguments
while [ -n "$1" ]
do
    case "$1" in
        --help|-h)
            usage
            exit 0
            ;;
        --verbose)
            HEIMDALL_SCRIPT_VERBOSE=1
            HEIMDALL_USE_NUMBER_OF_CORES=1
            ;;
        --disable-debug)
            HEIMDALL_DISABLE_DEBUG=1
            ;;
        --enable-bitcode|--enable-bitcode=full)
            HEIMDALL_USE_BITCODE=1
            ;;
        --enable-bitcode=marker)
            HEIMDALL_USE_BITCODE=1
            HEIMDALL_BITCODE_FLAG="-fembed-bitcode-marker"
            ;;
        --arch=*)
            HEIMDALL_HOST_ARCH="${1#--arch=}"
            ;;
        --sdk=*)
            HEIMDALL_APPLE_SDK_NAME="${1#--sdk=}"
            ;;
        --package-contribs)
            HEIMDALL_MAKE_PREBUILT_CONTRIBS=1
            ;;
        --with-prebuilt-contribs)
            HEIMDALL_USE_PREBUILT_CONTRIBS=1
            ;;
        --enable-shared)
            HEIMDALL_BUILD_DYNAMIC=1
            ;;
        --enable-extra-checks)
            HEIMDALL_BUILD_EXTRA_CHECKS=1
            ;;
        --with-contrib=*)
            HEIMDALL_WITH_CONTRIB="${1#--with-contrib=}"
            ;;
        --with-contrib)
            HEIMDALL_WITH_CONTRIB=yes
            ;;
        --without-contrib)
            HEIMDALL_WITH_CONTRIB=no
            ;;
        --config=*)
            HEIMDALL_BUILD_CONF_PATH="${1#--config=}"
            ;;
        HEIMDALL_PREBUILT_CONTRIBS_URL=*)
            HEIMDALL_PREBUILT_CONTRIBS_URL="${1#HEIMDALL_PREBUILT_CONTRIBS_URL=}"
            ;;
        -j*)
            HEIMDALL_REQUESTED_CORE_COUNT=${1#-j}
            ;;
        *)
            echo >&2 "ERROR: Unrecognized option '$1'"
            usage
            exit 1
            ;;
    esac
    shift
done

validate_build_conf "${HEIMDALL_BUILD_CONF_PATH}"
. "${HEIMDALL_BUILD_CONF_PATH}" || abort_err "Failed to source build config: ${HEIMDALL_BUILD_CONF_PATH}"

export MAKEFLAGS="-j${HEIMDALL_USE_NUMBER_OF_CORES} ${MAKEFLAGS}"
if [ "${HEIMDALL_REQUESTED_CORE_COUNT}" != "0" ]; then
    export MAKEFLAGS="${MAKEFLAGS} -j${HEIMDALL_REQUESTED_CORE_COUNT}"
fi

# Check for some required tools before proceeding
check_tool xcrun

# TODO: Better command to get SDK name if none is set:
# xcodebuild -sdk $(xcrun --show-sdk-path) -version | awk -F '[()]' '{ print $2; exit; }'
# Aditionally a lot more is reported by this command, so this needs some more
# awk parsing or something to get other values with just one query.

# Validate given SDK name
validate_sdk_name "$HEIMDALL_APPLE_SDK_NAME"

# Validate architecture argument
validate_architecture "$HEIMDALL_HOST_ARCH"

# Set triplet (needs to be called after validating the arch)
set_host_triplet "$HEIMDALL_HOST_ARCH"
set_build_triplet

# Set pseudo-triplet
# FIXME: This should match the actual clang triplet and should be used for compiler invocation too!
readonly HEIMDALL_PSEUDO_TRIPLET="${HEIMDALL_HOST_ARCH}-apple-${HEIMDALL_HOST_PLATFORM}_${HEIMDALL_DEPLOYMENT_TARGET}"
# Contrib install dir
readonly HEIMDALL_CONTRIB_INSTALL_DIR="$HEIMDALL_SRC_DIR/contrib/${HEIMDALL_HOST_ARCH}-${HEIMDALL_APPLE_SDK_NAME}"
# Heimdall install dir
readonly HEIMDALL_INSTALL_DIR="$HEIMDALL_BUILD_DIR/heimdall-${HEIMDALL_APPLE_SDK_NAME}-${HEIMDALL_HOST_ARCH}"

echo "Build configuration"
echo "  Platform:         $HEIMDALL_HOST_PLATFORM"
echo "  Architecture:     $HEIMDALL_HOST_ARCH"
echo "  SDK Version:      $HEIMDALL_APPLE_SDK_VERSION"
echo "  Xcode Version:    $HEIMDALL_APPLE_XCODE_VERSION"
echo "  Number of Cores:  $HEIMDALL_USE_NUMBER_OF_CORES"
if [ "$HEIMDALL_USE_BITCODE" -gt 0 ]; then
echo "  Bitcode:          enabled"
else
echo "  Bitcode:          disabled"
fi
echo ""

##########################################################
#                Prepare environment                     #
##########################################################

# Set PKG_CONFIG_LIBDIR to an empty string to prevent
# pkg-config from finding dependencies on the build
# machine, so that it only finds deps in contribs
export PKG_CONFIG_LIBDIR=""

# Add extras/tools to path
export PATH="$HEIMDALL_SRC_DIR/extras/tools/build/bin:$PATH"

# Do NOT set SDKROOT, as that is used by various Apple
# tools and clang and would lead to wrong results!
# Instead for now we set HEIMDALLSDKROOT which is needed
# to make the contrib script happy.
# TODO: Actually for macOS the contrib bootstrap script
# expects SDKROOT to be set, although we can't just do that
# due to the previously mentioned problem this causes.
export HEIMDALLSDKROOT="$HEIMDALL_APPLE_SDK_PATH"

# TODO: Adjust how that is handled in contrib script, to
# get rid of these env varibles that we need to set
if [ "$HEIMDALL_HOST_OS" = "ios" ]; then
    export BUILDFORIOS="yes"
elif [ "$HEIMDALL_HOST_OS" = "tvos" ]; then
    export BUILDFORIOS="yes"
    export BUILDFORTVOS="yes"
elif [ "$HEIMDALL_HOST_OS" = "xros" ]; then
    export BUILDFORIOS="yes"
    export BUILDFORVISIONOS="yes"
elif [ "$HEIMDALL_HOST_OS" = "watchos" ]; then
    export BUILDFORIOS="yes"
    export BUILDFORWATCHOS="yes"
fi
if [ "$HEIMDALL_HOST_PLATFORM_SIMULATOR" = "yes" ]; then
    export BUILDFORSIMULATOR="yes"
fi

# Default to "make" if there is no MAKE env variable
MAKE=${MAKE:-make}

# Attention! Do NOT use just "libtool" here and
# do NOT use the LIBTOOL env variable as this is
# expected to be Apple's libtool NOT GNU libtool!
APPL_LIBTOOL=$(xcrun -f libtool) \
  || abort_err "Failed to find Apple libtool with xcrun"

##########################################################
#                 Extras tools build                     #
##########################################################

echo "Building needed tools (if missing)"

TOOLS_BOOTSTRAP=
if [ "$HEIMDALL_USE_PREBUILT_CONTRIBS" = "0" ]; then
    if [ "$HEIMDALL_HOST_OS" = "xros" ]; then
        TOOLS_BOOTSTRAP="$TOOLS_BOOTSTRAP --min-cmake=3.28.0"
    fi
    # force local build until implicit C++ is properly supported, cmake 4.x is OK
    if [ "$HEIMDALL_HOST_OS" = "ios" ] || [ "$HEIMDALL_HOST_OS" = "tvos" ] || [ "$HEIMDALL_HOST_OS" = "watchos" ] || [ "$HEIMDALL_HOST_OS" = "xros" ]; then
        TOOLS_BOOTSTRAP="$TOOLS_BOOTSTRAP --min-cmake=3.31.10"
    fi
fi

cd "$HEIMDALL_SRC_DIR/extras/tools" || abort_err "Failed cd to tools dir"
./bootstrap $TOOLS_BOOTSTRAP || abort_err "Bootstrapping tools failed"
$MAKE || abort_err "Building tools failed"
echo ""

##########################################################
#                     Contribs build                     #
##########################################################
if [ "$HEIMDALL_WITH_CONTRIB" = "no" ] || { [ -n "$HEIMDALL_WITH_CONTRIB" ] && [ "$HEIMDALL_WITH_CONTRIB" != "yes" ]; }; then
    echo "Skipping contribs build"
else
# Combine settings from config file
HEIMDALL_CONTRIB_OPTIONS=( "${HEIMDALL_CONTRIB_OPTIONS_BASE[@]}" )

if [ "$HEIMDALL_HOST_OS" = "macosx" ]; then
    HEIMDALL_CONTRIB_OPTIONS+=( "${HEIMDALL_CONTRIB_OPTIONS_MACOSX[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "ios" ]; then
    HEIMDALL_CONTRIB_OPTIONS+=( "${HEIMDALL_CONTRIB_OPTIONS_IOS[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "tvos" ]; then
    HEIMDALL_CONTRIB_OPTIONS+=( "${HEIMDALL_CONTRIB_OPTIONS_TVOS[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "xros" ]; then
    HEIMDALL_CONTRIB_OPTIONS+=( "${HEIMDALL_CONTRIB_OPTIONS_XROS[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "watchos" ]; then
    HEIMDALL_CONTRIB_OPTIONS=( "${HEIMDALL_CONTRIB_OPTIONS_WATCHOS[@]}" )
fi

# Create dir to build contribs in
cd "$HEIMDALL_SRC_DIR/contrib" || abort_err "Failed cd to contrib dir"
mkdir -p "contrib-$HEIMDALL_PSEUDO_TRIPLET"
cd "contrib-$HEIMDALL_PSEUDO_TRIPLET" || abort_err "Failed cd to contrib build dir"

# Create contrib install dir if it does not already exist
mkdir -p "$HEIMDALL_CONTRIB_INSTALL_DIR"

# Write config.mak with flags for the build and compiler overrides
# Set flag to error on partial availability
write_config_mak "-Werror=partial-availability"

if [ "$HEIMDALL_USE_BITCODE" -gt "0" ]; then
    HEIMDALL_CONTRIB_OPTIONS+=( "--enable-bitcode" )
fi

# Bootstrap contribs
../bootstrap \
    --host="$HEIMDALL_HOST_TRIPLET" \
    ${HEIMDALL_BUILD_TRIPLET:+--build="$HEIMDALL_BUILD_TRIPLET"} \
    --prefix="$HEIMDALL_CONTRIB_INSTALL_DIR" \
    "${HEIMDALL_CONTRIB_OPTIONS[@]}" \
|| abort_err "Bootstrapping contribs failed"

# Print list of contribs that will be built
$MAKE list

if [ "$HEIMDALL_USE_PREBUILT_CONTRIBS" -gt "0" ]; then
    echo "Fetching prebuilt contribs"
    # Fetch prebuilt contribs
    if [ -z "$HEIMDALL_PREBUILT_CONTRIBS_URL" ]; then
        $MAKE prebuilt || PREBUILT_FAILED=yes && echo "ERROR: Fetching prebuilt contribs failed" >&2
    else
        $MAKE prebuilt PREBUILT_URL="$HEIMDALL_PREBUILT_CONTRIBS_URL" \
             || PREBUILT_FAILED=yes && echo "ERROR: Fetching prebuilt contribs from ${HEIMDALL_PREBUILT_CONTRIBS_URL} failed" >&2
    fi
else
    PREBUILT_FAILED=yes
fi
if [ -n "$PREBUILT_FAILED" ]; then
    echo "Building contribs for $HEIMDALL_HOST_ARCH"
    # Download source packages
    $MAKE fetch

    # Build contribs
    $MAKE || abort_err "Building contribs failed"

    # Make prebuilt contribs package
    if [ "$HEIMDALL_MAKE_PREBUILT_CONTRIBS" -gt "0" ]; then
        $MAKE package || abort_err "Creating prebuilt contribs package failed"
    fi
else
    $MAKE tools
fi
fi # HEIMDALL_WITH_CONTRIB

echo ""

##########################################################
#                      Heimdall build                         #
##########################################################

echo "Building Heimdall for $HEIMDALL_HOST_ARCH"

# Set flags for Heimdall build
set_host_envvars "-g"

# Combine settings from config file
HEIMDALL_CONFIG_OPTIONS=( "${HEIMDALL_CONFIG_OPTIONS_BASE[@]}" )

if [ "$HEIMDALL_HOST_OS" = "macosx" ]; then
    HEIMDALL_CONFIG_OPTIONS+=( "${HEIMDALL_CONFIG_OPTIONS_MACOSX[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "ios" ]; then
    HEIMDALL_CONFIG_OPTIONS+=( "${HEIMDALL_CONFIG_OPTIONS_IOS[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "tvos" ]; then
    HEIMDALL_CONFIG_OPTIONS+=( "${HEIMDALL_CONFIG_OPTIONS_TVOS[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "xros" ]; then
    HEIMDALL_CONFIG_OPTIONS+=( "${HEIMDALL_CONFIG_OPTIONS_XROS[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "watchos" ]; then
    HEIMDALL_CONFIG_OPTIONS+=( "${HEIMDALL_CONFIG_OPTIONS_WATCHOS[@]}" )
fi

if [ "$HEIMDALL_DISABLE_DEBUG" -gt "0" ]; then
    HEIMDALL_CONFIG_OPTIONS+=( "--disable-debug" )
fi

if [ "$HEIMDALL_BUILD_DYNAMIC" -gt "0" ]; then
    HEIMDALL_CONFIG_OPTIONS+=( "--enable-shared" )
else
    HEIMDALL_CONFIG_OPTIONS+=( "--disable-shared" "--enable-static" )
fi

if [ "$HEIMDALL_BUILD_EXTRA_CHECKS" -gt "0" ]; then
    HEIMDALL_CONFIG_OPTIONS+=( "--enable-extra-checks" )
fi

# Bootstrap Heimdall
cd "$HEIMDALL_SRC_DIR" || abort_err "Failed cd to Heimdall source dir"
if ! [ -e configure ]; then
    echo "Bootstraping heimdall"
    ./bootstrap
fi

# Build
mkdir -p "${HEIMDALL_BUILD_DIR}/build"
cd "${HEIMDALL_BUILD_DIR}/build" || abort_err "Failed cd to Heimdall build dir"

# Create Heimdall install dir if it does not already exist
mkdir -p "$HEIMDALL_INSTALL_DIR"


# Determine how to configure the contribs
if [ "$HEIMDALL_WITH_CONTRIB" = "no" ]; then
    HEIMDALL_CONTRIB_CONFIGURE_ARG="--without-contrib"
elif [ -n "$HEIMDALL_WITH_CONTRIB" ] && [ "$HEIMDALL_WITH_CONTRIB" != "yes" ]; then
    HEIMDALL_CONTRIB_CONFIGURE_ARG="--with-contrib=$HEIMDALL_WITH_CONTRIB"
else
    HEIMDALL_CONTRIB_CONFIGURE_ARG="--with-contrib=$HEIMDALL_CONTRIB_INSTALL_DIR"
fi

heimdallSetSymbolEnvironment \
hostenv "${HEIMDALL_SRC_DIR}/configure" \
    --host="$HEIMDALL_HOST_TRIPLET" \
    ${HEIMDALL_BUILD_TRIPLET:+--build="$HEIMDALL_BUILD_TRIPLET"} \
    --prefix="$HEIMDALL_INSTALL_DIR" \
    "$HEIMDALL_CONTRIB_CONFIGURE_ARG" \
    "${HEIMDALL_CONFIG_OPTIONS[@]}" \
    CFLAGS="${CFLAGS}" \
    OBJCFLAGS="${OBJCFLAGS}" \
    CXXFLAGS="${CXXFLAGS}" \
    OBJCXXFLAGS="${OBJCXXFLAGS}" \
 || abort_err "Configuring Heimdall failed"

$MAKE || abort_err "Building Heimdall failed"

$MAKE install || abort_err "Installing Heimdall failed"

echo ""
# Shortcut the build of the static bundle when using the dynamic loader
if [ "$HEIMDALL_BUILD_DYNAMIC" -gt "0" ]; then
    echo "Build succeeded!"
    exit 0
fi

##########################################################
#                 Remove unused modules                  #
##########################################################

echo "Removing modules that are on the removal list"

# Combine settings from config file
HEIMDALL_MODULE_REMOVAL_LIST=( "${HEIMDALL_MODULE_REMOVAL_LIST_BASE[@]}" )

if [ "$HEIMDALL_HOST_OS" = "macosx" ]; then
    HEIMDALL_MODULE_REMOVAL_LIST+=( "${HEIMDALL_MODULE_REMOVAL_LIST_MACOSX[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "ios" ]; then
    HEIMDALL_MODULE_REMOVAL_LIST+=( "${HEIMDALL_MODULE_REMOVAL_LIST_IOS[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "tvos" ]; then
    HEIMDALL_MODULE_REMOVAL_LIST+=( "${HEIMDALL_MODULE_REMOVAL_LIST_TVOS[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "xros" ]; then
    HEIMDALL_MODULE_REMOVAL_LIST+=( "${HEIMDALL_MODULE_REMOVAL_LIST_XROS[@]}" )
elif [ "$HEIMDALL_HOST_OS" = "watchos" ]; then
    HEIMDALL_MODULE_REMOVAL_LIST+=( "${HEIMDALL_MODULE_REMOVAL_LIST_WATCHOS[@]}" )
fi

for module in "${HEIMDALL_MODULE_REMOVAL_LIST[@]}"; do
    find "$HEIMDALL_INSTALL_DIR/lib/heimdall/plugins" \
        -name "lib${module}_plugin.a" \
        -type f \
        -exec rm '{}' \;
done

echo ""

##########################################################
#        Compile object with static module list          #
##########################################################

echo "Compile Heimdall static modules list object"

mkdir -p "${HEIMDALL_BUILD_DIR}/static-lib"
cd "${HEIMDALL_BUILD_DIR}/static-lib" \
 || abort_err "Failed cd to Heimdall static-lib build dir"

# Collect paths of all static libraries needed (plugins and contribs)
HEIMDALL_STATIC_FILELIST_NAME="static-libs-list"
rm -f "$HEIMDALL_STATIC_FILELIST_NAME"
touch "$HEIMDALL_STATIC_FILELIST_NAME"

HEIMDALL_PLUGINS_SYMBOL_LIST=()

# Find all static plugins in build dir
while IFS=  read -r -d $'\0' plugin_path; do
    # Get module entry point symbol name (_heimdall_entry__MODULEFULLNAME)
    nm_symbol_output=( $(${HEIMDALL_HOST_NM} "$plugin_path" | grep _heimdall_entry__) ) \
      || abort_err "Failed to find module entry function in '$plugin_path'"

    symbol_name="${nm_symbol_output[2]:1}"
    HEIMDALL_PLUGINS_SYMBOL_LIST+=( "$symbol_name" )

    echo "$plugin_path" >> "$HEIMDALL_STATIC_FILELIST_NAME"

done < <(find "$HEIMDALL_INSTALL_DIR/lib/heimdall/plugins" -name "*.a" -print0)

# Generate code with module list
HEIMDALL_STATIC_MODULELIST_NAME="static-module-list"
rm -f "${HEIMDALL_STATIC_MODULELIST_NAME}.c" "${HEIMDALL_STATIC_MODULELIST_NAME}.o"
gen_heimdall_static_module_list "${HEIMDALL_STATIC_MODULELIST_NAME}.c" "${HEIMDALL_PLUGINS_SYMBOL_LIST[@]}"

${HEIMDALL_HOST_CC:-cc} -c  ${CFLAGS} -I"${HEIMDALL_SRC_DIR}/include" -I"${HEIMDALL_BUILD_DIR}/build" "${HEIMDALL_STATIC_MODULELIST_NAME}.c" \
  || abort_err "Compiling module list file failed"

echo "${HEIMDALL_BUILD_DIR}/static-lib/${HEIMDALL_STATIC_MODULELIST_NAME}.o" \
  >> "$HEIMDALL_STATIC_FILELIST_NAME"

echo ""

##########################################################
#          Link together full static library             #
##########################################################

echo "Linking Heimdall modules and contribs statically"

echo "$HEIMDALL_INSTALL_DIR/lib/libheimdall.a" >> "$HEIMDALL_STATIC_FILELIST_NAME"
echo "$HEIMDALL_INSTALL_DIR/lib/libheimdallcore.a" >> "$HEIMDALL_STATIC_FILELIST_NAME"
echo "$HEIMDALL_INSTALL_DIR/lib/heimdall/libcompat.a" >> "$HEIMDALL_STATIC_FILELIST_NAME"

# Find all static contribs in build dir
find "$HEIMDALL_CONTRIB_INSTALL_DIR/lib" -name '*.a' -print >> "$HEIMDALL_STATIC_FILELIST_NAME" \
  || abort_err "Failed finding installed static contribs in '$HEIMDALL_CONTRIB_INSTALL_DIR/lib'"

# Link static libs together using libtool
$APPL_LIBTOOL -static \
    -no_warning_for_no_symbols \
    -filelist "$HEIMDALL_STATIC_FILELIST_NAME" \
    -o "libheimdall-full-static.a" \
  || abort_err "Failed running Apple libtool to combine static libraries together"

echo ""
echo "Build succeeded!"
