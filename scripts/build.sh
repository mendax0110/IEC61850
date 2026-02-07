#!/bin/bash
set -e

if ! command -v doxygen &> /dev/null; then
    echo "Doxygen not found. Installing..."
    sudo apt-get update || echo "Warning: apt-get update failed"
    sudo apt-get install -y doxygen graphviz
fi

BUILD_DIR="build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f Makefile ]; then
    cmake -DBUILD_STATIC=ON ..
fi

make -j$(nproc)

cd ../docs
doxygen Doxyfile
echo "Documentation generated in docs/"

cd ../build
ctest --output-on-failure
echo "All tests passed successfully!"