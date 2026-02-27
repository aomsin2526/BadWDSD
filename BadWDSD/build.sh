#!/bin/bash

export PICO_SDK_PATH=$PWD/pico_sdk

cd BadWDSD || exit 1

#rm -rf build
mkdir build

cd build || exit 1

cmake $@ .. || exit 1

make clean || exit 1
make BadWDSD -j12 || exit 1
