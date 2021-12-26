#!/bin/sh

set -x
SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./build}

mkdir -p $BUILD_DIR \
    && cd $BUILD_DIR \
    && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        $SOURCE_DIR \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=YES \
    && make &* -j 12