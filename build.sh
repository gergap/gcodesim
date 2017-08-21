#!/bin/bash

# default settings
BLDDIR=$PWD/bld
SRCDIR=$PWD
DISTDIR=$PWD
BUILD_TYPE="Release"
BUILD_TARGET="all"
# additional CMake options
CMAKE=cmake
OPTIONS=
# Use GNU Make
MAKE=make
MAKEOPTS="-j4"
GENERATOR=
# Use Ninja
MAKE=ninja
GENERATOR=-GNinja
# Unit Testing
RUN_TESTS=1
STRIP=0

function usage {
    echo "Usage: ./build.sh [-m32] [-b <build_type>] [-d] [-o <options>] [clean] [doc] [<build_target>]"
    echo "  -m32:  build 32bit code on 64bit hosts"
    echo "  -b:    select CMAKE_BUILD_TYPE"
    echo "  -o:    custom CMake options"
    echo "  -d:    sets a different installation prefix"
    echo "  -t:    select target configuration: mingw32"
    echo "  -T:    disable Testing (BUILD_TESTING=off)"
    echo "  -R:    disable test execution only"
    echo "  clean: performs a clean build by removing the build directory first"
    echo "  <build_target>: CMake build target like e.g. Nightly, doc, ..."
    echo "    If no build target was given it will be default build all, test,"
    echo "    and install."
    exit 1
}

# process commandline arguments
while getopts "m:b:d:t:o:RT" opt; do
    case $opt in
        m)
            if [ "$OPTARG" == "32" ]; then
                # build 32bit exeutable on 64bit systems
                BLDDIR="${BLDDIR}32"
                DISTDIR="${DISTDIR}/dist32"
                export CFLAGS="-m32 -march=i686"
            else
                usage
            fi
            ;;
        b)
            # select build type
            BUILD_TYPE=${OPTARG}
            BLDDIR="${BLDDIR}${BUILD_TYPE}"
            ;;
        d)
            DISTDIR="$OPTARG"
            ;;
        t)
            # load target configuration
            echo "Loading $OPTARG target configuration"
            source "target_config_$OPTARG" || exit 1
            OPTIONS="$OPTIONS -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN"
            ;;
        o)
            OPTIONS="$OPTIONS $OPTARG"
            ;;
        T)
            # disable test builds
            OPTIONS="$OPTIONS -DBUILD_TESTING=off"
            RUN_TESTS=0
            ;;
        R)
            # disable test execution only
            RUN_TESTS=0
            ;;
        *)
            usage
            ;;
    esac
done

# shift out arguments processed by getopt
shift $((OPTIND-1))

if [ $# -gt 0 ] && [ "$1" == "clean" ]; then
    shift 1
    rm -rf $BLDDIR
fi
# optional build target
if [ $# -gt 0 ]; then
    BUILD_TARGET=$1
    BLDDIR=${BLDDIR}$1
fi

mkdir -p $BLDDIR
cd $BLDDIR
$CMAKE $GENERATOR $OPTIONS -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX=$DISTDIR .. || exit 1

if [ "$BUILD_TARGET" == "all" ]; then
    # default developer build including test and install
    $MAKE $MAKEOPTS all || exit 3
    if [ $STRIP -eq 0 ]; then
        $MAKE install || exit 3
    else
        $MAKE install/strip || exit 3
    fi
    if [ $RUN_TESTS -eq 1 ]; then
        $MAKE test || exit 3
    fi
else
    # custom build target: for ci-system, etc.
    $MAKE $MAKEOPTS $BUILD_TARGET || exit 3
fi


