#!/bin/bash

wget https://cloud-images.ubuntu.com/noble/current/noble-server-cloudimg-amd64.img
qemu-img convert -f qcow2 -O qcow2 noble-server-cloudimg-amd64.img iec61850.qcow2
qemu-img resize iec61850.qcow2 +2G

cat > user-data <<EOF
#cloud-config
users:
  - name: iec61850
    sudo: ALL=(ALL) NOPASSWD:ALL
    groups: users, admin
    home: /home/iec61850
    shell: /bin/bash
    lock_passwd: false
    plain_text_passwd: iec61850

packages:
  - build-essential
  - cmake
  - git

runcmd:
  - mkdir -p /mnt/host
  - mkdir -p /opt/iec61850
  - git clone https://github.com/mendax0110/IEC61850.git /opt/iec61850
  - cd /opt/iec61850 && mkdir build && cd build && cmake .. && make
EOF

cloud-localds seed.img user-data

echo "Image prepared: iec61850.qcow2 and seed.img"