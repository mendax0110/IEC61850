# IEC61850 Sampled Values Implementation

This project implements IEC 61850 Sampled Values (SV) in C++17, using modern design patterns and clean architecture.

## Dependencies

Install all required dependencies:

```bash
./install_deps.sh
```

## Building

```bash
./scripts/build.sh
```

Or manually:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Running

```bash
./build/iec61850_demo
```

## QEMU Virtualization

For testing SV communication between isolated VMs using lightweight initramfs-based boot:

```bash
# Setup QEMU environment (creates minimal initramfs)
./build/qemu_tool setup

# Setup network interfaces (requires root)
sudo ./build/qemu_tool network

# Start server VM (in terminal 1)
./build/qemu_tool server

# Start client VM (in terminal 2)
./build/qemu_tool client

# Cleanup network when done
sudo ./build/qemu_tool cleanup
```

Inside each VM, the project is mounted at `/mnt/host`:
```bash
cd /mnt/host/build
./iec61850_demo    # Server
./iec61850_client  # Client
```

### QEMU Tool Options

```
qemu_tool <command> [options]

Commands:
  setup       Create initramfs and prepare QEMU environment
  server      Start server VM
  client      Start client VM  
  network     Setup TAP network interfaces
  cleanup     Remove TAP interfaces
  status      Show VM and network status

Options:
  --interface <name>  Network interface (default: tap0/tap1)
  --memory <MB>       VM memory (default: 512)
  --kernel <path>     Custom kernel path
```