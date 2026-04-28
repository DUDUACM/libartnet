/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * packets.h
 * Datagram definitions for libartnet
 * Copyright (C) 2004-2005 Simon Newton
 */


#ifndef ARTNET_PACKETS_H
#define ARTNET_PACKETS_H

#include <sys/types.h>
#include <stdint.h>

#ifndef WIN32
#include <netinet/in.h>
#endif

#include <artnet/common.h>




#ifdef _MSC_VER
	#define PACKED
	#pragma pack(push,1)
#else
	#define PACKED __attribute__((packed))
#endif


enum { ARTNET_MAX_RDM_ADCOUNT = 32 };

enum { ARTNET_MAX_UID_COUNT = 200 };

// according to the rdm spec, this should be 278 bytes
// we'll set to 512 here, the firmware datagram is still bigger
enum { ARTNET_MAX_RDM_DATA = 512 };

enum { ARTNET_FIRMWARE_SIZE = 512 };

/*
 * Art-Net 4 OpCode definitions (Table 1)
 * OpCodes are transmitted with low byte first
 */
enum artnet_packet_type_e {
  // discovery & configuration
  ARTNET_POLL = 0x2000,              // OpPoll - node discovery / poll
  ARTNET_REPLY = 0x2100,             // OpPollReply - node status response
  ARTNET_DIAGDATA = 0x2300,          // OpDiagData - diagnostic data packet
  ARTNET_COMMAND = 0x2400,           // OpCommand - text-based command
  // data transport
  ARTNET_DMX = 0x5000,               // OpOutput / OpDmx - DMX512 zero start code data
  ARTNET_NZS = 0x5100,               // OpNzs - DMX512 non-zero start code data (excl. RDM)
  ARTNET_SYNC = 0x5200,              // OpSync - synchronize ArtDmx output to nodes
  ARTNET_DATAREQUEST = 0x2700,       // OpDataRequest - request data (e.g. product URLs)
  ARTNET_DATAREPLY = 0x2800,         // OpDataReply - reply to ArtDataRequest
  // remote programming
  ARTNET_ADDRESS = 0x6000,           // OpAddress - remote node programming
  ARTNET_INPUT = 0x7000,             // OpInput - input port enable / disable
  // RDM (Remote Device Management)
  ARTNET_TODREQUEST = 0x8000,        // OpTodRequest - RDM table of devices request
  ARTNET_TODDATA = 0x8100,           // OpTodData - RDM table of devices data
  ARTNET_TODCONTROL = 0x8200,        // OpTodControl - RDM discovery control
  ARTNET_RDM = 0x8300,               // OpRdm - RDM non-discovery messages
  ARTNET_RDMSUB = 0x8400,            // OpRdmSub - compressed RDM sub-device data
  // media server
  ARTNET_MEDIA = 0x9000,             // OpMedia - media server data
  ARTNET_MEDIAPATCH = 0x9100,        // OpMediaPatch - media patch control
  ARTNET_MEDIACONTROL = 0x9200,      // OpMediaControl - media control
  ARTNET_MEDIACONTROLREPLY = 0x9300, // OpMediaContrlReply - media control response
  // time & trigger
  ARTNET_TIMECODE = 0x9700,          // OpTimeCode - SMPTE / EBU timecode transport
  ARTNET_TIMESYNC = 0x9800,          // OpTimeSync - real-time date & clock sync
  ARTNET_TRIGGER = 0x9900,           // OpTrigger - trigger macro
  // directory
  ARTNET_DIRECTORY = 0x9a00,         // OpDirectory - request node file list
  ARTNET_DIRECTORYREPLY = 0x9b00,    // OpDirectoryReply - node file list response
  // video extension (deprecated)
  ARTNET_VIDEOSETUP = 0xa010,        // OpVideoSetup - video screen settings (deprecated)
  ARTNET_VIDEOPALETTE = 0xa020,      // OpVideoPalette - video palette settings (deprecated)
  ARTNET_VIDEODATA = 0xa040,         // OpVideoData - video display data (deprecated)
  // MAC programming (deprecated)
  ARTNET_MACMASTER = 0xf000,         // OpMacMaster - MAC master (deprecated)
  ARTNET_MACSLAVE = 0xf100,          // OpMacSlave - MAC slave (deprecated)
  // firmware upload
  ARTNET_FIRMWAREMASTER = 0xf200,    // OpFirmwareMaster - firmware / UBEA upload
  ARTNET_FIRMWAREREPLY = 0xf300,     // OpFirmwareReply - firmware upload response
  // file transfer
  ARTNET_FILETNMASTER = 0xf400,      // OpFileTnMaster - upload user file to node
  ARTNET_FILEFNMASTER = 0xf500,      // OpFileFnMaster - download user file from node
  ARTNET_FILEFNREPLY = 0xf600,       // OpFileFnReply - file download response
  // IP programming
  ARTNET_IPPROG = 0xf800,            // OpIpProg - IP address / subnet / gateway programming
  ARTNET_IPREPLY = 0xf900,           // OpIpProgReply - IP programming response
} PACKED;

typedef enum artnet_packet_type_e artnet_packet_type_t;

// ---------------------------------------------------------------------------
// ArtPoll (OpCode 0x2000)
// Broadcast by a controller to poll all nodes on the network.
// ---------------------------------------------------------------------------
struct artnet_poll_s {
  uint8_t  id[8];                          // ID: "Art-Net" + 0x00
  uint16_t opCode;                         // OpCode: OpPoll (0x2000), low byte first
  uint8_t  verH;                           // ProtVerHi: protocol version high byte
  uint8_t  ver;                            // ProtVerLo: protocol version low byte (14)
  uint8_t  flags;                          // Flags: bit5=targeted, bit4=VLC disable, bit3=diag unicast, bit2=diag enable, bit1=reply on change, bit0=deprecated
  uint8_t  diagPriority;                   // DiagPriority: lowest diagnostic priority to send
  uint8_t  targetPortAddressTopHi;         // TargetPort Address TopHi: top of Port-Address range (Targeted Mode)
  uint8_t  targetPortAddressTopLo;         // TargetPort Address TopLo
  uint8_t  targetPortAddressBottomHi;      // TargetPort Address BottomHi: bottom of Port-Address range (Targeted Mode)
  uint8_t  targetPortAddressBottomLo;      // TargetPort Address BottomLo
  uint8_t  estaMan[2];                     // EstaManHi/Lo: ESTA manufacturer code (Art-Net 4)
  uint8_t  oem[2];                         // OemHi/Lo: OEM code (Art-Net 4)
} PACKED;

typedef struct artnet_poll_s artnet_poll_t;

// ---------------------------------------------------------------------------
// ArtPollReply (OpCode 0x2100)
// Response to ArtPoll, sent by each node to report its status.
// ---------------------------------------------------------------------------
struct artnet_reply_s {
  uint8_t  id[8];                          // ID: "Art-Net" + 0x00
  uint16_t opCode;                         // OpCode: OpPollReply (0x2100), low byte first
  uint8_t  ip[4];                          // IP Address: node IP address
  uint16_t port;                           // Port: 0x1936, low byte first
  uint8_t  verH;                           // VersInfoH: firmware version high byte
  uint8_t  ver;                            // VersInfoL: firmware version low byte
  uint8_t  netSwitch;                      // NetSwitch: bits 14-8 of 15-bit Port-Address
  uint8_t  subSwitch;                      // SubSwitch: bits 7-4 of 15-bit Port-Address
  uint8_t  oemH;                           // OemHi: OEM code high byte
  uint8_t  oem;                            // Oem: OEM code low byte
  uint8_t  ubea;                           // Ubea Version: UBEA firmware version (0 if not programmed)
  uint8_t  status;                         // Status1: bit7-6=indicator, bit5-4=prog authority, bit2=ROM boot, bit1=RDM, bit0=UBEA
  uint8_t  estaMan[2];                     // EstaManLo/Hi: ESTA manufacturer code (Lo byte first in wire format)
  uint8_t  shortName[ARTNET_SHORT_NAME_LENGTH];  // PortName: node short name (max 17 chars + null)
  uint8_t  longName[ARTNET_LONG_NAME_LENGTH];    // LongName: node long name (max 63 chars + null)
  uint8_t  nodeReport[ARTNET_REPORT_LENGTH];    // NodeReport: "#xxxx [yyyy] text" status report
  uint8_t  numbportsH;                     // NumPortsHi: number of ports high byte (0)
  uint8_t  numbports;                      // NumPortsLo: number of ports low byte (max 4)
  uint8_t  portTypes[ARTNET_MAX_PORTS];    // PortTypes: bit7=output, bit6=input, bit5-0=protocol
  uint8_t  goodInput[ARTNET_MAX_PORTS];    // GoodInput: bit7=data rx, bit6=test, bit5=SIP, bit4=text, bit3=disabled, bit2=errors, bit0=sACN
  uint8_t  goodOutputA[ARTNET_MAX_PORTS];  // GoodOutputA: bit7=DMX output, bit6=test, bit5=SIP, bit4=text, bit3=merging, bit2=short, bit1=LTP, bit0=sACN
  uint8_t  swIn[ARTNET_MAX_PORTS];         // SwIn: input Port-Address bits 3-0
  uint8_t  swOut[ARTNET_MAX_PORTS];        // SwOut: output Port-Address bits 3-0
  uint8_t  acnPriority;                    // AcnPriority: sACN priority for output conversion (0-200)
  uint8_t  swMacro;                        // SwMacro: macro key inputs (bit0=macro1 .. bit7=macro8)
  uint8_t  swRemote;                       // SwRemote: remote trigger inputs (bit0=remote1 .. bit7=remote8)
  uint8_t  sp1;                            // Spare: transmit as zero
  uint8_t  sp2;                            // Spare: transmit as zero
  uint8_t  sp3;                            // Spare: transmit as zero
  uint8_t  style;                          // Style: product style code (Table 4)
  uint8_t  mac[ARTNET_MAC_SIZE];           // MAC Hi..Lo: MAC address of node
  uint8_t  bindIp[ARTNET_IP_SIZE];         // BindIp: root device IP (for bound nodes)
  uint8_t  bindIndex;                      // BindIndex: bind order (0 or 1 = root device)
  uint8_t  status2;                        // Status2: bit7=RDM control, bit6=style switch, bit5=squawking, bit4=sACN, bit3=15-bit, bit2=DHCP cap, bit1=DHCP cfg, bit0=web
  uint8_t  goodOutputB[ARTNET_MAX_PORTS];  // GoodOutputB: bit7=RDM disabled, bit6=constant style, bit5=discovery idle, bit4=bg discovery disabled
  uint8_t  status3;                        // Status3: bit7-6=failsafe, bit5=prog failsafe, bit4=LLRP, bit3=port dir, bit2=RDMnet, bit1=BgQueue, bit0=bg disc ctrl
  uint8_t  defaultRespUid[ARTNET_RDM_UID_WIDTH]; // DefaultRespUID: RDMnet & LLRP default responder UID
  uint8_t  userHi;                         // UserHi: user specific data (Art-Net 4)
  uint8_t  userLo;                         // UserLo: user specific data (Art-Net 4)
  uint8_t  refreshRateHi;                  // RefreshRateHi: max ArtDmx refresh rate in Hz, high byte (Art-Net 4)
  uint8_t  refreshRateLo;                  // RefreshRateLo: max ArtDmx refresh rate in Hz, low byte (0-44 = DMX512)
  uint8_t  bgQueuePolicy;                  // Background QueuePolicy: RDM background discovery policy (Art-Net 4)
  uint8_t  filler[10];                     // Filler: transmit as zero
} PACKED;

typedef struct artnet_reply_s artnet_reply_t;

// ---------------------------------------------------------------------------
// ArtIpProg (OpCode 0xf800)
// Unicast by a controller to a node to program its IP/subnet/gateway.
// ---------------------------------------------------------------------------
struct artnet_ipprog_s {
  uint8_t  id[8];                          // ID: "Art-Net" + 0x00
  uint16_t OpCode;                         // OpCode: OpIpProg (0xf800), low byte first
  uint8_t  ProVerHi;                       // ProtVerHi: protocol version high byte
  uint8_t  ProVerLo;                       // ProtVerLo: protocol version low byte (14)
  uint8_t  Filler1;                        // Filler1: pad to match ArtPoll
  uint8_t  Filler2;                        // Filler2: pad to match ArtPoll
  uint8_t  Command;                        // Command: bit7=enable prog, bit6=DHCP, bit5=unused, bit4=gateway, bit3=reset, bit2=IP, bit1=mask, bit0=port(deprecated)
  uint8_t  Filler4;                        // Filler4: pad for word alignment, set to zero
  uint8_t  ProgIpHi;                       // ProgIpHi: IP address to program (MSB)
  uint8_t  ProgIp2;                        // ProgIp2
  uint8_t  ProgIp1;                        // ProgIp1
  uint8_t  ProgIpLo;                       // ProgIpLo: IP address to program (LSB)
  uint8_t  ProgSmHi;                       // ProgSmHi: subnet mask to program (MSB)
  uint8_t  ProgSm2;                        // ProgSm2
  uint8_t  ProgSm1;                        // ProgSm1
  uint8_t  ProgSmLo;                       // ProgSmLo: subnet mask to program (LSB)
  uint8_t  ProgPortHi;                     // ProgPort Hi: (deprecated)
  uint8_t  ProgPortLo;                     // ProgPort Lo: (deprecated)
  uint8_t  ProgDgHi;                       // ProgDgHi: default gateway to program (MSB)
  uint8_t  ProgDg2;                        // ProgDg2
  uint8_t  ProgDg1;                        // ProgDg1
  uint8_t  ProgDgLo;                       // ProgDgLo: default gateway to program (LSB)
  uint8_t  Spare1;                         // Spare4: transmit as zero
  uint8_t  Spare2;                         // Spare5
  uint8_t  Spare3;                         // Spare6
  uint8_t  Spare4;                         // Spare7
} PACKED;

typedef struct artnet_ipprog_s artnet_ipprog_t;

// ---------------------------------------------------------------------------
// ArtIpProgReply (OpCode 0xf900)
// Response to ArtIpProg, unicast from node back to controller.
// ---------------------------------------------------------------------------
struct artnet_ipprog_reply_s {
  uint8_t id[8];                           // ID: "Art-Net" + 0x00
  uint16_t  OpCode;                        // OpCode: OpIpProgReply (0xf900), low byte first
  uint8_t  ProVerHi;                       // ProtVerHi: protocol version high byte
  uint8_t  ProVerLo;                       // ProtVerLo: protocol version low byte (14)
  uint8_t  Filler1;                        // Filler1: pad to match ArtPoll
  uint8_t  Filler2;                        // Filler2: pad to match ArtPoll
  uint8_t  Filler3;                        // Filler3: pad to match ArtIpProg
  uint8_t  Filler4;                        // Filler4: pad to match ArtIpProg
  uint8_t  ProgIpHi;                       // ProgIpHi: node IP address (MSB)
  uint8_t  ProgIp2;                        // ProgIp2
  uint8_t  ProgIp1;                        // ProgIp1
  uint8_t  ProgIpLo;                       // ProgIpLo: node IP address (LSB)
  uint8_t  ProgSmHi;                       // ProgSmHi: node subnet mask (MSB)
  uint8_t  ProgSm2;                        // ProgSm2
  uint8_t  ProgSm1;                        // ProgSm1
  uint8_t  ProgSmLo;                       // ProgSmLo: node subnet mask (LSB)
  uint8_t  ProgPortHi;                     // ProgPort Hi: (deprecated)
  uint8_t  ProgPortLo;                     // ProgPort Lo: (deprecated)
  uint8_t  Status;                         // Status: bit7=0, bit6=DHCP enabled, bit5-0=0
  uint8_t  Spare2;                         // Spare2: transmit as zero
  uint8_t  ProgDgHi;                       // ProgDgHi: node default gateway (MSB)
  uint8_t  ProgDg2;                        // ProgDg2
  uint8_t  ProgDg1;                        // ProgDg1
  uint8_t  ProgDgLo;                       // ProgDgLo: node default gateway (LSB)
  uint8_t  Spare7;                         // Spare7: transmit as zero
  uint8_t  Spare8;                         // Spare8: transmit as zero
} PACKED;

typedef struct artnet_ipprog_reply_s artnet_ipprog_reply_t;

// ---------------------------------------------------------------------------
// ArtDataRequest (OpCode 0x2700)
// Request data from a node (e.g. product URLs). Unicast.
// ---------------------------------------------------------------------------
struct artnet_data_request_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpDataRequest (0x2700), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  estaManHi;                       // EstaManHi: ESTA manufacturer code high byte
  uint8_t  estaManLo;                       // EstaManLo: ESTA manufacturer code low byte
  uint8_t  oemHi;                           // OemHi: OEM code high byte
  uint8_t  oemLo;                           // OemLo: OEM code low byte
  uint8_t  requestHi;                       // RequestHi: data request code high byte (Table 4a)
  uint8_t  requestLo;                       // RequestLo: data request code low byte
  uint8_t  spare[22];                       // Spare[22]: transmit as zero
} PACKED;

typedef struct artnet_data_request_s artnet_data_request_t;

// ---------------------------------------------------------------------------
// ArtDataReply (OpCode 0x2800)
// Reply to ArtDataRequest. Unicast.
// ---------------------------------------------------------------------------
struct artnet_data_reply_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpDataReply (0x2800), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  estaManHi;                       // EstaManHi: ESTA manufacturer code high byte
  uint8_t  estaManLo;                       // EstaManLo: ESTA manufacturer code low byte
  uint8_t  oemHi;                           // OemHi: OEM code high byte
  uint8_t  oemLo;                           // OemLo: OEM code low byte
  uint8_t  requestHi;                       // RequestHi: reply contents code high byte (Table 4a)
  uint8_t  requestLo;                       // RequestLo: reply contents code low byte
  uint8_t  payLenHi;                        // PayLenHi: payload length high byte
  uint8_t  payLenLo;                        // PayLenLo: payload length low byte
  uint8_t  payLoad[512];                    // PayLoad[0-512]: reply data, null terminated
} PACKED;

typedef struct artnet_data_reply_s artnet_data_reply_t;

// ---------------------------------------------------------------------------
// ArtAddress (OpCode 0x6000)
// Remote programming of node parameters. Unicast.
// ---------------------------------------------------------------------------
struct artnet_address_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpAddress (0x6000), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  netSwitch;                       // NetSwitch: bits 14-8 of 15-bit Port-Address (bit7=program)
  uint8_t  bindIndex;                       // BindIndex: bind order (1=root)
  uint8_t  shortName[ARTNET_SHORT_NAME_LENGTH]; // PortName: short name (max 17 chars + null)
  uint8_t  longName[ARTNET_LONG_NAME_LENGTH];   // LongName: long name (max 63 chars + null)
  uint8_t  swIn[ARTNET_MAX_PORTS];          // SwIn: input Port-Address bits 3-0
  uint8_t  swOut[ARTNET_MAX_PORTS];         // SwOut: output Port-Address bits 3-0
  uint8_t  subSwitch;                       // SubSwitch: bits 7-4 of 15-bit Port-Address
  uint8_t  acnPriority;                     // AcnPriority: sACN priority (0-200, 255=no change)
  uint8_t  command;                         // Command: see ArtAddress command table
} PACKED;

typedef struct artnet_address_s artnet_address_t;

// ---------------------------------------------------------------------------
// ArtDmx (OpCode 0x5000)
// Carries DMX512 data with zero start code. Unicast only (Art-Net 4).
// ---------------------------------------------------------------------------
struct artnet_dmx_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpDmx (0x5000), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  sequence;                        // Sequence: sequence number (0x01-0xff, 0x00=disabled)
  uint8_t  physical;                        // Physical: physical input port
  uint16_t  universe;                       // SubUni/Net: low byte = SubUni (universe), high byte = Net (bits 14-8)
  uint8_t  lengthHi;                        // LengthHi: DMX data length high byte (2-512)
  uint8_t  length;                          // Length: DMX data length low byte
  uint8_t  data[ARTNET_DMX_LENGTH];         // Data: DMX512 lighting data
} PACKED;

typedef struct artnet_dmx_s artnet_dmx_t;

// ---------------------------------------------------------------------------
// ArtInput (OpCode 0x7000)
// Enables or disables input ports. Unicast.
// ---------------------------------------------------------------------------
struct artnet_input_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpInput (0x7000), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: pad to match ArtPoll
  uint8_t  bindIndex;                       // BindIndex: bind order (Art-Net 4)
  uint8_t  numbportsH;                      // NumPortsHi: number of ports high byte
  uint8_t  numbports;                       // NumPortsLo: number of ports low byte
  uint8_t  input[ARTNET_MAX_PORTS];         // Input[4]: bit0=disable input, bits 7-1 unused
} PACKED;

typedef struct artnet_input_s artnet_input_t;

// ---------------------------------------------------------------------------
// ArtTodRequest (OpCode 0x8000)
// Requests RDM Table of Devices data. Unicast or broadcast.
// ---------------------------------------------------------------------------
struct artnet_todrequest_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpTodRequest (0x8000), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: pad to match ArtPoll
  uint8_t  filler2;                         // Filler2: pad to match ArtPoll
  uint8_t  spare1;                          // Spare1: transmit as zero
  uint8_t  spare2;                          // Spare2: transmit as zero
  uint8_t  spare3;                          // Spare3: transmit as zero
  uint8_t  spare4;                          // Spare4: transmit as zero
  uint8_t  spare5;                          // Spare5: transmit as zero
  uint8_t  spare6;                          // Spare6: transmit as zero
  uint8_t  spare7;                          // Spare7: transmit as zero
  uint8_t  net;                             // Net: top 7 bits of Port-Address
  uint8_t  command;                         // Command: 0x00=TodFull (send entire TOD)
  uint8_t  adCount;                         // AddCount: number of entries in Address (max 32)
  uint8_t  address[ARTNET_MAX_RDM_ADCOUNT]; // Address[32]: low byte of Port-Address
} PACKED;

typedef struct artnet_todrequest_s artnet_todrequest_t;

// ---------------------------------------------------------------------------
// ArtTodData (OpCode 0x8100)
// Contains RDM Table of Devices data. Unicast.
// ---------------------------------------------------------------------------
struct artnet_toddata_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpTodData (0x8100), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  rdmVer;                          // RdmVer: 0x00=DRAFT V1.0, 0x01=STANDARD V1.0
  uint8_t  port;                            // Port: physical port index (1-4)
  uint8_t  spare1;                          // Spare1: transmit as zero
  uint8_t  spare2;                          // Spare2: transmit as zero
  uint8_t  spare3;                          // Spare3: transmit as zero
  uint8_t  spare4;                          // Spare4: transmit as zero
  uint8_t  spare5;                          // Spare5: transmit as zero
  uint8_t  spare6;                          // Spare6: transmit as zero
  uint8_t  bindIndex;                       // BindIndex: bind order, must match ArtPollReply (Art-Net 4)
  uint8_t  net;                             // Net: top 7 bits of Port-Address
  uint8_t  cmdRes;                          // Command Response: 0x00=TodFull, 0xff=TodNak
  uint8_t  address;                         // Address: low 8 bits of Port-Address
  uint8_t  uidTotalHi;                      // UidTotalHi: total UID count high byte
  uint8_t  uidTotal;                        // UidTotalLo: total UID count low byte
  uint8_t  blockCount;                      // BlockCount: packet index (0-based)
  uint8_t  uidCount;                        // UidCount: number of UIDs in this packet
  uint8_t  tod[ARTNET_MAX_UID_COUNT][ARTNET_RDM_UID_WIDTH]; // ToD: array of RDM UIDs (48-bit each)
} PACKED;

typedef struct artnet_toddata_s artnet_toddata_t;

// ---------------------------------------------------------------------------
// ArtFirmwareMaster (OpCode 0xf200)
// Uploads firmware or UBEA data to a node. Unicast.
// ---------------------------------------------------------------------------
struct artnet_firmware_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpFirmwareMaster (0xf200), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: pad to match ArtPoll
  uint8_t  filler2;                         // Filler2: pad to match ArtPoll
  uint8_t  type;                            // Type: 0x00=FirmFirst, 0x01=FirmCont, 0x02=FirmLast, 0x03=UbeaFirst, 0x04=UbeaCont, 0x05=UbeaLast
  uint8_t  blockId;                         // BlockId: block counter (0-based)
  uint8_t  length[4];                       // FirmwareLength3..0: total Int64 word count (big-endian: bytes 3,2,1,0)
  uint8_t  spare[20];                       // Spare[20]: transmit as zero
  uint16_t  data[ARTNET_FIRMWARE_SIZE ];    // Data[512]: firmware/UBEA data, hi byte first
} PACKED;

typedef struct artnet_firmware_s artnet_firmware_t;

// ---------------------------------------------------------------------------
// ArtTodControl (OpCode 0x8200)
// Controls RDM discovery process. Unicast or broadcast.
// ---------------------------------------------------------------------------
struct artnet_todcontrol_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpTodControl (0x8200), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: pad to match ArtPoll
  uint8_t  filler2;                         // Filler2: pad to match ArtPoll
  uint8_t  spare1;                          // Spare1: transmit as zero
  uint8_t  spare2;                          // Spare2: transmit as zero
  uint8_t  spare3;                          // Spare3: transmit as zero
  uint8_t  spare4;                          // Spare4: transmit as zero
  uint8_t  spare5;                          // Spare5: transmit as zero
  uint8_t  spare6;                          // Spare6: transmit as zero
  uint8_t  spare7;                          // Spare7: transmit as zero
  uint8_t  net;                             // Net: top 7 bits of Port-Address
  uint8_t  cmd;                             // Command: 0x00=AtcNone, 0x01=AtcFlush, 0x02=AtcEnd, 0x03=AtcIncOn, 0x04=AtcIncOff
  uint8_t  address;                         // Address: low byte of Port-Address
} PACKED;

typedef struct artnet_todcontrol_s artnet_todcontrol_t;

// ---------------------------------------------------------------------------
// ArtRdm (OpCode 0x8300)
// Carries non-discovery RDM messages. Unicast (Art-Net 4 mandates unicast).
// ---------------------------------------------------------------------------
struct artnet_rdm_s {
  uint8_t id[8];                            // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpRdm (0x8300), low byte first
  uint8_t verH;                             // ProtVerHi: protocol version high byte
  uint8_t ver;                              // ProtVerLo: protocol version low byte (14)
  uint8_t rdmVer;                           // RdmVer: 0x00=DRAFT, 0x01=STANDARD
  uint8_t filler2;                          // Filler2: pad to match ArtPoll
  uint8_t spare1;                           // Spare1: transmit as zero
  uint8_t spare2;                           // Spare2: transmit as zero
  uint8_t spare3;                           // Spare3: transmit as zero
  uint8_t spare4;                           // Spare4: transmit as zero
  uint8_t spare5;                           // Spare5: transmit as zero
  uint8_t fifoAvail;                        // FifoAvail: free entries in RDM transmit queue (Art-Net 4, 0 if not implemented)
  uint8_t fifoMax;                          // FifoMax: max entries in RDM transmit queue (Art-Net 4, 0 if not implemented)
  uint8_t net;                              // Net: top 7 bits of Port-Address
  uint8_t cmd;                              // Command: 0x00=ArProcess
  uint8_t address;                          // Address: low 8 bits of Port-Address
  uint8_t data[ARTNET_MAX_RDM_DATA];        // RdmPacket: RDM data excluding DMX start code
} PACKED;

typedef struct artnet_rdm_s artnet_rdm_t;

// ---------------------------------------------------------------------------
// ArtRdmSub (OpCode 0x8400)
// Compressed RDM sub-device data. Unicast.
// ---------------------------------------------------------------------------
struct artnet_rdm_sub_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpRdmSub (0x8400), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  rdmVer;                          // RdmVer: 0x00=DRAFT, 0x01=STANDARD
  uint8_t  filler2;                         // Filler2: transmit as zero
  uint8_t  uid[ARTNET_RDM_UID_WIDTH];       // UID[6]: target RDM device UID
  uint8_t  spare1;                          // Spare1: transmit as zero
  uint8_t  commandClass;                    // CommandClass: GET/SET/GET_RESPONSE/SET_RESPONSE
  uint8_t  paramIdHi;                       // ParameterId Hi: RDM parameter ID high byte (big-endian)
  uint8_t  paramId;                         // ParameterId Lo: RDM parameter ID low byte
  uint8_t  subDeviceHi;                     // SubDevice Hi: sub-device number high byte (big-endian)
  uint8_t  subDevice;                       // SubDevice Lo: sub-device number low byte
  uint8_t  subCountHi;                      // SubCount Hi: sub-device count high byte (big-endian)
  uint8_t  subCount;                        // SubCount Lo: sub-device count low byte
  uint8_t  spare2;                          // Spare2: transmit as zero
  uint8_t  spare3;                          // Spare3: transmit as zero
  uint8_t  spare4;                          // Spare4: transmit as zero
  uint8_t  spare5;                          // Spare5: transmit as zero
  uint8_t  data[ARTNET_MAX_RDM_DATA];       // Data: packed big-endian data
} PACKED;

typedef struct artnet_rdm_sub_s artnet_rdm_sub_t;

// ---------------------------------------------------------------------------
// ArtDiagData (OpCode 0x2300)
// Carries diagnostic text messages. Broadcast or unicast.
// ---------------------------------------------------------------------------
struct artnet_diagdata_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpDiagData (0x2300), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: transmit as zero
  uint8_t  diagPriority;                    // DiagPriority: priority of diagnostic data (Table 5)
  uint8_t  logicalPort;                     // LogicalPort: logical DMX port (0=general)
  uint8_t  filler3;                         // Filler3: transmit as zero
  uint8_t  lengthHi;                        // LengthHi: text length high byte
  uint8_t  length;                          // LengthLo: text length low byte
  uint8_t  data[ARTNET_DMX_LENGTH];         // Data: ASCII text, null terminated, max 512
} PACKED;

typedef struct artnet_diagdata_s artnet_diagdata_t;

// ---------------------------------------------------------------------------
// ArtFirmwareReply (OpCode 0xf300)
// Response to ArtFirmwareMaster upload. Unicast.
// ---------------------------------------------------------------------------
struct artnet_firmware_reply_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpFirmwareReply (0xf300), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: pad to match ArtPoll
  uint8_t  filler2;                         // Filler2: pad to match ArtPoll
  uint8_t  type;                            // Type: 0x00=FirmBlockGood, 0x01=FirmAllGood, 0xff=FirmFail
  uint8_t  spare[21];                       // Spare[21]: transmit as zero
} PACKED;

typedef struct artnet_firmware_reply_s artnet_firmware_reply_t;

// ---------------------------------------------------------------------------
// ArtSync (OpCode 0x5200)
// Synchronizes ArtDmx output across nodes. Broadcast.
// ---------------------------------------------------------------------------
struct artnet_sync_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpSync (0x5200), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  aux1;                            // Aux1: transmit as zero
  uint8_t  aux2;                            // Aux2: transmit as zero
} PACKED;

typedef struct artnet_sync_s artnet_sync_t;

// ---------------------------------------------------------------------------
// ArtNzs (OpCode 0x5100)
// Carries DMX512 data with non-zero start code (excl. RDM). Unicast only (Art-Net 4).
// ---------------------------------------------------------------------------
struct artnet_nzs_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpNzs (0x5100), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  sequence;                        // Sequence: sequence number (0x01-0xff, 0x00=disabled)
  uint8_t  startCode;                       // StartCode: DMX512 start code (not zero, not RDM)
  uint16_t universe;                        // SubUni/Net: low byte = SubUni (universe), high byte = Net (bits 14-8)
  uint8_t  lengthHi;                        // LengthHi: data length high byte (1-512)
  uint8_t  length;                          // Length: data length low byte
  uint8_t  data[ARTNET_DMX_LENGTH];         // Data: DMX512 data
} PACKED;

typedef struct artnet_nzs_s artnet_nzs_t;

// ---------------------------------------------------------------------------
// ArtCommand (OpCode 0x2400)
// Text-based command. Manufacturer-specific commands use EstaMan=0xFFFF.
// Broadcast or unicast.
// ---------------------------------------------------------------------------
struct artnet_command_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpCommand (0x2400), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  estaManHi;                       // EstaManHi: ESTA manufacturer code high byte
  uint8_t  estaManLo;                       // EstaManLo: ESTA manufacturer code low byte
  uint8_t  lengthHi;                        // LengthHi: text length high byte
  uint8_t  lengthLo;                        // LengthLo: text length low byte
  uint8_t  data[ARTNET_DMX_LENGTH];         // Data: ASCII text command, null terminated, max 512
} PACKED;

typedef struct artnet_command_s artnet_command_t;

// ---------------------------------------------------------------------------
// ArtTimeCode (OpCode 0x9700)
// Transports SMPTE/EBU timecode. Broadcast.
// ---------------------------------------------------------------------------
struct artnet_timecode_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpTimeCode (0x9700), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: transmit as zero
  uint8_t  streamId;                        // StreamId: stream identifier (Art-Net 4, 0x00 = master)
  uint8_t  frames;                          // Frames: 0-29 depending on mode
  uint8_t  seconds;                         // Seconds: 0-59
  uint8_t  minutes;                         // Minutes: 0-59
  uint8_t  hours;                           // Hours: 0-23
  uint8_t  type;                            // Type: 0=Film(24fps), 1=EBU(25fps), 2=DF(29.97fps), 3=SMPTE(30fps)
} PACKED;

typedef struct artnet_timecode_s artnet_timecode_t;

// ---------------------------------------------------------------------------
// ArtTimeSync (OpCode 0x9800)
// Synchronizes real-time date and clock. Broadcast.
// ---------------------------------------------------------------------------
struct artnet_timesync_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpTimeSync (0x9800), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: transmit as zero
  uint8_t  filler2;                         // Filler2: transmit as zero
  uint8_t  tm_sec;                          // tm_sec: seconds (0-59)
  uint8_t  tm_min;                          // tm_min: minutes (0-59)
  uint8_t  tm_hour;                         // tm_hour: hours (0-23)
  uint8_t  tm_mday;                         // tm_mday: day of month (1-31)
  uint8_t  tm_mon;                          // tm_mon: month (0-11)
  uint8_t  tm_year;                         // tm_year: year (0 = 1900)
  uint8_t  filler3;                         // Filler3: transmit as zero
  uint8_t  filler4;                         // Filler4: transmit as zero
  uint8_t  filler5;                         // Filler5: transmit as zero
  uint8_t  filler6;                         // Filler6: transmit as zero
} PACKED;

typedef struct artnet_timesync_s artnet_timesync_t;

// ---------------------------------------------------------------------------
// ArtTrigger (OpCode 0x9900)
// Triggers macros on target nodes. Broadcast.
// ---------------------------------------------------------------------------
struct artnet_trigger_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpTrigger (0x9900), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: transmit as zero
  uint8_t  filler2;                         // Filler2: transmit as zero
  uint8_t  oemCodeHi;                       // OemHi: OEM code high byte of target nodes
  uint8_t  oemCodeLo;                       // OemLo: OEM code low byte of target nodes
  uint8_t  key;                             // Key: trigger key (Table 7: 0=ASCII, 1=Macro, 2=Soft, 3=Show)
  uint8_t  subKey;                          // SubKey: trigger sub-key
  uint8_t  data[ARTNET_DMX_LENGTH];         // Data[512]: payload (fixed length)
} PACKED;

typedef struct artnet_trigger_s artnet_trigger_t;

// ---------------------------------------------------------------------------
// ArtDirectory (OpCode 0x9a00)
// Requests a node's file list. Unicast.
// ---------------------------------------------------------------------------
struct artnet_directory_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpDirectory (0x9a00), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: transmit as zero
  uint8_t  filler2;                         // Filler2: transmit as zero
  uint8_t  spare[11];                       // Spare[11]: transmit as zero
} PACKED;

typedef struct artnet_directory_s artnet_directory_t;

// ---------------------------------------------------------------------------
// ArtDirectoryReply (OpCode 0x9b00)
// Response to ArtDirectory with file list. Unicast.
// ---------------------------------------------------------------------------
struct artnet_directory_reply_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpDirectoryReply (0x9b00), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  spare[4];                        // Spare[4]: transmit as zero
  uint8_t  dirCountHi;                      // DirCountHi: entries in this packet high byte
  uint8_t  dirCountLo;                      // DirCountLo: entries in this packet low byte
  uint8_t  dirTotalHi;                      // DirTotalHi: total entries high byte
  uint8_t  dirTotalLo;                      // DirTotalLo: total entries low byte
  uint8_t  dirEntry[2048];                  // DirEntry: directory entries
} PACKED;

typedef struct artnet_directory_reply_s artnet_directory_reply_t;

// ---------------------------------------------------------------------------
// ArtFileTnMaster (OpCode 0xf400)
// Upload user file to node. Unicast.
// ---------------------------------------------------------------------------
struct artnet_file_tn_master_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpFileTnMaster (0xf400), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: pad to match ArtPoll
  uint8_t  filler2;                         // Filler2: pad to match ArtPoll
  uint8_t  type;                            // Type: file type indicator
  uint8_t  blockId;                         // BlockId: block counter (0-based)
  uint8_t  length[4];                       // Lcb[4]: total byte count (big-endian)
  uint8_t  spare[20];                       // Spare[20]: transmit as zero
  uint16_t data[ARTNET_FIRMWARE_SIZE];      // Data[512]: file data
} PACKED;

typedef struct artnet_file_tn_master_s artnet_file_tn_master_t;

// ---------------------------------------------------------------------------
// ArtFileFnMaster (OpCode 0xf500)
// Download user file from node. Unicast.
// ---------------------------------------------------------------------------
struct artnet_file_fn_master_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpFileFnMaster (0xf500), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: transmit as zero
  uint8_t  filler2;                         // Filler2: transmit as zero
  uint8_t  spare[11];                       // Spare[11]: transmit as zero
  uint8_t  lengthHi;                        // LengthHi: filename length high byte
  uint8_t  lengthLo;                        // LengthLo: filename length low byte
  uint8_t  filename[256];                   // Filename[256]: filename to download, null terminated
} PACKED;

typedef struct artnet_file_fn_master_s artnet_file_fn_master_t;

// ---------------------------------------------------------------------------
// ArtFileFnReply (OpCode 0xf600)
// Response to ArtFileFnMaster with file data. Unicast.
// ---------------------------------------------------------------------------
struct artnet_file_fn_reply_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpFileFnReply (0xf600), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: transmit as zero
  uint8_t  filler2;                         // Filler2: transmit as zero
  uint8_t  spare[11];                       // Spare[11]: transmit as zero
  uint8_t  fileLengthHi;                    // FileLengthHi: total file length high byte
  uint8_t  fileLengthLo;                    // FileLengthLo: total file length low byte
  uint8_t  blockId;                         // BlockId: block counter (0-based)
  uint16_t data[ARTNET_FIRMWARE_SIZE];      // Data[512]: file data
} PACKED;

typedef struct artnet_file_fn_reply_s artnet_file_fn_reply_t;

// ---------------------------------------------------------------------------
// ArtMediaPatch (OpCode 0x9100)
// Media server patch control. Broadcast or unicast.
// ---------------------------------------------------------------------------
struct artnet_media_patch_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpMediaPatch (0x9100), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  physical;                        // Physical: physical input port
  uint16_t universe;                        // SubUni/Net: low byte = SubUni, high byte = Net
  uint8_t  lengthHi;                        // LengthHi: data length high byte
  uint8_t  length;                          // Length: data length low byte
  uint8_t  data[ARTNET_DMX_LENGTH];         // Data: patch data
} PACKED;

typedef struct artnet_media_patch_s artnet_media_patch_t;

// ---------------------------------------------------------------------------
// ArtMediaControl / ArtMediaControlReply (OpCode 0x9200 / 0x9300)
// Media server control. Unicast.
// ---------------------------------------------------------------------------
struct artnet_media_control_s {
  uint8_t  id[8];                           // ID: "Art-Net" + 0x00
  uint16_t opCode;                          // OpCode: OpMediaControl (0x9200) or OpMediaContrlReply (0x9300), low byte first
  uint8_t  verH;                            // ProtVerHi: protocol version high byte
  uint8_t  ver;                             // ProtVerLo: protocol version low byte (14)
  uint8_t  filler1;                         // Filler1: transmit as zero
  uint8_t  filler2;                         // Filler2: transmit as zero
  uint8_t  spare[12];                       // Spare[12]: transmit as zero
  uint8_t  data[ARTNET_DMX_LENGTH];         // Data: media control data
} PACKED;

typedef struct artnet_media_control_s artnet_media_control_t;


// union of all artnet packets
typedef union {
  artnet_poll_t ap;
  artnet_reply_t ar;
  artnet_ipprog_t aip;
  artnet_ipprog_reply_t aipr;
  artnet_data_request_t datareq;
  artnet_data_reply_t datarep;
  artnet_address_t addr;
  artnet_dmx_t admx;
  artnet_input_t ainput;
  artnet_todrequest_t todreq;
  artnet_toddata_t toddata;
  artnet_firmware_t firmware;
  artnet_firmware_reply_t firmwarer;
  artnet_todcontrol_t todcontrol;
  artnet_rdm_t rdm;
  artnet_rdm_sub_t rdmsub;
  artnet_diagdata_t diagdata;
  artnet_sync_t asyn;
  artnet_nzs_t nzs;
  artnet_command_t cmd;
  artnet_timecode_t tc;
  artnet_timesync_t tsync;
  artnet_trigger_t trigger;
  artnet_directory_t dir;
  artnet_directory_reply_t dirr;
  artnet_file_tn_master_t filetn;
  artnet_file_fn_master_t filefn;
  artnet_file_fn_reply_t filefnr;
  artnet_media_patch_t mpatch;
  artnet_media_control_t mctrl;
} artnet_packet_union_t;


// a packet, containing data, length, type and a src/dst address
typedef struct {
  int length;
  struct in_addr from;
  struct in_addr to;
  artnet_packet_type_t type;
  artnet_packet_union_t data;
} artnet_packet_t;

typedef artnet_packet_t *artnet_packet;


#ifdef _MSC_VER
	#pragma pack(pop)
#endif

#undef PACKED


#endif
