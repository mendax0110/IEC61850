# IEC61850 Sampled Values Implementation

This project implements IEC 61850 Sampled Values (SV) in C++17, using modern design patterns and clean architecture.

## Dependencies

Install all required dependencies:

```bash
./install_deps.sh
```

This installs build tools, QEMU, network utilities, and documentation tools.

## Building

```bash
./scripts/build.sh
```

Or manually:

```bash
mkdir build
cd build
cmake ..
make
```

## Running

```bash
./build/iec61850_demo
```

## Testing with Virtual Network

To test the SV communication in a virtual network:

```bash
./scripts/test_network.sh
```

This sets up network namespaces with veth pairs and runs the server and client.

## QEMU Virtualization

To test in virtualized environments:

1. Install deps: `./install_deps.sh`
2. Create VM image: `./qemu/create_image.sh`
3. Setup network: `./qemu/setup_network.sh`
4. Start server VM: `./qemu/start_server_vm.sh`
5. Start client VM: `./qemu/start_client_vm.sh`

The VMs mount the host repo at `/mnt/host`. In VM:
```bash
sudo mount -t 9p -o trans=virtio host0 /mnt/host
cd /mnt/host/build
./iec61850_demo  # Server
./iec61850_client  # Client
```

The VMs will automatically clone, build, and run the IEC61850 code via cloud-init.

## Architecture

- **Core**: Data types and structures (ASDU, AnalogValue, etc.)
- **Model**: IED model, Logical Nodes, SampledValueControlBlocks
- **Network**: Ethernet-based sender for SV frames

Uses smart pointers, RAII, and modular design for extensibility.