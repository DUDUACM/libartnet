# libartnet

A C library for the [Art-Net 4](https://art-net.org.uk/) protocol, implementing DMX512 data distribution over UDP/IP networks.

Based on [OpenLightingProject/libartnet](https://github.com/OpenLightingProject/libartnet), upgraded to Art-Net 4 compliance with full 15-bit port addressing support.

Art-Net 4 Protocol Specification: [art-net4.md](art-net4.md)

## Features

- Art-Net 4 protocol compliance with 15-bit port addressing (32768 universes)
- Node and Controller modes with up to 4 ports per node
- Node joining for multi-node configurations (8+ universes)
- DMX512 transmit and receive (ArtDmx, ArtNzs)
- Node discovery (ArtPoll / ArtPollReply with random delay to prevent packet storms)
- RDM over Art-Net (ArtRdm, ArtTodRequest, ArtTodData, ArtTodControl)
- Firmware upload (ArtFirmwareMaster / ArtFirmwareReply)
- Remote programming (ArtAddress, ArtInput, ArtIPProg)
- TimeCode, TimeSync, Trigger, Sync, Diagnostic messages
- ArtDataRequest/Reply for manufacturer-specific data exchange
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
| `BUILD_EXAMPLES` | `ON` | Build example programs |
| `BUILD_WERROR` | `OFF` | Treat compiler warnings as errors |

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

## Supported Packet Types

| Packet | Opcode | Direction | Description |
|--------|--------|-----------|-------------|
| ArtPoll | 0x2000 | TX/RX | Discover nodes on the network |
| ArtPollReply | 0x2100 | TX/RX | Node identification response |
| ArtDmx | 0x5000 | TX/RX | DMX512 data transfer |
| ArtNzs | 0x5100 | TX/RX | Non-zero start code DMX |
| ArtSync | 0x5200 | TX/RX | Synchronize DMX output |
| ArtAddress | 0x6000 | TX | Remote programming |
| ArtInput | 0x7000 | TX | Remote port configuration |
| ArtTodRequest | 0x8000 | TX/RX | Request Table of Devices |
| ArtTodData | 0x8100 | TX/RX | Transfer Table of Devices |
| ArtTodControl | 0x8200 | RX | RDM discovery control |
| ArtRdm | 0x8300 | TX/RX | RDM sub-device communication |
| ArtFirmwareMaster | 0x9000 | TX | Firmware upload |
| ArtFirmwareReply | 0x9100 | TX/RX | Firmware upload response |
| ArtTimeCode | 0x9700 | TX/RX | Time code distribution |
| ArtTimeSync | 0x9800 | TX | Time synchronization |
| ArtTrigger | 0x9900 | TX | Trigger macros/show keys |
| ArtCommand | 0x9400 | RX | String command |
| ArtDiagData | 0x2300 | TX | Diagnostic text messages |
| ArtIpProg | 0xF800 | TX/RX | IP programming |
| ArtIpProgReply | 0xF810 | TX | IP programming response |
| ArtDirectory | 0x9A00 | RX | Directory request |
| ArtDirectoryReply | 0x9B00 | TX | Directory response |
| ArtFileTnMaster | 0x9C00 | RX | File transfer name |
| ArtFileFnMaster | 0x9D00 | RX | File transfer function |
| ArtFileFnReply | 0x9E00 | TX | File transfer response |
| ArtMediaPatch | 0x9200 | RX | Media patch control |
| ArtMediaControl | 0x9300 | RX | Media playback control |
| ArtDataRequest | 0x2700 | RX | Manufacturer data request |
| ArtDataReply | 0x2800 | TX | Manufacturer data reply |

## API Overview

### Node Lifecycle

```c
artnet_node node = artnet_new("192.168.1.10", 0);  // create
artnet_start(node);                                 // start (controllers auto-poll)
while (running) artnet_read(node, timeout);         // event loop
artnet_stop(node);                                  // stop
artnet_destroy(node);                               // cleanup
```

### Port Configuration

```c
artnet_set_node_type(node, ARTNET_NODE);            // ARTNET_NODE or ARTNET_SRV
artnet_set_style_code(node, ARTNET_ST_NODE);        // product style
artnet_set_status2(node, status2_flags);            // Art-Net 4 status flags
artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, universe_addr);
artnet_set_net_addr(node, net);                     // net 0-127
artnet_set_subnet_addr(node, subnet);               // subnet 0-15
uint16_t addr = artnet_get_universe_addr(node, 0, ARTNET_OUTPUT_PORT);  // 15-bit
```

### DMX Transmit & Receive

```c
artnet_set_dmx_handler(node, my_dmx_callback, NULL); // register DMX RX callback
artnet_send_dmx(node, port_id, 512, dmx_data);       // send DMX
artnet_raw_send_dmx(node, uni_addr, 512, dmx_data);  // send to any 15-bit address
artnet_send_nzs(node, port_id, start_code, len, data); // non-zero start code
```

### Node Discovery

```c
artnet_send_poll(node, NULL, ARTNET_TTM_AUTO);       // broadcast poll
artnet_node_list nl = artnet_get_nl(node);           // get node list
artnet_nl_first(nl);                                  // iterate nodes
artnet_nl_next(nl);
artnet_nl_get_length(nl);                            // node count
```

### RDM / TOD

```c
artnet_send_rdm(node, address, rdm_data, length);
artnet_send_tod_request(node);                       // request TOD
artnet_send_tod_control(node, address, ARTNET_TOD_FULL);
artnet_add_rdm_device(node, port, uid);
```

### TimeCode, Trigger, Sync, Diagnostics

```c
artnet_send_timecode(node, frames, sec, min, hour, ARTNET_TC_FILM, 0);
artnet_send_timesync(node, tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year);
artnet_send_trigger(node, oem_hi, oem_lo, key, sub_key, data, len);
artnet_send_sync(node);                               // synchronize DMX output
artnet_send_diagnostic(node, ARTNET_DIAG_LOW, port, "message");
```

### Configuration & Utilities

```c
artnet_set_short_name(node, "My Node");
artnet_set_long_name(node, "Long Description");
artnet_setoem(node, hi, lo);
artnet_setesta(node, hi, lo);
artnet_set_bcast_limit(node, 50);
artnet_set_default_resp_uid(node, uid);
artnet_dump_config(node);                            // print config to stdout
artnet_get_config(node, &config);                    // export config struct
artnet_strerror();                                   // last error string
```

### Callbacks

Register callbacks to handle incoming packets:

```c
// Generic handler for any packet type
artnet_set_handler(node, ARTNET_DMX_HANDLER, my_callback, user_data);

// Convenience helpers for common callbacks
artnet_set_dmx_handler(node, my_dmx_callback, NULL);
artnet_set_rdm_handler(node, my_rdm_callback, NULL);
artnet_set_rdm_initiate_handler(node, my_rdm_init_callback, NULL);
artnet_set_rdm_tod_handler(node, my_tod_callback, NULL);
artnet_set_firmware_handler(node, my_fw_callback, NULL);
artnet_set_program_handler(node, my_prog_callback, NULL);
```

All 28 handler types are available via `artnet_set_handler()`: `ARTNET_RECV_HANDLER`, `ARTNET_POLL_HANDLER`, `ARTNET_REPLY_HANDLER`, `ARTNET_SYNC_HANDLER`, `ARTNET_TIMECODE_HANDLER`, `ARTNET_TRIGGER_HANDLER`, `ARTNET_DATAREQUEST_HANDLER`, `ARTNET_DATAREPLY_HANDLER`, and more. See `artnet.h` for the complete list.

## License

LGPL-2.1 - see [COPYING](COPYING) for details.
