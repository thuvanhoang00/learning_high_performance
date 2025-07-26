#!/bin/bash

CMAKE=$(which cmake)
BUILD_DIR=cmake-build-release
MAKE=$(which make)

mkdir -p $BUILD_DIR
$CMAKE -DCMAKE_BUILD_TYPE=Release -S . -B $BUILD_DIR

$CMAKE --build $BUILD_DIR --target -j 4 clean
$CMAKE --build $BUILD_DIR --target -j 4 all

# Clean the build directory using make
$MAKE -C $BUILD_DIR clean -j 4

# Build all targets using make
$MAKE -C $BUILD_DIR all -j 4