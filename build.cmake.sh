#!/bin/bash -e

mkdir -p build
pushd build
    cmake ..
    make -j4
popd
