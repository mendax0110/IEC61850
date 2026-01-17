#!/bin/bash

set -e

echo "Updating package list..."
sudo apt update

echo "Installing build tools..."
sudo apt install -y build-essential cmake git

echo "Installing testing and documentation tools..."
sudo apt install -y doxygen

echo "Installing QEMU (minimal)..."
sudo apt install -y qemu-system-x86 qemu-utils busybox-static cpio

echo "Installing network tools..."
sudo apt install -y iproute2 bridge-utils

echo "All dependencies installed successfully!"