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
  ARTNET_POLL = 0x2000,              // ArtPoll - node discovery / poll
  ARTNET_REPLY = 0x2100,             // ArtPollReply - node status response
  ARTNET_DIAGDATA = 0x2300,          // ArtDiagData - diagnostic data packet
  ARTNET_COMMAND = 0x2400,           // ArtCommand - text-based command
  // data transport
  ARTNET_DMX = 0x5000,               // ArtDmx - DMX512 zero start code data
  ARTNET_NZS = 0x5100,               // ArtNzs - DMX512 non-zero start code data (excl. RDM)
  ARTNET_SYNC = 0x5200,              // ArtSync - synchronize ArtDmx output to nodes
  // remote programming
  ARTNET_ADDRESS = 0x6000,           // ArtAddress - remote node programming
  ARTNET_INPUT = 0x7000,             // ArtInput - input port enable / disable
  // RDM (Remote Device Management)
  ARTNET_TODREQUEST = 0x8000,        // ArtTodRequest - RDM table of devices request
  ARTNET_TODDATA = 0x8100,           // ArtTodData - RDM table of devices data
  ARTNET_TODCONTROL = 0x8200,        // ArtTodControl - RDM discovery control
  ARTNET_RDM = 0x8300,               // ArtRdm - RDM non-discovery messages
  ARTNET_RDMSUB = 0x8400,            // ArtRdmSub - compressed RDM sub-device data
  // media server
  ARTNET_MEDIA = 0x9000,             // ArtMedia - media server data
  ARTNET_MEDIAPATCH = 0x9100,        // ArtMediaPatch - media patch control
  ARTNET_MEDIACONTROL = 0x9200,      // ArtMediaControl - media control
  ARTNET_MEDIACONTROLREPLY = 0x9300, // ArtMediaControlReply - media control response
  // time & trigger
  ARTNET_TIMECODE = 0x9700,          // ArtTimeCode - SMPTE / EBU timecode transport
  ARTNET_TIMESYNC = 0x9800,          // ArtTimeSync - real-time date & clock sync
  ARTNET_TRIGGER = 0x9900,           // ArtTrigger - trigger macro
  // directory
  ARTNET_DIRECTORY = 0x9a00,         // ArtDirectory - request node file list
  ARTNET_DIRECTORYREPLY = 0x9b00,    // ArtDirectoryReply - node file list response
  // video extension (deprecated)
  ARTNET_VIDEOSTEUP = 0xa010,        // ArtVideoSetup - video screen settings
  ARTNET_VIDEOPALETTE = 0xa020,      // ArtVideoPalette - video palette settings
  ARTNET_VIDEODATA = 0xa040,         // ArtVideoData - video display data
  // MAC programming (deprecated)
  ARTNET_MACMASTER = 0xf000,         // ArtMacMaster - MAC master (deprecated)
  ARTNET_MACSLAVE = 0xf100,          // ArtMacSlave - MAC slave (deprecated)
  // firmware upload
  ARTNET_FIRMWAREMASTER = 0xf200,    // ArtFirmwareMaster - firmware / UBEA upload
  ARTNET_FIRMWAREREPLY = 0xf300,     // ArtFirmwareReply - firmware upload response
  // file transfer
  ARTNET_FILETNMASTER = 0xf400,      // ArtFileTnMaster - upload user file to node
  ARTNET_FILEFNMASTER = 0xf500,      // ArtFileFnMaster - download user file from node
  ARTNET_FILEFNREPLY = 0xf600,       // ArtFileFnReply - file download response
  // IP programming
  ARTNET_IPPROG = 0xf800,            // ArtIpProg - IP address / subnet / gateway programming
  ARTNET_IPREPLY = 0xf900,           // ArtIpProgReply - IP programming response
} PACKED;

typedef enum artnet_packet_type_e artnet_packet_type_t;

struct	artnet_poll_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  flags;
  uint8_t  diagPriority;
  uint8_t  targetPortAddressTopHi;
  uint8_t  targetPortAddressTopLo;
  uint8_t  targetPortAddressBottomHi;
  uint8_t  targetPortAddressBottomLo;
} PACKED;

typedef struct artnet_poll_s artnet_poll_t;

struct artnet_reply_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  ip[4];
  uint16_t port;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  netSwitch;
  uint8_t  subSwitch;
  uint8_t  oemH;
  uint8_t  oem;
  uint8_t  ubea;
  uint8_t  status;
  uint8_t  estaMan[2];
  uint8_t  shortName[ARTNET_SHORT_NAME_LENGTH];
  uint8_t  longName[ARTNET_LONG_NAME_LENGTH];
  uint8_t  nodeReport[ARTNET_REPORT_LENGTH];
  uint8_t  numbportsH;
  uint8_t  numbports;
  uint8_t  portTypes[ARTNET_MAX_PORTS];
  uint8_t  goodInput[ARTNET_MAX_PORTS];
  uint8_t  goodOutputA[ARTNET_MAX_PORTS];
  uint8_t  swIn[ARTNET_MAX_PORTS];
  uint8_t  swOut[ARTNET_MAX_PORTS];
  uint8_t  acnPriority;
  uint8_t  swMacro;
  uint8_t  swRemote;
  uint8_t  sp1;
  uint8_t  sp2;
  uint8_t  mac[ARTNET_MAC_SIZE];
  uint8_t  bindIp[ARTNET_IP_SIZE];
  uint8_t  bindIndex;
  uint8_t  status2;
  uint8_t  goodOutputB[ARTNET_MAX_PORTS];
  uint8_t  status3;
  uint8_t  defaultRespUid[ARTNET_RDM_UID_WIDTH];
  uint8_t  filler[15];
} PACKED;

typedef struct artnet_reply_s artnet_reply_t;

struct artnet_ipprog_s {
  uint8_t  id[8];
  uint16_t OpCode;
  uint8_t  ProVerH;
  uint8_t  ProVer;
  uint8_t  Filler1;
  uint8_t  Filler2;
  uint8_t  Command;
  uint8_t  Filler4;
  uint8_t  ProgIpHi;
  uint8_t  ProgIp2;
  uint8_t  ProgIp1;
  uint8_t  ProgIpLo;
  uint8_t  ProgSmHi;
  uint8_t  ProgSm2;
  uint8_t  ProgSm1;
  uint8_t  ProgSmLo;
  uint8_t  ProgPortHi;
  uint8_t  ProgPortLo;
  uint8_t  ProgDgHi;
  uint8_t  ProgDg2;
  uint8_t  ProgDg1;
  uint8_t  ProgDgLo;
  uint8_t  Spare1;
  uint8_t  Spare2;
  uint8_t  Spare3;
  uint8_t  Spare4;

} PACKED;

typedef struct artnet_ipprog_s artnet_ipprog_t;

struct artnet_ipprog_reply_s {
  uint8_t id[8];
  uint16_t  OpCode;
  uint8_t  ProVerH;
  uint8_t  ProVer;
  uint8_t  Filler1;
  uint8_t  Filler2;
  uint8_t  Status;
  uint8_t  Filler4;
  uint8_t  ProgIpHi;
  uint8_t  ProgIp2;
  uint8_t  ProgIp1;
  uint8_t  ProgIpLo;
  uint8_t  ProgSmHi;
  uint8_t  ProgSm2;
  uint8_t  ProgSm1;
  uint8_t  ProgSmLo;
  uint8_t  ProgPortHi;
  uint8_t  ProgPortLo;
  uint8_t  ProgDgHi;
  uint8_t  ProgDg2;
  uint8_t  ProgDg1;
  uint8_t  ProgDgLo;
  uint8_t  Spare1;
  uint8_t  Spare2;
  uint8_t  Spare3;
  uint8_t  Spare4;
} PACKED;

typedef struct artnet_ipprog_reply_s artnet_ipprog_reply_t;


struct artnet_address_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  netSwitch;
  uint8_t  bindIndex;
  uint8_t  shortName[ARTNET_SHORT_NAME_LENGTH];
  uint8_t  longName[ARTNET_LONG_NAME_LENGTH];
  uint8_t  swIn[ARTNET_MAX_PORTS];
  uint8_t  swOut[ARTNET_MAX_PORTS];
  uint8_t  subSwitch;
  uint8_t  acnPriority;
  uint8_t  command;
} PACKED;

typedef struct artnet_address_s artnet_address_t;


struct artnet_dmx_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  sequence;
  uint8_t  physical;
  uint16_t  universe;
  uint8_t  lengthHi;
  uint8_t  length;
  uint8_t  data[ARTNET_DMX_LENGTH];
} PACKED;

typedef struct artnet_dmx_s artnet_dmx_t;


struct artnet_input_s {
  uint8_t id[8];
  uint16_t  opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  bindIndex;
  uint8_t  numbportsH;
  uint8_t  numbports;
  uint8_t  input[ARTNET_MAX_PORTS];
} PACKED;

typedef struct artnet_input_s artnet_input_t;


struct artnet_todrequest_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  spare1;
  uint8_t  spare2;
  uint8_t  spare3;
  uint8_t  spare4;
  uint8_t  spare5;
  uint8_t  spare6;
  uint8_t  spare7;
  uint8_t  net;
  uint8_t  command;
  uint8_t  adCount;
  uint8_t  address[ARTNET_MAX_RDM_ADCOUNT];
} PACKED;

typedef struct artnet_todrequest_s artnet_todrequest_t;



struct artnet_toddata_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  rdmVer;
  uint8_t  port;
  uint8_t  spare1;
  uint8_t  spare2;
  uint8_t  spare3;
  uint8_t  spare4;
  uint8_t  spare5;
  uint8_t  spare6;
  uint8_t  bindIndex;
  uint8_t  net;
  uint8_t  cmdRes;
  uint8_t  address;
  uint8_t  uidTotalHi;
  uint8_t  uidTotal;
  uint8_t  blockCount;
  uint8_t  uidCount;
  uint8_t  tod[ARTNET_MAX_UID_COUNT][ARTNET_RDM_UID_WIDTH];
} PACKED;

typedef struct artnet_toddata_s artnet_toddata_t;

struct artnet_firmware_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  type;
  uint8_t  blockId;
  uint8_t  length[4];
  uint8_t  spare[20];
  uint16_t  data[ARTNET_FIRMWARE_SIZE ];
} PACKED;

typedef struct artnet_firmware_s artnet_firmware_t;

struct artnet_todcontrol_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  spare1;
  uint8_t  spare2;
  uint8_t  spare3;
  uint8_t  spare4;
  uint8_t  spare5;
  uint8_t  spare6;
  uint8_t  spare7;
  uint8_t  net;
  uint8_t  cmd;
  uint8_t  address;
} PACKED;


typedef struct artnet_todcontrol_s artnet_todcontrol_t;



struct artnet_rdm_s {
  uint8_t id[8];
  uint16_t  opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  rdmVer;
  uint8_t  filler2;
  uint8_t  spare1;
  uint8_t  spare2;
  uint8_t  spare3;
  uint8_t  spare4;
  uint8_t  spare5;
  uint8_t  spare6;
  uint8_t  spare7;
  uint8_t  net;
  uint8_t  cmd;
  uint8_t  address;
  uint8_t  data[ARTNET_MAX_RDM_DATA];
} PACKED;


typedef struct artnet_rdm_s artnet_rdm_t;


struct artnet_rdm_sub_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  rdmVer;
  uint8_t  filler2;
  uint8_t  uid[ARTNET_RDM_UID_WIDTH];
  uint8_t  spare1;
  uint8_t  commandClass;
  uint8_t  paramIdHi;
  uint8_t  paramId;
  uint8_t  subDeviceHi;
  uint8_t  subDevice;
  uint8_t  subCountHi;
  uint8_t  subCount;
  uint8_t  spare2;
  uint8_t  spare3;
  uint8_t  spare4;
  uint8_t  spare5;
  uint8_t  data[ARTNET_MAX_RDM_DATA];
} PACKED;

typedef struct artnet_rdm_sub_s artnet_rdm_sub_t;


struct artnet_diagdata_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  diagPriority;
  uint8_t  logicalPort;
  uint8_t  filler3;
  uint8_t  lengthHi;
  uint8_t  length;
  uint8_t  data[ARTNET_DMX_LENGTH];
} PACKED;

typedef struct artnet_diagdata_s artnet_diagdata_t;


struct artnet_firmware_reply_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  type;
  uint8_t  spare[21];
} PACKED;

typedef struct artnet_firmware_reply_s artnet_firmware_reply_t;


struct artnet_sync_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  aux1;
  uint8_t  aux2;
} PACKED;

typedef struct artnet_sync_s artnet_sync_t;


struct artnet_nzs_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  sequence;
  uint8_t  startCode;
  uint16_t universe;
  uint8_t  lengthHi;
  uint8_t  length;
  uint8_t  data[ARTNET_DMX_LENGTH];
} PACKED;

typedef struct artnet_nzs_s artnet_nzs_t;


struct artnet_command_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  estaManHi;
  uint8_t  estaManLo;
  uint8_t  lengthHi;
  uint8_t  lengthLo;
  uint8_t  data[ARTNET_DMX_LENGTH];
} PACKED;

typedef struct artnet_command_s artnet_command_t;


struct artnet_timecode_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  frames;
  uint8_t  seconds;
  uint8_t  minutes;
  uint8_t  hours;
  uint8_t  type;
} PACKED;

typedef struct artnet_timecode_s artnet_timecode_t;


struct artnet_timesync_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  tm_sec;
  uint8_t  tm_min;
  uint8_t  tm_hour;
  uint8_t  tm_mday;
  uint8_t  tm_mon;
  uint8_t  tm_year;
  uint8_t  filler3;
  uint8_t  filler4;
  uint8_t  filler5;
  uint8_t  filler6;
} PACKED;

typedef struct artnet_timesync_s artnet_timesync_t;


struct artnet_trigger_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  oemCodeHi;
  uint8_t  oemCodeLo;
  uint8_t  key;
  uint8_t  subKey;
  uint8_t  data[ARTNET_DMX_LENGTH];
} PACKED;

typedef struct artnet_trigger_s artnet_trigger_t;


struct artnet_directory_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  spare[11];
} PACKED;

typedef struct artnet_directory_s artnet_directory_t;


struct artnet_directory_reply_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  spare[4];
  uint8_t  dirCountHi;
  uint8_t  dirCountLo;
  uint8_t  dirTotalHi;
  uint8_t  dirTotalLo;
  uint8_t  dirEntry[2048];
} PACKED;

typedef struct artnet_directory_reply_s artnet_directory_reply_t;


struct artnet_file_tn_master_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  type;
  uint8_t  blockId;
  uint8_t  length[4];
  uint8_t  spare[20];
  uint16_t data[ARTNET_FIRMWARE_SIZE];
} PACKED;

typedef struct artnet_file_tn_master_s artnet_file_tn_master_t;


struct artnet_file_fn_master_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  spare[11];
  uint8_t  lengthHi;
  uint8_t  lengthLo;
  uint8_t  filename[256];
} PACKED;

typedef struct artnet_file_fn_master_s artnet_file_fn_master_t;


struct artnet_file_fn_reply_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  spare[11];
  uint8_t  fileLengthHi;
  uint8_t  fileLengthLo;
  uint8_t  blockId;
  uint16_t data[ARTNET_FIRMWARE_SIZE];
} PACKED;

typedef struct artnet_file_fn_reply_s artnet_file_fn_reply_t;


struct artnet_media_patch_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  physical;
  uint16_t universe;
  uint8_t  lengthHi;
  uint8_t  length;
  uint8_t  data[ARTNET_DMX_LENGTH];
} PACKED;

typedef struct artnet_media_patch_s artnet_media_patch_t;


struct artnet_media_control_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint8_t  verH;
  uint8_t  ver;
  uint8_t  filler1;
  uint8_t  filler2;
  uint8_t  spare[12];
  uint8_t  data[ARTNET_DMX_LENGTH];
} PACKED;

typedef struct artnet_media_control_s artnet_media_control_t;


// union of all artnet packets
typedef union {
  artnet_poll_t ap;
  artnet_reply_t ar;
  artnet_ipprog_t aip;
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
  artnet_ipprog_reply_t aipr;
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
