#!/bin/bash

echo "Updating package list..."
sudo apt update

echo "Installing build tools..."
sudo apt install -y build-essential cmake git

echo "Installing testing and documentation tools..."
sudo apt install -y doxygen

echo "Installing QEMU and virtualization tools..."
sudo apt install -y qemu-system-x86 qemu-utils cloud-image-utils

echo "Installing network tools..."
sudo apt install -y iproute2 bridge-utils

echo "Installing additional libraries..."
sudo apt install -y curl wget

echo "All dependencies installed successfully!"