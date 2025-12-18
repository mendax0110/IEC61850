#!/bin/bash

# Start QEMU VM for IEC61850 Client

qemu-system-x86_64 \
    -m 1024 \
    -drive file=iec61850.qcow2,if=virtio \
    -drive file=seed.img,if=virtio,format=raw \
    -net nic,model=virtio \
    -net tap,ifname=tap1,script=no,downscript=no \
    -virtfs local,path=/workspaces/IEC61850,mount_tag=host0,security_model=passthrough,id=host0 \
    -nographic \
    -serial mon:stdio \
    -device virtio-serial-pci \
    -chardev socket,path=/tmp/qemu-monitor-client,server=on,wait=off,id=monitor \
    -mon chardev=monitor

# Inside VM, mount with: 
# sudo mkdir -p /mnt/host
# sudo mount -t 9p -o trans=virtio host0 /mnt/host
# Then run: /mnt/host/build/iec61850_client