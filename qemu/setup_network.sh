#!/bin/bash

# Setup TAP interfaces for QEMU VMs

sudo ip tuntap add dev tap0 mode tap
sudo ip tuntap add dev tap1 mode tap

sudo ip link set tap0 up
sudo ip link set tap1 up

sudo ip addr add 192.168.1.1/24 dev tap0
sudo ip addr add 192.168.1.2/24 dev tap1

# Bridge them if needed for communication
sudo ip link add br0 type bridge
sudo ip link set tap0 master br0
sudo ip link set tap1 master br0
sudo ip link set br0 up

echo "TAP interfaces and bridge set up."