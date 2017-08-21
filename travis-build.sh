#!/bin/bash
# Helper script for building also for Windows using MinGw on Travis-CI
# Travis exports the compiler via CC.
# If this is mingw we are executing build.sh with the correct toolchain file.

OPTIONS=

if [ "$CC" = "i686-w64-mingw32-gcc" ]; then
    OPTIONS="-t mingw32"
elif [ "$CC" = "x86_64-w64-mingw32-gcc" ]; then
    OPTIONS="-t mingw64"
fi

./build.sh $OPTIONS || exit 1

