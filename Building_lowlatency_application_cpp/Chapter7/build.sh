#!/bin/bash

CMAKE=$(which cmake)
NINJA=$(which ninja)
BUILD_DIR=cmake-build-release

mkdir -p cmake-build-release
# $CMAKE -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=$NINJA -G Ninja -S . -B cmake-build-release
# clean
# $CMAKE --build "$BUILD_DIR" --target clean
# # build
# $CMAKE --build "$BUILD_DIR" -- -j4

# $CMAKE --build cmake-build-release --target clean -j 4
# $CMAKE --build cmake-build-release --target all -j 4