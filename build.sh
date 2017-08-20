#!/bin/bash

BUILD_TYPE="Release"
PREFIX=$PWD

if [ "$1" == "clean" ]; then
    rm -rf bld
fi

mkdir -p bld
cd bld
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX=$PREFIX .. || exit 1
make
make install

