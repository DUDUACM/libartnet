# Changelog

All notable changes to this project are documented in this file.

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
- ArtPollReply random delay (0-4 seconds) in ARTNET_REPLY_AUTO mode to prevent packet storms

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
