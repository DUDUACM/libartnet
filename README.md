# libartnet

A C library for the [Art-Net 4](https://art-net.org.uk/) protocol, implementing DMX512 data distribution over UDP/IP networks.

Based on [OpenLightingProject/libartnet](https://github.com/OpenLightingProject/libartnet), upgraded to Art-Net 4 compliance with full 15-bit port addressing support.

Art-Net 4 Protocol Specification: [art-net4.md](art-net4.md)

## Features

- Art-Net 4 protocol implementation with 15-bit port addressing (32768 universes)
- Node and Controller modes with up to 4 ports per node
- Node joining for multi-node configurations (8+ universes)
- Built-in protocol behaviors for DMX512 transport, merge, keepalive, fail-safe, ArtSync, discovery, remote programming, TOD/RDM exchange, firmware upload, directory reply, and IP programming
- Generic packet send/receive support plus handler callbacks for ArtCommand, ArtTimeCode, ArtTimeSync, ArtTrigger, ArtDiagData, ArtDataRequest/Reply, file transfer packets, media packets, and other Art-Net 4 opcodes
- Strict inbound packet validation for protocol version, minimum packet length, key field ranges, and variable-length payload consistency
- Unified millisecond-precision monotonic clock (GetTickCount64 / clock_gettime)
- IPv4 and IPv6 support
- Cross-platform: Linux, macOS, Windows

## Capability Model

The library exposes two classes of Art-Net functionality:

- Built-in protocol behavior: libartnet maintains state machines and default protocol actions for `ArtPoll` / `ArtPollReply`, `ArtDmx` / `ArtNzs`, `ArtSync`, `ArtAddress`, `ArtInput`, `ArtIpProg`, `ArtTodRequest` / `ArtTodControl` / `ArtTodData`, `ArtRdm`, `ArtRdmSub`, firmware upload, directory reply, node list maintenance, merge, keepalive, and fail-safe handling.
- Packet transport plus callbacks: libartnet also parses, validates, and dispatches many other opcodes to application handlers. For these packets, the library provides wire-format support and callback delivery, but application-specific behavior remains the responsibility of the embedding program. This includes `ArtCommand`, `ArtTimeCode`, `ArtTimeSync`, `ArtTrigger`, `ArtDiagData`, `ArtDataRequest`, `ArtDataReply`, `ArtFileFnMaster`, `ArtFileFnReply`, `ArtMedia`, `ArtMediaPatch`, and `ArtMediaControl` / `ArtMediaControlReply`.

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
| `BUILD_TESTS` | `OFF` | Build protocol regression tests and enable `ctest` |
| `BUILD_WERROR` | `OFF` | Treat compiler warnings as errors |

### Static Library

```bash
cmake .. -DBUILD_SHARED_LIBS=OFF
```

## Testing

Build and run the protocol regression suite with:

```bash
cmake -B build-test -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF
cmake --build build-test --config Release
ctest --test-dir build-test --output-on-failure
```

The regression suite uses a stubbed network backend to validate protocol behavior without requiring a live Art-Net network. Current coverage includes:

- `ArtPoll` / `ArtPollReply`
- `ArtAddress` / `ArtInput`
- `ArtDmx` / `ArtNzs` / `ArtSync`
- `ArtIpProg` / `ArtIpProgReply`
- `ArtDataRequest` / `ArtDataReply`
- `ArtDirectory` / `ArtDirectoryReply`
- `ArtFileFnMaster` / `ArtFileFnReply`
- `ArtTodRequest` / `ArtTodControl` / `ArtTodData`
- `ArtRdm` / `ArtRdmSub`
- `ArtCommand` / `ArtTimeCode` / `ArtTimeSync` / `ArtTrigger` / `ArtDiagData`
- Firmware upload / reply state machine
- Node list update and timeout cleanup
- Strict inbound packet validation for truncated packets, invalid protocol versions, invalid field ranges, and malformed variable-length payloads
- Fail-safe, merge, keepalive, sync buffering, and diagnostic controller edge cases

CI runs this suite as a required gate before packaging and release on both GitHub Actions and GitLab CI.

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

## Documentation

Build API documentation with Doxygen:

```bash
doxygen Doxyfile
```

Output is generated in `docs/html/`. Open `docs/html/index.html` to browse.

## Examples

Thirteen example programs are included:

### DMX Transmitter (`dmx_tx`)

Sends a sine wave chase on 4 universes (single node, 40 FPS) with ArtSync. Supports standard DMX (ArtDmx), non-zero start code (ArtNzs), and raw 15-bit universe addressing.

```bash
# Standard DMX on universes 0-3
./build/examples/dmx_tx/dmx_tx

# ArtNzs with start code 0xCF
./build/examples/dmx_tx/dmx_tx -z 0xCF

# Raw 15-bit universe addressing (net=1, subnet=2, universe=3)
./build/examples/dmx_tx/dmx_tx -n 1 -s 2 -u 3 -r
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |
| `-n <net>` | Net address 0-127 (default: 0) |
| `-s <subnet>` | Subnet address 0-15 (default: 0) |
| `-u <universe>` | Starting port address 0-15 (default: 0) |
| `-c <channels>` | DMX channels per universe 1-512 (default: 512) |
| `-z <code>` | Non-zero start code for ArtNzs (default: 0 = ArtDmx) |
| `-r` | Use raw 15-bit universe addressing |

### DMX Receiver (`dmx_rx`)

Receives DMX on 4 universes and prints first/last channel values. Registers an ArtSync handler that prints `[Sync] frame complete` when a full frame is received.

```bash
# Start receiver (must match transmitter's net/subnet/universe)
./build/examples/dmx_rx/dmx_rx

# Bind to specific IP, start at universe 1
./build/examples/dmx_rx/dmx_rx -i 192.168.1.11 -n 0 -s 0 -u 1
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |
| `-n <net>` | Net address 0-127 (default: 0) |
| `-s <subnet>` | Subnet address 0-15 (default: 0) |
| `-u <universe>` | Starting port address 0-15 (default: 0) |

### Target Node (`target_node`)

A DMX receiver node that supports remote management via ArtAddress and ArtInput. When a controller remotely changes its configuration (name, address, port), it prints the updated configuration via the program handler callback.

```bash
# Start target node on a specific IP
./build/examples/target_node/target_node -i 192.168.1.20

# Start with custom address
./build/examples/target_node/target_node -i 192.168.1.20 -n 1 -s 2 -u 3
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |
| `-n <net>` | Net address 0-127 (default: 0) |
| `-s <subnet>` | Subnet address 0-15 (default: 0) |
| `-u <universe>` | Starting port address 0-15 (default: 0) |

### Full Node (`full_node`)

A test-oriented Art-Net 4 bidirectional node with 4 input + 4 output ports. Supports DMX receive/transmit, RDM device discovery (TOD), remote programming via ArtAddress/ArtInput, ArtSync, ArtTimeCode, ArtTimeSync, ArtTrigger, ArtNzs receive, firmware upload reception, diagnostics, `ArtDataRequest` replies, `ArtDirectory` replies, and in-memory file download responses for `ArtFileFnMaster`. Sends ArtPollReply on condition change.

```bash
./build/examples/full_node/full_node -i 192.168.1.20

# Custom address range
./build/examples/full_node/full_node -i 192.168.1.20 -n 1 -s 2 -u 3
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |
| `-n <net>` | Net address 0-127 (default: 0) |
| `-s <subnet>` | Subnet address 0-15 (default: 0) |
| `-u <universe>` | Starting port address 0-15 (default: 0) |

Test-node notes:

- The example exposes an in-memory directory and file table for controller-side directory and file-download testing.
- `ArtDataRequest` replies are minimal test payloads intended for protocol validation, not product metadata completeness.
- File download replies are generated from memory, not a persistent filesystem.

### Node Manager (`node_manager`)

Interactive controller for remote node management. Discovers nodes via ArtPoll and provides a command-line menu to change names, addresses, port states, LED indicators, and failsafe modes.

```bash
# Start manager
./build/examples/node_manager/node_manager -i 192.168.1.100
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |

Interactive commands:

| Key | Action |
|-----|--------|
| `p` | Send ArtPoll to discover nodes |
| `l` | List discovered nodes (IP, name, address, ports) |
| `1` | Change node short name (ArtAddress) |
| `2` | Change node long name (ArtAddress) |
| `3` | Change node net address 0-127 (ArtAddress) |
| `4` | Change node subnet address 0-15 (ArtAddress) |
| `5` | Change a specific port's universe 0-15 (ArtAddress) |
| `6` | Enable a port (ArtInput) |
| `7` | Disable a port (ArtInput) |
| `8` | LED Locate — rapid flash for identification |
| `9` | LED Mute — turn off LEDs |
| `0` | LED Normal — restore normal LED behavior |
| `f` | Failsafe: Hold last state on data loss |
| `g` | Failsafe: Zero all outputs on data loss |
| `h` | Failsafe: Full output on data loss |
| `j` | Failsafe: Play recorded scene on data loss |
| `q` | Quit |

Example workflow:
```
# Terminal 1: start a target node
./build/examples/target_node/target_node -i 192.168.1.20

# Terminal 2: start manager, discover and reconfigure
./build/examples/node_manager/node_manager -i 192.168.1.100
> p                    # discover nodes
> l                    # list found nodes
> 1                    # change short name -> select node -> enter name
> 3                    # change net -> select node -> enter new net value
> 8                    # LED locate -> select node -> node flashes LEDs
```

### Full Controller (`full_controller`)

Interactive controller demonstrating all Art-Net 4 controller features. Provides a comprehensive menu covering node discovery, DMX transmission (ArtDmx/ArtNzs/raw 15-bit), ArtSync, remote management (ArtAddress/ArtInput), RDM (ArtTodRequest/ArtTodControl/ArtRdm/ArtRdmSub), firmware upload, file transfer, TimeCode/TimeSync, triggers, diagnostics, directory queries, and ArtDataRequest queries. The event loop listens to both the Art-Net socket and stdin so interactive input remains responsive while network traffic is active.

```bash
./build/examples/full_controller/full_controller -i 192.168.1.100
```

Interactive commands:

| Key | Action |
|-----|--------|
| `p` | Send ArtPoll to discover nodes |
| `l` | List discovered nodes |
| `d` | Send DMX to a universe (ArtDmx) |
| `D` | Flood DMX continuously at 40 FPS |
| `z` | Send ArtNzs (non-zero start code) |
| `s` | Send ArtSync |
| `1` | Change short name (ArtAddress) |
| `2` | Change long name (ArtAddress) |
| `3` | Change net address (ArtAddress) |
| `4` | Change subnet address (ArtAddress) |
| `5` | Change merge mode (ArtAddress) |
| `6` | Enable/disable ports (ArtInput) |
| `7` | Send ArtAddress with command |
| `8` | LED Locate |
| `9` | LED Mute |
| `0` | LED Normal |
| `f` | Failsafe: Hold last state |
| `g` | Failsafe: Zero all outputs |
| `h` | Failsafe: Full output |
| `j` | Failsafe: Play recorded scene |
| `t` | Send ArtTodRequest (discover RDM devices) |
| `T` | Send ArtTodControl (flush/end TOD) |
| `r` | Send ArtRdm command |
| `R` | Send ArtRdmSub (RDM sub-device) |
| `c` | Send ArtTimeCode |
| `y` | Send ArtTimeSync |
| `k` | Send ArtTrigger |
| `w` | Upload firmware (ArtFirmwareMaster) |
| `u` | Upload file (ArtFileTnMaster) |
| `v` | Download file (ArtFileFnMaster) |
| `x` | Send ArtDirectory query |
| `b` | Send ArtDataRequest |
| `q` | Quit |

Recommended controller-to-node workflow:

```text
1. Start `full_node` on a reachable Art-Net IP.
2. Start `full_controller` on the same subnet.
3. Press `p` then `l` to discover and inspect the node.
4. Use `d` / `D` / `s` to verify DMX and ArtSync behavior.
5. Use `1`-`9`, `0`, `f`-`j` to verify remote programming and failsafe commands.
6. Use `t`, `T`, `r`, `R` to validate TOD/RDM paths.
7. Use `x` to query the node's directory, then `v` to download one of the advertised files.
8. Use `b` to query the node's ArtDataRequest responses (product URL, user guide URL, support URL).
9. Use `w` / `u` to exercise firmware and file upload receive paths on the node.
```

Controller-to-node checklist:

| Step | Controller Action | Expected Node / Controller Result |
|------|-------------------|-----------------------------------|
| 1 | Start `full_node`, then `full_controller`, press `p` | Controller prints at least one `Reply` entry for `FullNode` |
| 2 | Press `l` | Controller shows node IP, short name, Net/Sub, and port list |
| 3 | Press `d` and send a small DMX frame | Node prints `[DMX] Port ...` with channel values |
| 4 | Press `D` to start flood, then `s` if needed | Node shows regular DMX updates; controller remains interactive |
| 5 | Press `1` or `2` to change names | Node prints `Remote Programming Applied` and updated config |
| 6 | Press `3` / `4` / `5` / `6` / `7` / `8` / `9` / `0` | Node prints `Address` / `Input` updates and refreshed configuration |
| 7 | Press `t` | Controller receives `TodData`; node prints `ArtTodRequest received` |
| 8 | Press `r` or `R` | Node prints `RDM` activity for the requested address or UID |
| 9 | Press `x` | Controller prints `DirectoryReply` and lists `full_node.log`, `scene_A.dmx`, `readme.txt` |
| 10 | Press `v` and request one of the listed files | Controller prints one or more `FileFnReply` blocks and then the reconstructed file payload |
| 11 | Press `b` and request codes `1`, `2`, `3` | Controller prints `DataReply` payloads for product URL, user guide URL, and support URL |
| 12 | Press `w` or `u` | Node prints firmware/file upload activity and controller sees upload acknowledgements where applicable |

Notes:

- `full_node` is a protocol test node, not a production device. Directory contents, files, and `ArtDataRequest` replies are in-memory test fixtures.
- `full_controller` now multiplexes stdin and the Art-Net socket, so it should remain responsive while replies are arriving.

### TimeCode Transmitter (`timecode_tx`)

Sends SMPTE/EBU timecode frames at the selected frame rate (Film 24fps, EBU 25fps, DF 29.97fps, SMPTE 30fps). Timecode auto-increments from 00:00:00:00.

```bash
# Send EBU 25fps timecode (default)
./build/examples/timecode_tx/timecode_tx

# Send SMPTE 30fps on a specific IP
./build/examples/timecode_tx/timecode_tx -i 192.168.1.10 -t 3

# Send Film 24fps with custom stream ID
./build/examples/timecode_tx/timecode_tx -t 0 -s 1
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |
| `-t <type>` | TimeCode type: 0=Film(24fps), 1=EBU(25fps), 2=DF(29.97fps), 3=SMPTE(30fps) |
| `-s <stream>` | Stream ID 0-255 (default: 0 = master) |

### TimeCode Receiver (`timecode_rx`)

Listens for ArtTimeCode packets and prints the received timecode (HH:MM:SS:FF), type, and stream ID.

```bash
# Start receiver
./build/examples/timecode_rx/timecode_rx

# Bind to specific IP
./build/examples/timecode_rx/timecode_rx -i 192.168.1.11
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |

### RDM Controller (`rdm_controller`)

Interactive RDM controller that discovers RDM devices via ArtTodRequest, displays the Table of Devices (TOD), flushes TOD, and sends raw RDM commands. Prints TOD data and RDM responses as they arrive.

```bash
./build/examples/rdm_controller/rdm_controller -i 192.168.1.100
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |

Interactive commands:

| Key | Action |
|-----|--------|
| `p` | Send ArtPoll to discover nodes |
| `t` | Send ArtTodRequest to discover RDM devices |
| `f` | Flush TOD on all nodes (ArtTodControl) |
| `r` | Send raw RDM command to a device |
| `q` | Quit |

### TimeSync Transmitter (`timesync_tx`)

Sends ArtTimeSync packets with the current system date/time at a configurable interval. Useful for synchronizing clocks across Art-Net nodes.

`ArtTimeSync` arguments follow `struct tm` semantics: `tm_mon` is `0-11` and `tm_year` is years since 1900.

```bash
# Send every 1 second (default)
./build/examples/timesync_tx/timesync_tx

# Send every 5 seconds
./build/examples/timesync_tx/timesync_tx -i 192.168.1.10 -r 5000
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |
| `-r <ms>` | Send interval in milliseconds (default: 1000, range: 100-60000) |

### Diagnostic Monitor (`diag_monitor`)

Listens for and prints ArtDiagData, ArtTimeSync, ArtTrigger, and ArtCommand packets. A passive network monitor useful for debugging and observing Art-Net traffic.

```bash
./build/examples/diag_monitor/diag_monitor -i 192.168.1.11
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |

Output format:

| Prefix | Packet Type | Information Displayed |
|--------|-------------|-----------------------|
| `[Diag]` | ArtDiagData | Priority, port, message text |
| `[TimeSync]` | ArtTimeSync | Date and time (YYYY-MM-DD HH:MM:SS) |
| `[Trigger]` | ArtTrigger | OEM code, key type, sub-key |
| `[Command]` | ArtCommand | ESTA manufacturer code, text |

### File Transfer (`file_transfer`)

Interactive controller for uploading and downloading files to/from Art-Net nodes via ArtFileTnMaster and ArtFileFnMaster. Displays ArtFileFnReply data blocks as they arrive.

```bash
./build/examples/file_transfer/file_transfer -i 192.168.1.100
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |

Interactive commands:

| Key | Action |
|-----|--------|
| `p` | Send ArtPoll to discover nodes |
| `l` | List discovered nodes |
| `u` | Upload file data block to a node (ArtFileTnMaster) |
| `d` | Download file from a node (ArtFileFnMaster) |
| `q` | Quit |

### Directory Query (`directory_query`)

Interactive controller that sends ArtDirectory to query node file lists and displays ArtDirectoryReply responses.

```bash
./build/examples/directory_query/directory_query -i 192.168.1.100
```

| Option | Description |
|--------|-------------|
| `-i <ip>` | IP address to bind (default: auto-detect) |

Interactive commands:

| Key | Action |
|-----|--------|
| `p` | Send ArtPoll to discover nodes |
| `d` | Send ArtDirectory to query file lists |
| `q` | Quit |

## Supported Packet Types

| Packet | Opcode | Direction | Library Behavior |
|--------|--------|-----------|------------------|
| ArtPoll | 0x2000 | TX/RX | Built-in discovery handling and delayed `ArtPollReply` scheduling |
| ArtPollReply | 0x2100 | TX/RX | Built-in reply generation and node list maintenance |
| ArtDiagData | 0x2300 | TX/RX | Validated packet transport and callback dispatch |
| ArtCommand | 0x2400 | TX/RX | Validated packet transport and callback dispatch |
| ArtDataRequest | 0x2700 | TX/RX | Validated packet transport and callback dispatch |
| ArtDataReply | 0x2800 | TX/RX | Validated packet transport and callback dispatch |
| ArtDmx | 0x5000 | TX/RX | Built-in DMX buffering, merge, keepalive, fail-safe, and sync interaction |
| ArtNzs | 0x5100 | TX/RX | Built-in receive state update and transmit support |
| ArtSync | 0x5200 | TX/RX | Built-in sync buffering and flush behavior |
| ArtAddress | 0x6000 | TX/RX | Built-in remote programming and `ArtPollReply` update |
| ArtInput | 0x7000 | TX/RX | Built-in input enable/disable handling and `ArtPollReply` update |
| ArtTodRequest | 0x8000 | TX/RX | Built-in TOD request routing and reply targeting |
| ArtTodData | 0x8100 | TX/RX | Built-in TOD packet generation plus validated callback dispatch |
| ArtTodControl | 0x8200 | TX/RX | Built-in discovery control handling and TOD reply generation |
| ArtRdm | 0x8300 | TX/RX | Built-in reply targeting plus application RDM callback delivery |
| ArtRdmSub | 0x8400 | TX/RX | Built-in reply targeting plus validated callback dispatch |
| ArtMedia | 0x9000 | RX | Validated packet transport and callback dispatch |
| ArtMediaPatch | 0x9100 | TX/RX | Validated packet transport and callback dispatch |
| ArtMediaControl | 0x9200 | TX/RX | Validated packet transport and callback dispatch |
| ArtMediaControlReply | 0x9300 | RX | Validated packet transport and dedicated reply callback dispatch |
| ArtTimeCode | 0x9700 | TX/RX | Validated packet transport and callback dispatch |
| ArtTimeSync | 0x9800 | TX/RX | Validated packet transport and callback dispatch |
| ArtTrigger | 0x9900 | TX/RX | Validated packet transport and callback dispatch |
| ArtDirectory | 0x9A00 | TX/RX | Built-in request handling and empty directory reply generation |
| ArtDirectoryReply | 0x9B00 | TX/RX | Built-in transmit support plus validated callback dispatch |
| ArtFirmwareMaster | 0xF200 | TX/RX | Built-in firmware upload state machine |
| ArtFirmwareReply | 0xF300 | TX/RX | Built-in firmware upload response state machine |
| ArtFileTnMaster | 0xF400 | TX/RX | Built-in receive acknowledgement plus application firmware callback bridge |
| ArtFileFnMaster | 0xF500 | TX/RX | Built-in reply targeting plus callback dispatch |
| ArtFileFnReply | 0xF600 | TX/RX | Built-in transmit support plus validated callback dispatch |
| ArtIpProg | 0xF800 | TX/RX | Built-in IP programming query/program handling |
| ArtIpProgReply | 0xF900 | TX/RX | Built-in reply generation |

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
artnet_set_status3(node, status3_flags);            // fail-safe, RDMnet, LLRP flags
artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, universe_addr);
artnet_set_net_addr(node, net);                     // net 0-127
artnet_set_subnet_addr(node, subnet);               // subnet 0-15
uint16_t addr = artnet_get_universe_addr(node, 0, ARTNET_OUTPUT_PORT);  // 15-bit
```

### DMX Transmit & Receive

```c
artnet_set_dmx_handler(node, my_dmx_callback, NULL);    // register DMX RX callback
artnet_send_dmx(node, port_id, 512, dmx_data);          // send DMX via port
artnet_raw_send_dmx(node, uni_addr, 512, dmx_data);     // send to any 15-bit address
artnet_send_nzs(node, uni_addr, start_code, len, data); // non-zero start code
artnet_read_dmx(node, port_id, &length);                 // read latest DMX data
```

### Node Discovery

```c
artnet_send_poll(node, NULL, ARTNET_TTM_AUTO);       // broadcast poll
artnet_node_list nl = artnet_get_nl(node);           // get node list
artnet_nl_first(nl);                                  // iterate nodes
artnet_nl_next(nl);
artnet_nl_get_length(nl);                            // node count
artnet_nl_foreach(node, my_callback, user_data);     // iterate with callback
```

### Remote Programming

```c
// ArtAddress: change name, address, sACN priority, LED, failsafe, merge mode
artnet_send_address(node, entry, "newName", NULL, inAddr, outAddr,
                    net, sub, ARTNET_PC_NONE, 0xFF);

// ArtInput: enable/disable ports
artnet_send_input(node, entry, settings);
```

### RDM / TOD

```c
artnet_send_tod_request(node);                       // request TOD
artnet_send_tod_control(node, address, ARTNET_TOD_FULL);
artnet_send_rdm(node, address, rdm_data, length);
artnet_send_rdmsub(node, uid, cmd_class, param_id, sub_dev, sub_count, data, len);
artnet_add_rdm_device(node, port, uid);
artnet_add_rdm_devices(node, port, uids, count);
artnet_remove_rdm_device(node, port, uid);
```

### Firmware & File Transfer

```c
// Firmware upload with progress callback
artnet_send_firmware(node, entry, ubea, data, length, progress_cb, NULL);

// File transfer
artnet_send_file_tn_master(node, entry, type, blockId, totalLen, data, dataLen);
artnet_send_file_fn_master(node, entry, "filename");
```

### TimeCode, Trigger, Sync, Diagnostics

```c
artnet_send_timecode(node, frames, sec, min, hour, ARTNET_TIMECODE_FILM, 0);
artnet_send_timesync(node, tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year);
artnet_send_trigger(node, oem_hi, oem_lo, key, sub_key, data, len);
artnet_send_sync(node);                               // synchronize DMX output
artnet_send_diagnostic(node, ARTNET_DIAG_LOW, port, "message");
```

`artnet_send_timesync()` uses `struct tm`-style fields for month and year.

### Directory & Data

```c
artnet_send_directory(node);                          // query file listings
artnet_send_data_request(node, ip, request_code);    // query node metadata / URLs
artnet_send_directory_reply(node, entries, count, total);
artnet_send_data_reply(node, ip, request_code, payload, length);
```

### Configuration & Utilities

```c
artnet_set_short_name(node, "My Node");
artnet_set_long_name(node, "Long Description");
artnet_setoem(node, hi, lo);
artnet_setesta(node, hi, lo);
artnet_set_bcast_limit(node, 50);
artnet_set_default_resp_uid(node, uid);
artnet_set_gateway(node, "192.168.1.1");
artnet_dump_config(node);                            // print config to stdout
artnet_get_config(node, &config);                    // export config struct
artnet_strerror();                                   // last error string
```

### Callbacks

Register callbacks to handle incoming packets. Use these both for passive packet observation and for opcodes where application-specific behavior is expected:

```c
// Generic packet handler for a specific opcode
artnet_set_handler(node, ARTNET_DMX_HANDLER, my_callback, user_data);

// Convenience helpers for common callbacks
artnet_set_dmx_handler(node, my_dmx_callback, NULL);
artnet_set_rdm_handler(node, my_rdm_callback, NULL);
artnet_set_rdm_initiate_handler(node, my_rdm_init_callback, NULL);
artnet_set_rdm_tod_handler(node, my_tod_callback, NULL);
artnet_set_firmware_handler(node, my_fw_callback, NULL);
artnet_set_program_handler(node, my_prog_callback, NULL);
```

All 31 handler types are available via `artnet_set_handler()`: `ARTNET_RECV_HANDLER`, `ARTNET_POLL_HANDLER`, `ARTNET_REPLY_HANDLER`, `ARTNET_DMX_HANDLER`, `ARTNET_ADDRESS_HANDLER`, `ARTNET_INPUT_HANDLER`, `ARTNET_SYNC_HANDLER`, `ARTNET_NZS_HANDLER`, `ARTNET_TOD_REQUEST_HANDLER`, `ARTNET_TOD_DATA_HANDLER`, `ARTNET_TOD_CONTROL_HANDLER`, `ARTNET_RDM_HANDLER`, `ARTNET_IPPROG_HANDLER`, `ARTNET_FIRMWARE_HANDLER`, `ARTNET_FIRMWARE_REPLY_HANDLER`, `ARTNET_DIAGDATA_HANDLER`, `ARTNET_COMMAND_HANDLER`, `ARTNET_TIMECODE_HANDLER`, `ARTNET_TIMESYNC_HANDLER`, `ARTNET_TRIGGER_HANDLER`, `ARTNET_DIRECTORY_HANDLER`, `ARTNET_DIRECTORY_REPLY_HANDLER`, `ARTNET_FILE_TN_MASTER_HANDLER`, `ARTNET_FILE_FN_MASTER_HANDLER`, `ARTNET_FILE_FN_REPLY_HANDLER`, `ARTNET_MEDIAPATCH_HANDLER`, `ARTNET_MEDIA_HANDLER`, `ARTNET_MEDIACONTROL_HANDLER`, `ARTNET_MEDIACONTROL_REPLY_HANDLER`, `ARTNET_DATAREQUEST_HANDLER`, `ARTNET_DATAREPLY_HANDLER`. See `artnet.h` for the complete list.

## License

LGPL-2.1 - see [COPYING](COPYING) for details.
