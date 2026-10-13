#!/bin/sh
set -e

info()
{
    local green="\033[1;32m"
    local normal="\033[0m"
    echo "[${green}build${normal}] $1"
}

SCRIPTDIR=$(dirname "$0")
source "$SCRIPTDIR/env.build.sh" "none"

SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
HEIMDALLBUILDDIR=""

CORE_COUNT=`getconf NPROCESSORS_ONLN 2>&1`
let JOBS=$CORE_COUNT+1

usage()
{
cat << EOF
usage: $0 [options]

Build heimdall in the current directory

OPTIONS:
   -h            Show some help
   -q            Be quiet
   -j            Force number of cores to be used
   -r            Rebuild everything (tools, contribs, heimdall)
   -c            Recompile contribs from sources
   -p            Build packages for all artifacts
   -i <n|u|z>    Create an installable package
                     n: nightly
                     u: unsigned stripped release archive
                     z: zip
   -k <sdk>      Use the specified SDK (default: $SDKROOT)
   -a <arch>     Use the specified arch (default: $HOST_ARCH)
   -C            Use the specified Heimdall build dir
   -b <url>      Enable breakpad support and send crash reports to this URL
   -d            Disable debug mode (on by default)
   -g <g|l|a>    Select the license of contribs
                     g: GPLv3 (default)
                     l: LGPLv3 + ad-clauses
                     a: LGPLv2 + ad-clauses
   -x            Add extra checks when compiling
EOF

}

spushd()
{
    pushd "$1" > /dev/null
}

spopd()
{
    popd > /dev/null
}

while getopts "qhvrcdpi:k:a:j:C:b:g:x" OPTION
do
     case $OPTION in
         h)
             usage
             exit 1
             ;;
         q)
             set +x
             QUIET="yes"
         ;;
         d)
             NODEBUG="yes"
         ;;
         r)
             REBUILD="yes"
         ;;
         c)
             CONTRIBFROMSOURCE="yes"
         ;;
         p)
             PACKAGE="yes"
         ;;
         i)
             PACKAGETYPE=$OPTARG
         ;;
         a)
             HOST_ARCH=$OPTARG
         ;;
         k)
             SDKROOT=$OPTARG
         ;;
         j)
             JOBS=$OPTARG
         ;;
         C)
             HEIMDALLBUILDDIR=$OPTARG
         ;;
         b)
             BREAKPAD=$OPTARG
         ;;
         g)
             LICENSE=$OPTARG
         ;;
         x)
             EXTRA_CHECKS="yes"
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

#
# Various initialization
#

out="/dev/stdout"
if [ "$QUIET" = "yes" ]; then
    out="/dev/null"
fi

ACTUAL_HOST_ARCH=`get_actual_arch $HOST_ARCH`

info "Building Heimdall for macOS, architecture ${HOST_ARCH} (aka: ${ACTUAL_HOST_ARCH}) on a ${BUILD_ARCH} device"

BUILD_TRIPLET=$(heimdallGetBuildTriplet)
HOST_TRIPLET=$(heimdallGetHostTriplet)
export SDKROOT
heimdallSetBaseEnvironment
heimdallroot="$(heimdallGetRootDir)"

# Checking prerequisites
info "Checking for python3 ..."
python3 --version || { echo "python3 not found. Please install from python.org, or set" \
	"HEIMDALL_PATH environment variable to include python3." \
	; exit 1; }


builddir="$(pwd)"
info "Building in \"$builddir\""

#
# heimdall/extras/tools
#

info "Building building tools"
spushd "${heimdallroot}/extras/tools"
./bootstrap > $out
if [ "$REBUILD" = "yes" ]; then
    make clean
    ./bootstrap > $out
fi
make -j$JOBS > $out
spopd

if [ ! -z "$HEIMDALLBUILDDIR" ];then
    mkdir -p $HEIMDALLBUILDDIR
    pushd $HEIMDALLBUILDDIR
fi

#
# heimdall/contribs
#

heimdallSetSymbolEnvironment
heimdallSetContribEnvironment "$MINIMAL_OSX_VERSION"

info "Building contribs"
mkdir -p contrib
spushd "contrib"

case $LICENSE in
    l)
        # LGPL v3 + ad-clauses
        CONTRIBFLAGS="$CONTRIBFLAGS --disable-gpl --enable-ad-clauses"
    ;;
    a)
        # LGPL v2.1 + ad-clauses
        CONTRIBFLAGS="$CONTRIBFLAGS --disable-gpl --disable-gnuv3 --enable-ad-clauses"
    ;;
    g|*)
        # GPL v3
    ;;
esac

CONTRIBFLAGS="$CONTRIBFLAGS --enable-sections"

if [ "$REBUILD" = "yes" ]; then
    rm -rf contrib-$HOST_TRIPLET
    rm -rf $HOST_TRIPLET
fi
mkdir -p contrib-$HOST_TRIPLET && cd contrib-$HOST_TRIPLET
${heimdallroot}/contrib/bootstrap --build=$BUILD_TRIPLET --host=$HOST_TRIPLET $CONTRIBFLAGS > $out

make list
if [ "$CONTRIBFROMSOURCE" != "yes" ]; then
    if [ ! -e "../$HOST_TRIPLET" ]; then
        if [ -n "$HEIMDALL_PREBUILT_CONTRIBS_URL" ]; then
            make prebuilt PREBUILT_URL="$HEIMDALL_PREBUILT_CONTRIBS_URL" || PREBUILT_FAILED=yes
        else
            make prebuilt || PREBUILT_FAILED=yes
        fi
    fi
else
    PREBUILT_FAILED=yes
fi
if [ -n "$PREBUILT_FAILED" ]; then
    make -j$JOBS fetch
    make -j$JOBS -k || make -j1

    if [ "$PACKAGE" = "yes" ]; then
        make package
    fi
else
    make -j$JOBS tools
fi
spopd


heimdallUnsetContribEnvironment
heimdallSetLibHeimdallEnvironment

#
# heimdall/bootstrap
#

info "Bootstrap-ing configure"
spushd "${heimdallroot}"
if ! [ -e "${heimdallroot}/configure" ]; then
    ${heimdallroot}/bootstrap > $out
fi
spopd


#
# heimdall/configure
#

CONFIGFLAGS=""
if [ ! -z "$BREAKPAD" ]; then
     CONFIGFLAGS="$CONFIGFLAGS --with-breakpad=$BREAKPAD"
fi
if [ "$NODEBUG" = "yes" ]; then
     CONFIGFLAGS="$CONFIGFLAGS --disable-debug"
fi
if [ -n "$EXTRA_CHECKS" ]; then
    CONFIGFLAGS="$CONFIGFLAGS --enable-extra-checks"
fi

export CFLAGS
export CXXFLAGS

if [ "${heimdallroot}/configure" -nt Makefile ]; then

  ${heimdallroot}/extras/package/macosx/configure.sh \
      --build=$BUILD_TRIPLET \
      --host=$HOST_TRIPLET \
      --with-contrib=contrib/$HOST_TRIPLET \
      --with-macosx-version-min=$MINIMAL_OSX_VERSION \
      --with-macosx-sdk=$SDKROOT \
      $CONFIGFLAGS \
      $HEIMDALL_CONFIGURE_ARGS > $out
fi

#
# make
#

if [ "$REBUILD" = "yes" ]; then
    info "Running make clean"
    make clean
fi

info "Running make -j$JOBS"
make -j$JOBS

info "Preparing Heimdall.app"
make Heimdall.app

if [ "$PACKAGETYPE" = "u" ]; then
    info "Copying app with debug symbols into Heimdall-debug.app and stripping"
    rm -rf Heimdall-debug.app
    cp -Rp Heimdall.app Heimdall-debug.app

    # Workaround for breakpad symbol parsing:
    # Symbols must be uploaded for libheimdall(core).dylib, not libheimdall(core).x.dylib
    (cd Heimdall-debug.app/Contents/MacOS/lib/ && rm libheimdallcore.dylib && mv libheimdallcore.*.dylib libheimdallcore.dylib)
    (cd Heimdall-debug.app/Contents/MacOS/lib/ && rm libheimdall.dylib && mv libheimdall.*.dylib libheimdall.dylib)

    find Heimdall.app/ -name "*.dylib" -exec strip -x {} \;
    find Heimdall.app/ -type f -name "Heimdall" -exec strip -x {} \;
    find Heimdall.app/ -type f -name "Sparkle" -exec strip -x {} \;
    find Heimdall.app/ -type f -name "Breakpad" -exec strip -x {} \;

    if [ "$BUILD_TRIPLET" = "$HOST_TRIPLET" ]; then
        bin/heimdall-cache-gen Heimdall.app/Contents/Frameworks/plugins
    fi

    info "Building Heimdall release archive"
    make package-macosx-release
    make package-macosx-sdk

    shasum -a 512 heimdall-*-release.zip
    shasum -a 512 heimdall-macos-sdk-*.tar.gz

elif [ "$PACKAGETYPE" = "z" ]; then
    info "Packaging Heimdall zip archive"
    make package-macosx-zip
elif [ "$PACKAGETYPE" = "n" -o "$PACKAGE" = "yes" ]; then
    info "Building Heimdall dmg package"
    make package-macosx
    make package-macosx-sdk
fi

if [ ! -z "$HEIMDALLBUILDDIR" ]; then
    popd
fi
