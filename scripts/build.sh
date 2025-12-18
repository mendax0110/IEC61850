#!/bin/bash

if [ ! -d build ]; then
    mkdir build
fi

if [ -f build/Makefile ]; then
    echo "Build directory already configured."
else
    echo "Configuring build directory..."
    cd build
    cmake ..
    cd ..
fi

cd build
make -j$(nproc)