# Changelog

All notable changes to this project are documented in this file.

## [1.2.0] - 2026-04-28

### Added

- `artnet_send_nzs()` public API with 15-bit universe addressing and start code validation (rejects 0x00/0xCC)
- `acnPriority` parameter in `artnet_send_address()` for sACN priority control (was hardcoded to 0x00)
- `artnet_nl_foreach()` for iterating node list entries with a callback
- `artnet_get_nzs_start_code()` to read the last received NZS start code per port
- `artnet_send_rdmsub()` for compressed RDM sub-device communication
- `artnet_set_status3()` for Art-Net 4 Status3 register (fail-safe, RDMnet, LLRP, port direction)
- `artnet_set_gateway()` for ArtPollReply gateway IP field
- `full_node` example: complete Art-Net 4 bidirectional node (4 input + 4 output ports, RDM, fail-safe, remote programming)
- `full_controller` example: interactive controller demonstrating all Art-Net 4 features (30+ commands)
- Doxygen API documentation with `/** */` comments on all 148 functions across all source files
- GitHub Actions CI/CD pipeline (Windows amd64, Linux amd64/arm64, macOS amd64/arm64)
- Doxyfile configuration: EXTRACT_ALL, MACRO_EXPANSION, PREDEFINED for EXTERN macro
- `time_util.h` / `time_util.c`: unified millisecond-precision monotonic clock module
  - `artnet_gettime_ms()`: GetTickCount64() on Windows, clock_gettime(CLOCK_MONOTONIC) on POSIX
  - `artnet_time_diff_ms()`: compute timestamp difference in milliseconds
  - `artnet_is_timeout()`: check whether a timeout has expired
  - Platform-specific `artnet_mtime_t` type (ULONGLONG on Windows, uint64_t on POSIX)

### Fixed

- Memory leaks and dangling pointers after free in node cleanup paths
- Linux build: enable GNU extensions and fix preprocessor logic
- Link libm only on Linux where it is required (was unconditionally linked)
- Sign-compare warnings in timeout comparisons (int64_t casts for unsigned/signed mixing)
- Multiple Art-Net 4 spec compliance bugs and code quality issues
- Code style: enforce braces on single-statement bodies, fix ternary spacing

### Changed

- Time precision unified from mixed `time_t` (seconds) / `clock_t` (ticks) to single `artnet_mtime_t` millisecond monotonic clock
- All timeout constants converted to millisecond units (DMX_FAILSAFE_TIMEOUT_MS, ARTSYNC_TIMEOUT_MS, etc.)
- All 6 timeout comparison sites now use direct millisecond arithmetic (removed `/1000` and `*1000/CLOCKS_PER_SEC` conversions)
- All internal time fields converted to `artnet_mtime_t`: input_port_t::last_dmx_send_time, output_port_t::timeA/timeB/last_dmx_time, firmware_transfer_t::last_time, node_entry_private_t::last_seen, node_state_t::last_sync_time/apr_pending_time
- Example count from 12 to 13 (added full_node and full_controller)
- Updated README.md with new examples, API sections, and Doxygen documentation instructions

## [1.1.4] - 2026-04-26

### Added

- `artnet_send_directory()` for broadcasting ArtDirectory requests
- `artnet_send_directory_reply()` for sending ArtDirectoryReply with file entries
- `artnet_send_file_tn_master()` for uploading file blocks to nodes (ArtFileTnMaster)
- `artnet_send_file_fn_master()` for requesting file downloads from nodes (ArtFileFnMaster)
- `artnet_send_file_fn_reply()` for responding with file data blocks (ArtFileFnReply)
- 12 example programs covering all 30 Art-Net 4 packet types:
  - `dmx_tx`: DMX transmitter with ArtSync, ArtNzs (`-z`), and raw 15-bit addressing (`-r`)
  - `dmx_rx`: DMX receiver with ArtSync handler
  - `timecode_tx` / `timecode_rx`: SMPTE/EBU timecode send/receive
  - `target_node`: remotely manageable DMX node (ArtAddress, ArtInput)
  - `node_manager`: interactive remote management (names, addresses, ports, LED, failsafe, trigger, IP) *(not yet tested)*
  - `rdm_controller`: RDM discovery (ArtTodRequest), TOD control (ArtTodControl), RDM commands *(not yet tested)*
  - `timesync_tx`: ArtTimeSync with system clock *(not yet tested)*
  - `diag_monitor`: passive monitor for DiagData, TimeSync, Trigger, Command *(not yet tested)*
  - `file_transfer`: file upload/download via ArtFileTnMaster/ArtFileFnMaster *(not yet tested)*
  - `directory_query`: ArtDirectory query with ArtDirectoryReply display *(not yet tested)*

### Fixed

- ArtPollReply debug print now shows both input and output ports (was output only)
- Port direction convention documented: OUTPUT port = receive DMX from network per Art-Net spec

### Changed

- Simplified all examples to 4 universes (single node) instead of 8 universes (2 joined nodes)
- All DMX examples include ArtSync support
- Removed `dmx_srv_controller_tx_rx` example (superseded by dmx_tx, dmx_rx, node_manager)
- Updated README.md with detailed per-example descriptions, options, and usage workflows

## [1.1.3] - 2026-04-26

### Added

- DALI port data type (`ARTNET_PORT_DALI = 0x06`) per Art-Net 4 spec
- `ARTNET_DIAGDATA_HANDLER` callback for ArtDiagData packets
- Dedicated handle functions for all packet types in receive.c:
  `handle_rdm_sub`, `handle_diagdata`, `handle_data_request`,
  `handle_data_reply`, `handle_media`, `handle_media_control_reply`

### Fixed

- ArtDmx and ArtNzs packet comments corrected to "Unicast only" per Art-Net 4 spec
- ArtMedia (0x9000) receive now routes through `handle_media()` callback
- ArtMediaControlReply (0x9300) receive now routes through `handle_media_control_reply()`
- ArtRdmSub (0x8400) receive now routes through `handle_rdm_sub()`
- ArtDiagData (0x2300) receive now routes through `handle_diagdata()`
- ArtDataRequest (0x2700) receive now routes through `handle_data_request()`
- ArtDataReply (0x2800) receive now routes through `handle_data_reply()`
- README.md: corrected multiple wrong opcode values in Supported Packet Types table
- README.md: added missing packet types (ArtRdmSub, ArtMedia, ArtMediaControlReply)

## [1.1.2] - 2026-04-25

### Added

- Art-Net 4 full protocol compliance with 15-bit port addressing (32768 universes)
- Art-Net 4 packet types: ArtSync, ArtNzs, ArtTimeCode, ArtTimeSync, ArtTrigger,
  ArtCommand, ArtDirectory, ArtDirectoryReply, ArtFileTnMaster, ArtFileFnMaster,
  ArtFileFnReply, ArtMediaPatch, ArtMediaControl
- Art-Net 4 ArtPollReply fields: Status2, Status3, GoodOutputB, BindIp, BindIndex,
  DefaultRespUid
- Art-Net 4 ArtPoll flags, diagnostic priority, style codes, node report codes,
  fail-safe modes, LED states
- ArtAddress remote programming support (net, subnet, port addressing)
- Controller mode with automatic ArtPoll on start
- `artnet_raw_send_dmx()` for sending to arbitrary 15-bit universe addresses
- `artnet_send_nzs()` for non-zero start code DMX
- `artnet_send_timecode()`, `artnet_send_timesync()` for time code (with stream_id parameter)
- `artnet_send_data_reply()` for ArtDataReply transmission
- `ARTNET_DATAREQUEST_HANDLER` and `ARTNET_DATAREPLY_HANDLER` dedicated callbacks
- `ARTNET_TOD_END`, `ARTNET_TOD_INC_ON`, `ARTNET_TOD_INC_OFF` TOD control commands
- `artnet_send_trigger()` for trigger messages
- `artnet_send_diagnostic()` for diagnostic messages
- `artnet_set_style_code()`, `artnet_set_status2()` configuration APIs
- `artnet_set_default_resp_uid()` for RDM default responder UID
- `artnet_get_config()` for exporting node configuration
- `artnet_send_poll()` with Art-Net 4 talk-to-me / flags support
- Example programs: DMX node (8-universe) and DMX controller (8-universe)
- CMake build system with shared/static library support, pkg-config, and CMake config
- `compile_commands.json` generation for IDE support (VS Code, Qt Creator)

### Fixed

- macOS interface enumeration: use `getifaddrs()` with `AF_LINK` instead of broken `SIOCGIFCONF`
- Cross-platform stdout buffering in example programs (Qt Creator output visibility)
- 31 Art-Net 4 protocol compliance issues across 7 rounds of fixes:
  - SubSwitch encoding (low nibble per Art-Net 4 spec)
  - NetSwitch 7-bit masking
  - ArtPollReply Status2/Status3/GoodOutputB field encoding
  - ArtDmx universe address (16-bit, not 8-bit)
  - ArtTodRequest/ArtTodData/ArtRdm address byte encoding
  - ArtAddress SubSwitch and ProgramChange bit handling
  - ArtPollReply port address reconstruction on net/subnet changes
  - ArtFirmwareMaster block count
  - Sign-comparison warnings and missing includes
  - Uninitialized local variables
- Compiler warnings with GCC 12.2.0
- All MSVC warnings resolved with `/W4` level (C4244, C4267, C4706, C4245, D9025)
- ArtTodData unicasts to the TOD requester instead of broadcasting (Art-Net 4 requirement)
- ArtTodControl handles all 5 command codes (TOD_FULL/FLUSH/END/INC_ON/INC_OFF)
- Multi-controller diagnostic configuration uses most-restrictive accumulation (min priority)
- ArtPollReply ar_count wraps at 9999 as required by specification
- ArtPollReply random delay corrected to 0-1 second per specification (was 0-4 seconds)
- Firmware transfer timeout corrected to 30 seconds per specification (was 20 seconds)
- ArtTodControl now stores requester IP for unicast ArtTodData replies
- ArtPollReply bgQueuePolicy now reflects programmed value from ArtAddress
- ArtDataReply request_code parameter widened to uint16_t for manufacturer-specific codes
- ArtDmx minimum data length enforced as 2 per specification (was 1)
- ArtNzs handler uses correct struct accessor instead of ArtDmx struct
- ArtPollReply Status3 bit 5 set to indicate programmable failsafe support
- ArtSync no longer blocked globally when a single port is merging
- ArtMedia packet routed to dedicated callback (was incorrectly routed to mediapatch)
- Multi-controller diagnostic broadcast rule: diagnostics now broadcast when multiple controllers request them

### Changed

- Removed autotools build system (configure.ac, Makefile.am)
- Removed MSVC project files (replaced by CMake)
- Removed Debian packaging files
- Updated build to CMake-only
- Library version now derived from project version instead of hardcoded
- `-Werror` is now optional, controlled by `BUILD_WERROR` option (default OFF)
- Added `-Wextra` to default compiler warnings
- Added `BUILD_EXAMPLES` option (default ON) to skip example compilation
- Added `CMAKE_C_EXTENSIONS OFF` for strict C99 compatibility
- Updated README.md with upstream project attribution

## [1.1.1] - 2024-04-22

### Fixed

- ArtFirmwareMaster blockId count

## [1.1.0] - 2025-12-17

### Added

- CMake build system support

### Fixed

- Compiler warning `-Wmemset-elt-size` for memset

## [1.0.5] - 2006-12-24

### Fixed

- Compile warnings on macOS
- OS X interface detection

## [1.0.4] - 2006-08-28

### Fixed

- Various network.c patches

## [1.0.1] - 2006-04-17

### Fixed

- Namespace issues

## [1.0.0] - 2005-08-25

### Added

- Initial release
- Art-Net II support
- DMX512 transmit and receive
- Node discovery (ArtPoll / ArtPollReply)
- RDM over Art-Net
- Firmware upload
- Windows support
