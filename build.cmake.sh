#!/bin/bash -e

mkdir -p build
pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX=$PWD/install
    make -j4
    make install
popd
