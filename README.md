# libartnet

A C library for the [Art-Net 4](https://art-net.org.uk/) protocol, implementing DMX512 data distribution over UDP/IP networks.

Based on [OpenLightingProject/libartnet](https://github.com/OpenLightingProject/libartnet), upgraded to Art-Net 4 compliance with full 15-bit port addressing support.

Art-Net 4 Protocol Specification: [art-net4.md](art-net4.md)

## Features

- Art-Net 4 protocol implementation with 15-bit port addressing (32768 universes)
- Node and Controller modes with up to 4 ports per node
- Node joining for multi-node configurations (8+ universes)
- Built-in protocol behaviors for DMX512 transport, merge, keepalive, fail-safe, ArtSync, discovery, remote programming, TOD/RDM exchange, firmware upload, directory reply, and ArtIpProg query/program handling
- Generic packet transport support plus handler callbacks for ArtCommand, ArtTimeCode, ArtTimeSync, ArtTrigger, ArtDiagData, ArtDataRequest/Reply, file transfer packets, media packets, and other Art-Net 4 opcodes
- Strict inbound packet validation for protocol version, minimum packet length, key field ranges, and variable-length payload consistency
- Unified millisecond-precision monotonic clock (GetTickCount64 / clock_gettime)
- IPv4 and IPv6 support
- Cross-platform: Linux, macOS, Windows

## Capability Model

The library exposes two classes of Art-Net functionality:

- Built-in protocol behavior: libartnet maintains state machines and default protocol actions for `ArtPoll` / `ArtPollReply`, `ArtDmx` / `ArtNzs`, `ArtSync`, `ArtAddress`, `ArtInput`, `ArtIpProg` query/program handling, `ArtTodRequest` / `ArtTodControl` / `ArtTodData`, `ArtRdm`, `ArtRdmSub`, firmware upload, directory reply, node list maintenance, merge, keepalive, and fail-safe handling.
- Packet transport plus callbacks: libartnet also parses, validates, and dispatches many other opcodes to application handlers. For these packets, the library provides wire-format support and callback delivery, but application-specific behavior remains the responsibility of the embedding program. This includes `ArtCommand`, `ArtTimeCode`, `ArtTimeSync`, `ArtTrigger`, `ArtDiagData`, `ArtDataRequest`, `ArtDataReply`, `ArtFileFnMaster`, `ArtFileFnReply`, `ArtMedia`, `ArtMediaPatch`, and `ArtMediaControl` / `ArtMediaControlReply`. `ArtMedia` remains receive-only in the public API.

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
| `BUILD_EXAMPLES_MINIMAL` | `ON` | Build minimal single-purpose examples (`dmx_*`, `timecode_*`, `timesync_tx`, `diag_monitor`, `target_node`) |
| `BUILD_EXAMPLES_FOCUSED` | `OFF` | Build focused workflow examples (`node_manager`, `rdm_controller`, `file_transfer`, `directory_query`) |
| `BUILD_EXAMPLES_FULL` | `ON` | Build integration examples (`full_node`, `full_controller`) |
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

The example set is intentionally layered:

- Minimal examples: the smallest possible programs for one protocol topic.
- Focused workflow examples: interactive tools centered on one operational task.
- Integration examples: broader controller/node programs that exercise many protocol paths together.

You can selectively build those layers with:

```bash
cmake -B build \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_EXAMPLES_MINIMAL=ON \
  -DBUILD_EXAMPLES_FOCUSED=OFF \
  -DBUILD_EXAMPLES_FULL=ON
```

The default configuration now builds the minimal and integration layers, while focused workflow examples are opt-in.

Thirteen example programs are included.

### Minimal Examples

These are the smallest entry points for one protocol topic. Start here if you want the least context and the quickest manual verification loop.

| Example | Purpose |
|--------|---------|
| `dmx_tx` | Send ArtDmx, ArtNzs, and ArtSync across up to 4 universes |
| `dmx_rx` | Receive DMX and observe ArtSync frame boundaries |
| `timecode_tx` | Send SMPTE/EBU ArtTimeCode |
| `timecode_rx` | Receive and print ArtTimeCode |
| `timesync_tx` | Send ArtTimeSync from the local system clock |
| `diag_monitor` | Passively print ArtDiagData, ArtTimeSync, ArtTrigger, and ArtCommand traffic |
| `target_node` | Small remotely manageable DMX node for controller-side examples |

Quick start commands:

```bash
./build/examples/dmx_tx/dmx_tx
./build/examples/dmx_rx/dmx_rx
./build/examples/timecode_tx/timecode_tx
./build/examples/timecode_rx/timecode_rx
./build/examples/timesync_tx/timesync_tx
./build/examples/diag_monitor/diag_monitor -i 192.168.1.11
./build/examples/target_node/target_node -i 192.168.1.20
```

Notes:

- `dmx_tx` supports standard DMX, ArtNzs, and raw 15-bit universe addressing.
- `dmx_rx` is the smallest way to observe ArtSync-coordinated receive behavior.
- `target_node` is a convenient peer for `node_manager` and `full_controller`.

### Focused Workflow Examples

These examples are narrower interactive tools for one operational task. They overlap with the integration examples, but expose a smaller command surface.

| Example | Purpose |
|--------|---------|
| `node_manager` | Remote programming, port enable/disable, LED, failsafe, and ArtIpProg |
| `rdm_controller` | TOD discovery, TOD control, and raw RDM commands |
| `file_transfer` | ArtFileTnMaster upload and ArtFileFnMaster download |
| `directory_query` | ArtDirectory / ArtDirectoryReply workflow |

Quick start commands:

```bash
./build/examples/node_manager/node_manager -i 192.168.1.100
./build/examples/rdm_controller/rdm_controller -i 192.168.1.100
./build/examples/file_transfer/file_transfer -i 192.168.1.100
./build/examples/directory_query/directory_query -i 192.168.1.100
```

Typical `node_manager` flow:

```text
1. Start `target_node`.
2. Start `node_manager`.
3. Use `p` then `l` to discover nodes.
4. Use `1`-`9`, `0`, `f`-`j`, `i` for remote management.
```

### Integration Examples

These are the broadest examples in the repository. They intentionally overlap with the focused examples and are best suited for end-to-end protocol exercise, interoperability checks, and manual regression passes.

#### Full Node (`full_node`)

A test-oriented Art-Net 4 bidirectional node with 4 input + 4 output ports. It supports DMX receive/transmit, TOD/RDM, ArtAddress, ArtInput, ArtIpProg, ArtTimeCode, ArtTimeSync, ArtTrigger, ArtCommand, ArtNzs receive, firmware upload reception, diagnostics, ArtMediaPatch / ArtMediaControl monitoring, ArtDataRequest replies, ArtDirectory replies, and in-memory ArtFileFnMaster responses.

```bash
./build/examples/full_node/full_node -i 192.168.1.20
./build/examples/full_node/full_node -i 192.168.1.20 -n 1 -s 2 -u 3
```

Notes:

- The example exposes an in-memory directory and file table for controller-side directory and file-download testing.
- `ArtDataRequest` replies are minimal test payloads intended for protocol validation, not product metadata completeness.
- File download replies are generated from memory, not a persistent filesystem.

#### Full Controller (`full_controller`)

Interactive controller demonstrating most controller-side features exposed by the public API. It covers discovery, DMX transmission, ArtSync, remote management, TOD/RDM, firmware upload, file transfer, TimeCode/TimeSync, triggers, diagnostics, ArtDataRequest, ArtIpProg, ArtCommand, and ArtMedia* workflows.

```bash
./build/examples/full_controller/full_controller -i 192.168.1.100
```

Recommended controller-to-node workflow:

```text
1. Start `full_node` on a reachable Art-Net IP.
2. Start `full_controller` on the same subnet.
3. Press `p` then `l` to discover and inspect the node.
4. Use `d` / `D` / `s` to verify DMX and ArtSync behavior.
5. Use `1`-`9`, `0`, `f`-`j` to verify remote programming and failsafe commands.
6. Use `t`, `T`, `r`, `R` to validate TOD/RDM paths.
7. Use `x` to query the node's directory, then `v` to download one of the advertised files.
8. Use `b` to query the node's ArtDataRequest responses.
9. Use `i`, `m`, `M`, `C`, `V` to validate ArtIpProg, ArtCommand, and ArtMedia* paths.
10. Use `w` / `u` to exercise firmware and file upload receive paths on the node.
```

Command groups:

- Discovery: `p`, `P`, `l`
- DMX and sync: `d`, `D`, `z`, `s`
- Remote management: `1`-`9`, `0`, `f`-`j`, `i`
- RDM: `t`, `T`, `r`, `R`
- Time and trigger: `c`, `y`, `k`
- Firmware and file: `w`, `u`, `v`, `x`
- Diagnostics and metadata: `a`, `b`, `m`, `M`, `C`, `V`

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
artnet_send_poll_flags(node, NULL,
                       ARTNET_POLL_FLAG_REPLY_ON_CHANGE |
                       ARTNET_POLL_FLAG_DIAG_ENABLE,
                       ARTNET_DIAG_MEDIUM,
                       0, 0, 0x1122, 0x3344);        // explicit Art-Net 4 flags
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

// ArtIpProg: query or program node IP settings
artnet_send_ipprog(node, entry, 0x00, NULL, NULL, NULL);  // query only
artnet_send_ipprog(node, entry, 0x86, "192.168.1.50", "255.255.255.0", NULL);
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

`artnet_send_address()` accepts `NULL` for `shortName`, `longName`, `inAddr`, and `outAddr` to mean "no change".

### Directory & Data

```c
artnet_send_directory(node);                          // query file listings
artnet_send_data_request(node, ip, request_code);    // query node metadata / URLs
artnet_send_directory_reply(node, entries, count, total);
artnet_send_data_reply(node, ip, request_code, payload, length);
artnet_send_command(node, 0xFFFF, "SwoutText=Node\0", 15, NULL);
artnet_send_media_patch(node, entry, physical, universe, patch_data, patch_len);
artnet_send_media_control(node, entry, control_data, control_len);
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

All 32 handler types are available via `artnet_set_handler()`: `ARTNET_RECV_HANDLER`, `ARTNET_SEND_HANDLER`, `ARTNET_POLL_HANDLER`, `ARTNET_REPLY_HANDLER`, `ARTNET_DMX_HANDLER`, `ARTNET_ADDRESS_HANDLER`, `ARTNET_INPUT_HANDLER`, `ARTNET_SYNC_HANDLER`, `ARTNET_NZS_HANDLER`, `ARTNET_TOD_REQUEST_HANDLER`, `ARTNET_TOD_DATA_HANDLER`, `ARTNET_TOD_CONTROL_HANDLER`, `ARTNET_RDM_HANDLER`, `ARTNET_IPPROG_HANDLER`, `ARTNET_FIRMWARE_HANDLER`, `ARTNET_FIRMWARE_REPLY_HANDLER`, `ARTNET_DIAGDATA_HANDLER`, `ARTNET_COMMAND_HANDLER`, `ARTNET_TIMECODE_HANDLER`, `ARTNET_TIMESYNC_HANDLER`, `ARTNET_TRIGGER_HANDLER`, `ARTNET_DIRECTORY_HANDLER`, `ARTNET_DIRECTORY_REPLY_HANDLER`, `ARTNET_FILE_TN_MASTER_HANDLER`, `ARTNET_FILE_FN_MASTER_HANDLER`, `ARTNET_FILE_FN_REPLY_HANDLER`, `ARTNET_MEDIAPATCH_HANDLER`, `ARTNET_MEDIA_HANDLER`, `ARTNET_MEDIACONTROL_HANDLER`, `ARTNET_MEDIACONTROL_REPLY_HANDLER`, `ARTNET_DATAREQUEST_HANDLER`, `ARTNET_DATAREPLY_HANDLER`. See `artnet.h` for the complete list.

## License

LGPL-2.1 - see [COPYING](COPYING) for details.
