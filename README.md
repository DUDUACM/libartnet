# libartnet

A C library for the [Art-Net 4](https://art-net.org.uk/) protocol, implementing DMX512 data distribution over UDP/IP networks.

Art-Net 4 Protocol Specification: [ArtNet4.md](ArtNet4.md)

## Features

- Art-Net 4 protocol compliance with 15-bit port addressing (32768 universes)
- Node and Controller modes with up to 4 ports per node
- Node joining for multi-node configurations (8+ universes)
- DMX512 transmit and receive (ArtDmx, ArtNzs)
- Node discovery (ArtPoll / ArtPollReply)
- RDM over Art-Net (ArtRdm, ArtTodRequest, ArtTodData, ArtTodControl)
- Firmware upload (ArtFirmwareMaster / ArtFirmwareReply)
- Remote programming (ArtAddress, ArtInput, ArtIPProg)
- TimeCode, TimeSync, Trigger, Sync, Diagnostic messages
- IPv4 and IPv6 support
- Cross-platform: Linux, macOS, Windows

## Building

### Prerequisites

- CMake 3.10+
- C99 compiler (GCC, Clang, MSVC)

### Build Steps

```bash
mkdir build && cd build
cmake ..
make
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SHARED_LIBS` | `ON` | Build shared library (.so/.dylib/.dll); set `OFF` for static (.a/.lib) |
| `ENABLE_IPV6` | `ON` | Enable IPv6 support |

### Static Library

```bash
cmake .. -DBUILD_SHARED_LIBS=OFF
```

### Cross-Platform Notes

- **Linux**: Uses `getifaddrs()` with `AF_PACKET` for MAC address retrieval
- **macOS**: Uses `getifaddrs()` with `AF_LINK` for MAC address retrieval
- **Windows**: Uses `GetAdaptersInfo()`; links against `ws2_32`, `iphlpapi`, `netapi32`

## Installation

```bash
cmake --install build --prefix /usr/local
```

Installs:
- Library: `lib/libartnet.so` (or `.a` for static builds)
- Headers: `include/artnet/artnet.h`, `include/artnet/common.h`, `include/artnet/packets.h`
- pkg-config: `lib/pkgconfig/libartnet.pc`
- CMake config: `lib/cmake/libartnet/`

### Using in Your CMake Project

```cmake
find_package(libartnet REQUIRED)
target_link_libraries(your_target PRIVATE libartnet::artnet)
```

## Examples

Two example programs are included:

### DMX Node (`dmx_srv_node_rx`)

Art-Net node that receives and transmits DMX on 8 universes (2 joined nodes):

```bash
./build/examples/dmx_srv_node_rx/dmx_srv_node_rx
./build/examples/dmx_srv_node_rx/dmx_srv_node_rx -i 192.168.1.10 -n 0 -s 0 -u 0
./build/examples/dmx_srv_node_rx/dmx_srv_node_rx --no-chase  # receive only
```

### DMX Controller (`dmx_srv_controller_tx_rx`)

Art-Net controller that discovers nodes, receives DMX, and sends a sine wave chase:

```bash
./build/examples/dmx_srv_controller_tx_rx/dmx_srv_controller_tx_rx
./build/examples/dmx_srv_controller_tx_rx/dmx_srv_controller_tx_rx -i 192.168.1.100 --no-chase
```

### Common Options

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |
| `-n <net>` | Net address 0-127 (default: 0) |
| `-s <subnet>` | Subnet address 0-15 (default: 0) |
| `-u <universe>` | Starting port address 0-15 (default: 0) |
| `-c <channels>` | DMX channels per universe 1-512 (default: 512) |
| `--no-chase` | Disable DMX transmit, only receive |

## API Overview

```c
#include <artnet/artnet.h>
#include <artnet/common.h>

// Create and configure a node
artnet_node node = artnet_new("192.168.1.10", 0);
artnet_set_node_type(node, ARTNET_NODE);
artnet_set_short_name(node, "My Node");
artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, 0);

// Set DMX receive callback
artnet_set_dmx_handler(node, my_dmx_callback, NULL);

// Start and run
artnet_start(node);
while (running) {
    artnet_read(node, 0);
    artnet_send_dmx(node, 0, 512, dmx_data);
}

// Cleanup
artnet_stop(node);
artnet_destroy(node);
```

## License

LGPL-2.1 - see [COPYING](COPYING) for details.
