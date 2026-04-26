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
 * artnet.h
 * Interface to libartnet
 * Copyright (C) 2004-2007 Simon Newton
 */

#ifndef ARTNET_HEADER_H
#define ARTNET_HEADER_H

#include <stdint.h>
// order is important here for osx
#include <sys/types.h>

#if !defined(_WIN32) && !defined(_MSC_VER)
#include <sys/select.h>
#else
#include <winsock2.h>
typedef unsigned long in_addr_t;
#endif

#include <artnet/common.h>

/* the external storage class is "extern" in UNIX; in MSW it's ugly. */
#ifndef EXTERN
#ifdef MSW
#define EXTERN __declspec(dllexport) extern
#else
#define EXTERN extern
#endif /* MSW */
#endif

#ifdef __cplusplus
extern "C" {
#endif

EXTERN int ARTNET_ADDRESS_NO_CHANGE;

/**
 * An enum for setting the behaviour of a port.
 * Ports can either input data (DMX -> ArtNet) or
 * output (ArtNet -> DMX) data.
 */
typedef enum {
  ARTNET_ENABLE_INPUT = 0x40,  /**< Enables the input for this port */
  ARTNET_ENABLE_OUTPUT = 0x80  /**< Enables the output for this port */
} artnet_port_settings_t;


/*
 * ArtAddress Command field values (spec ArtAddress Command table).
 * Port-indexed commands with suffix _1/_2/_3 are deprecated in Art-Net 4
 * (Art-Net 4 uses BindIndex instead of port-1/2/3 within a single packet).
 */
typedef enum {
  ARTNET_PC_NONE = 0x00,             /**< AcNone: no action */
  ARTNET_PC_CANCEL = 0x01,           /**< AcCancelMerge: cancel merge on next ArtDmx */
  ARTNET_PC_LED_NORMAL = 0x02,       /**< AcLedNormal: front panel indicators operate normally */
  ARTNET_PC_LED_MUTE = 0x03,         /**< AcLedMute: front panel indicators switched off */
  ARTNET_PC_LED_LOCATE = 0x04,       /**< AcLedLocate: rapid flash for outlet identification */
  ARTNET_PC_RESET = 0x05,            /**< AcResetRx: reset node SIP flags */
  ARTNET_PC_ANALYSIS_ON = 0x06,      /**< AcAnalysisOn: enable analysis/debug mode */
  ARTNET_PC_ANALYSIS_OFF = 0x07,     /**< AcAnalysisOff: disable analysis/debug mode */
  ARTNET_PC_FAIL_HOLD = 0x08,        /**< AcFailHold: hold last state on data loss */
  ARTNET_PC_FAIL_ZERO = 0x09,        /**< AcFailZero: output zero on data loss */
  ARTNET_PC_FAIL_FULL = 0x0a,        /**< AcFailFull: output full on data loss */
  ARTNET_PC_FAIL_SCENE = 0x0b,       /**< AcFailScene: play failsafe scene on data loss */
  ARTNET_PC_FAIL_RECORD = 0x0c,      /**< AcFailRecord: record current state as failsafe scene */
  ARTNET_PC_MERGE_LTP_0 = 0x10,      /**< AcMergeLtp0: set port 0 to LTP merge */
  ARTNET_PC_MERGE_LTP_1 = 0x11,      /**< AcMergeLtp1 (deprecated in Art-Net 4) */
  ARTNET_PC_MERGE_LTP_2 = 0x12,      /**< AcMergeLtp2 (deprecated in Art-Net 4) */
  ARTNET_PC_MERGE_LTP_3 = 0x13,      /**< AcMergeLtp3 (deprecated in Art-Net 4) */
  ARTNET_PC_DIRECTION_TX_0 = 0x20,   /**< AcDirectionTx0: set port 0 direction to output */
  ARTNET_PC_DIRECTION_TX_1 = 0x21,   /**< AcDirectionTx1 (deprecated in Art-Net 4) */
  ARTNET_PC_DIRECTION_TX_2 = 0x22,   /**< AcDirectionTx2 (deprecated in Art-Net 4) */
  ARTNET_PC_DIRECTION_TX_3 = 0x23,   /**< AcDirectionTx3 (deprecated in Art-Net 4) */
  ARTNET_PC_DIRECTION_RX_0 = 0x30,   /**< AcDirectionRx0: set port 0 direction to input */
  ARTNET_PC_DIRECTION_RX_1 = 0x31,   /**< AcDirectionRx1 (deprecated in Art-Net 4) */
  ARTNET_PC_DIRECTION_RX_2 = 0x32,   /**< AcDirectionRx2 (deprecated in Art-Net 4) */
  ARTNET_PC_DIRECTION_RX_3 = 0x33,   /**< AcDirectionRx3 (deprecated in Art-Net 4) */
  ARTNET_PC_MERGE_HTP_0 = 0x50,      /**< AcMergeHtp0: set port 0 to HTP merge (default) */
  ARTNET_PC_MERGE_HTP_1 = 0x51,      /**< AcMergeHtp1 (deprecated in Art-Net 4) */
  ARTNET_PC_MERGE_HTP_2 = 0x52,      /**< AcMergeHtp2 (deprecated in Art-Net 4) */
  ARTNET_PC_MERGE_HTP_3 = 0x53,      /**< AcMergeHtp3 (deprecated in Art-Net 4) */
  ARTNET_PC_ARTNET_SEL_0 = 0x60,     /**< AcArtNetSel0: set port 0 to Art-Net (default) */
  ARTNET_PC_ARTNET_SEL_1 = 0x61,     /**< AcArtNetSel1 (deprecated in Art-Net 4) */
  ARTNET_PC_ARTNET_SEL_2 = 0x62,     /**< AcArtNetSel2 (deprecated in Art-Net 4) */
  ARTNET_PC_ARTNET_SEL_3 = 0x63,     /**< AcArtNetSel3 (deprecated in Art-Net 4) */
  ARTNET_PC_ACN_SEL_0 = 0x70,        /**< AcAcnSel0: set port 0 to sACN */
  ARTNET_PC_ACN_SEL_1 = 0x71,        /**< AcAcnSel1 (deprecated in Art-Net 4) */
  ARTNET_PC_ACN_SEL_2 = 0x72,        /**< AcAcnSel2 (deprecated in Art-Net 4) */
  ARTNET_PC_ACN_SEL_3 = 0x73,        /**< AcAcnSel3 (deprecated in Art-Net 4) */
  ARTNET_PC_CLR_0 = 0x90,            /**< AcClearOp0: clear DMX output buffer for port 0 */
  ARTNET_PC_CLR_1 = 0x91,            /**< AcClearOp1 (deprecated in Art-Net 4) */
  ARTNET_PC_CLR_2 = 0x92,            /**< AcClearOp2 (deprecated in Art-Net 4) */
  ARTNET_PC_CLR_3 = 0x93,            /**< AcClearOp3 (deprecated in Art-Net 4) */
  ARTNET_PC_STYLE_DELTA_0 = 0xa0,    /**< AcStyleDelta0: delta output mode for port 0 */
  ARTNET_PC_STYLE_DELTA_1 = 0xa1,    /**< AcStyleDelta1 (deprecated in Art-Net 4) */
  ARTNET_PC_STYLE_DELTA_2 = 0xa2,    /**< AcStyleDelta2 (deprecated in Art-Net 4) */
  ARTNET_PC_STYLE_DELTA_3 = 0xa3,    /**< AcStyleDelta3 (deprecated in Art-Net 4) */
  ARTNET_PC_STYLE_CONST_0 = 0xb0,    /**< AcStyleConst0: constant output mode for port 0 */
  ARTNET_PC_STYLE_CONST_1 = 0xb1,    /**< AcStyleConst1 (deprecated in Art-Net 4) */
  ARTNET_PC_STYLE_CONST_2 = 0xb2,    /**< AcStyleConst2 (deprecated in Art-Net 4) */
  ARTNET_PC_STYLE_CONST_3 = 0xb3,    /**< AcStyleConst3 (deprecated in Art-Net 4) */
  ARTNET_PC_RDM_ENABLED_0 = 0xc0,    /**< AcRdmEnable0: enable RDM for port 0 */
  ARTNET_PC_RDM_ENABLED_1 = 0xc1,    /**< AcRdmEnable1 (deprecated in Art-Net 4) */
  ARTNET_PC_RDM_ENABLED_2 = 0xc2,    /**< AcRdmEnable2 (deprecated in Art-Net 4) */
  ARTNET_PC_RDM_ENABLED_3 = 0xc3,    /**< AcRdmEnable3 (deprecated in Art-Net 4) */
  ARTNET_PC_RDM_DISABLED_0 = 0xd0,   /**< AcRdmDisable0: disable RDM for port 0 */
  ARTNET_PC_RDM_DISABLED_1 = 0xd1,   /**< AcRdmDisable1 (deprecated in Art-Net 4) */
  ARTNET_PC_RDM_DISABLED_2 = 0xd2,   /**< AcRdmDisable2 (deprecated in Art-Net 4) */
  ARTNET_PC_RDM_DISABLED_3 = 0xd3,   /**< AcRdmDisable3 (deprecated in Art-Net 4) */
  ARTNET_PC_BQP_NONE = 0xe0,         /**< AcBqp0: BackgroundQueuePolicy STATUS_NONE */
  ARTNET_PC_BQP_ADVISORY = 0xe1,     /**< AcBqp1: BackgroundQueuePolicy STATUS_ADVISORY */
  ARTNET_PC_BQP_WARNING = 0xe2,      /**< AcBqp2: BackgroundQueuePolicy STATUS_WARNING */
  ARTNET_PC_BQP_ERROR = 0xe3,        /**< AcBqp3: BackgroundQueuePolicy STATUS_ERROR */
  ARTNET_PC_BQP_DISABLED = 0xe4,     /**< AcBqp4: BackgroundQueuePolicy Disabled */
  ARTNET_PC_BQP_USER5 = 0xe5,        /**< AcBqp5: BackgroundQueuePolicy user defined 5 */
  ARTNET_PC_BQP_USER6 = 0xe6,        /**< AcBqp6: BackgroundQueuePolicy user defined 6 */
  ARTNET_PC_BQP_USER7 = 0xe7,        /**< AcBqp7: BackgroundQueuePolicy user defined 7 */
  ARTNET_PC_BQP_USER8 = 0xe8,        /**< AcBqp8: BackgroundQueuePolicy user defined 8 */
  ARTNET_PC_BQP_USER9 = 0xe9,        /**< AcBqp9: BackgroundQueuePolicy user defined 9 */
  ARTNET_PC_BQP_USER10 = 0xea,       /**< AcBqp10: BackgroundQueuePolicy user defined 10 */
  ARTNET_PC_BQP_USER11 = 0xeb,       /**< AcBqp11: BackgroundQueuePolicy user defined 11 */
  ARTNET_PC_BQP_USER12 = 0xec,       /**< AcBqp12: BackgroundQueuePolicy user defined 12 */
  ARTNET_PC_BQP_USER13 = 0xed,       /**< AcBqp13: BackgroundQueuePolicy user defined 13 */
  ARTNET_PC_BQP_USER14 = 0xee,       /**< AcBqp14: BackgroundQueuePolicy user defined 14 */
  ARTNET_PC_BQP_USER15 = 0xef,       /**< AcBqp15: BackgroundQueuePolicy user defined 15 */
} artnet_port_command_t;


/*
 * An enum for the type of data transmitted on a port.
 * As far as I know, only DMX-512 is supported
 */
typedef enum  {
  ARTNET_PORT_DMX = 0x00,    /**< Data is DMX-512 */
  ARTNET_PORT_MIDI = 0x01,  /**< Data is MIDI */
  ARTNET_PORT_AVAB = 0x02,  /**< Data is Avab */
  ARTNET_PORT_CMX = 0x03,    /**< Data is Colortran CMX */
  ARTNET_PORT_ADB = 0x04,    /**< Data is ABD 62.5 */
  ARTNET_PORT_ARTNET = 0x05  /**< Data is ArtNet */
} artnet_port_data_code;


// defines the status of the firmware transfer
typedef enum  {
  ARTNET_FIRMWARE_BLOCKGOOD = 0x00,
  ARTNET_FIRMWARE_ALLGOOD = 0x01,
  ARTNET_FIRMWARE_FAIL = 0xff,
} artnet_firmware_status_code;

// tod actions
typedef enum  {
  ARTNET_TOD_FULL = 0x00,
  ARTNET_TOD_FLUSH = 0x01,
  ARTNET_TOD_END = 0x02,
  ARTNET_TOD_INC_ON = 0x03,
  ARTNET_TOD_INC_OFF = 0x04,
} artnet_tod_command_code;


/**
 * An enum for refering to a particular input or output port.
 */
typedef enum {
  ARTNET_INPUT_PORT = 1,    /**< The input port */
  ARTNET_OUTPUT_PORT,      /**< The output port */
} artnet_port_dir_t;


/*
 * Enum describing the type of node
 */
typedef enum {
  ARTNET_SRV,      /**< An ArtNet server (transmitts DMX data) */
  ARTNET_NODE,    /**< An ArtNet node   (dmx reciever) */
  ARTNET_MSRV,    /**< A Media Server */
  ARTNET_ROUTE,    /**< No Effect currently */
  ARTNET_BACKUP,    /**< No Effect currently */
  ARTNET_RAW      /**< Raw Node - used for diagnostics */
} artnet_node_type;


/*
 * Enum for the talk-to-me value
 * These values can be &'ed togeather, so for example to set private replies
 * and auto replying use :
 *   (ARTNET_TTM_PRIVATE & ARTNET_TTM_AUTO)
 *
 * Art-Net 4: This is now mapped to the Flags field in ArtPoll.
 */
typedef enum {
  ARTNET_TTM_DEFAULT = 0xFF,  /**< default, ArtPollReplies are broadcast, and nodes won't send a ArtPollReply when conditions change */
  ARTNET_TTM_PRIVATE = 0xFE,  /**< ArtPollReplies aren't broadcast */
  ARTNET_TTM_AUTO = 0xFD    /**< ArtPollReplies are send when node conditions chang */
} artnet_ttm_value_t;

/*
 * ArtPoll Flags bit masks (Art-Net 4)
 * Bit numbering follows the spec (1-indexed): bit 1 = 0x02, bit 2 = 0x04, etc.
 */
typedef enum {
  ARTNET_POLL_FLAG_UNICAST_DEPRECATED = 0x01,  /**< bit 0 (deprecated): was private replies */
  ARTNET_POLL_FLAG_REPLY_ON_CHANGE   = 0x02,  /**< bit 1: send ArtPollReply on condition change */
  ARTNET_POLL_FLAG_DIAG_ENABLE       = 0x04,  /**< bit 2: send diagnostics to me */
  ARTNET_POLL_FLAG_DIAG_UNICAST      = 0x08,  /**< bit 3: unicast diagnostics (if bit 2 set) */
  ARTNET_POLL_FLAG_VLC_DISABLE       = 0x10,  /**< bit 4: disable VLC transmission */
  ARTNET_POLL_FLAG_TARGET_MODE       = 0x20,  /**< bit 5: enable target mode filtering */
} artnet_poll_flags_t;

/*
 * DiagPriority codes (Art-Net 4, Table 5)
 */
typedef enum {
  ARTNET_DIAG_LOW      = 0x10,  /**< low priority diagnostic message */
  ARTNET_DIAG_MEDIUM   = 0x40,  /**< medium priority diagnostic message */
  ARTNET_DIAG_HIGH     = 0x80,  /**< high priority diagnostic message */
  ARTNET_DIAG_CRITICAL = 0xe0,  /**< critical priority diagnostic message */
  ARTNET_DIAG_VOLATILE = 0xf0,  /**< volatile diagnostic message */
} artnet_diag_priority_t;

/*
 * ArtPollReply Status2 bit masks (Art-Net 4)
 */
typedef enum {
  ARTNET_STATUS2_WEB_CONFIG       = 0x01,  /**< bit 0: supports web browser configuration */
  ARTNET_STATUS2_DHCP_ENABLED     = 0x02,  /**< bit 1: IP is DHCP configured */
  ARTNET_STATUS2_DHCP_SUPPORTED   = 0x04,  /**< bit 2: supports DHCP */
  ARTNET_STATUS2_15BIT_ADDR       = 0x08,  /**< bit 3: supports 15-bit port addressing */
  ARTNET_STATUS2_SACN_SWITCHABLE  = 0x10,  /**< bit 4: can switch between Art-Net and sACN */
  ARTNET_STATUS2_Sounding         = 0x20,  /**< bit 5: sounding alarm */
  ARTNET_STATUS2_STYLE_SWITCH     = 0x40,  /**< bit 6: supports output style switching */
  ARTNET_STATUS2_RDM_CONTROL      = 0x80,  /**< bit 7: supports RDM control via ArtAddress */
} artnet_status2_t;

/*
 * ArtPollReply Status3 bit masks (Art-Net 4)
 */
typedef enum {
  ARTNET_STATUS3_FAILSAFE_SHIFT = 6,
  ARTNET_STATUS3_FAILSAFE_MASK  = 0xC0,  /**< bits 7-6: fail-safe mode */
  ARTNET_STATUS3_FAILOVER       = 0x20,  /**< bit 5: supports programmable fail-safe */
  ARTNET_STATUS3_LLRP           = 0x10,  /**< bit 4: supports LLRP */
  ARTNET_STATUS3_PORT_DIRECTION = 0x08,  /**< bit 3: supports port direction switching */
  ARTNET_STATUS3_RDMNET         = 0x04,  /**< bit 2: supports RDMnet */
  ARTNET_STATUS3_BACKGROUND_QUEUE = 0x02, /**< bit 1: BackgroundQueue is supported */
  ARTNET_STATUS3_BG_DISCOVERY_CTRL = 0x01, /**< bit 0: background discovery can be disabled by ArtAddress */
} artnet_status3_t;

/*
 * ArtPollReply Status3 fail-safe mode values
 */
typedef enum {
  ARTNET_FAILSAFE_HOLD  = 0x00,  /**< hold last state on data loss */
  ARTNET_FAILSAFE_ZERO  = 0x40,  /**< zero all outputs on data loss */
  ARTNET_FAILSAFE_FULL  = 0x80,  /**< full output on data loss */
  ARTNET_FAILSAFE_SCENE = 0xC0,  /**< play fail-safe scene on data loss */
} artnet_failsafe_mode_t;

/*
 * ArtPollReply GoodOutputB bit masks (Art-Net 4)
 */
typedef enum {
  ARTNET_GOODB_RDM_DISABLED    = 0x80,  /**< bit 7: RDM is disabled */
  ARTNET_GOODB_STYLE_CONSTANT  = 0x40,  /**< bit 6: output style is constant */
  ARTNET_GOODB_STYLE_DELTA     = 0x00,  /**< bit 6 clr: output style is delta */
  ARTNET_GOODB_DISCOVERY_IDLE  = 0x20,  /**< bit 5: discovery is currently not running */
  ARTNET_GOODB_BG_DISCOVERY_DISABLED = 0x10, /**< bit 4: background discovery is disabled */
} artnet_good_output_b_t;

/*
 * ArtTimeCode type (Art-Net 4)
 */
typedef enum {
  ARTNET_TIMECODE_FILM  = 0x00,  // 24 fps
  ARTNET_TIMECODE_EBU   = 0x01,  // 25 fps
  ARTNET_TIMECODE_DF    = 0x02,  // 29.97 fps drop-frame
  ARTNET_TIMECODE_SMPTE = 0x03,  // 30 fps
} artnet_timecode_type_t;

/*
 * ArtTrigger key values (Art-Net 4)
 */
typedef enum {
  ARTNET_TRIGGER_KEY_ASCII = 0x00,
  ARTNET_TRIGGER_KEY_MACRO = 0x01,
  ARTNET_TRIGGER_KEY_SOFT  = 0x02,
  ARTNET_TRIGGER_KEY_SHOW  = 0x03,
} artnet_trigger_key_t;

/*
 * NodeReport codes (Art-Net 4, Table 3)
 * Returned in ArtPollReply NodeReport field.
 */
typedef enum {
  ARTNET_RC_DEBUG        = 0x0000,  /**< RcDebug: booted in debug mode */
  ARTNET_RC_POWER_OK     = 0x0001,  /**< RcPowerOk: power-on test passed */
  ARTNET_RC_POWER_FAIL   = 0x0002,  /**< RcPowerFail: power-on hardware test failed */
  ARTNET_RC_SOCKET_WR1   = 0x0003,  /**< RcSocketWr1: last UDP failed due to length truncation */
  ARTNET_RC_PARSE_FAIL   = 0x0004,  /**< RcParseFail: unrecognised last UDP */
  ARTNET_RC_UDP_FAIL     = 0x0005,  /**< RcUdpFail: cannot open UDP socket */
  ARTNET_RC_SHNAME_OK    = 0x0006,  /**< RcShNameOk: short name programmed successfully */
  ARTNET_RC_LONAME_OK    = 0x0007,  /**< RcLoNameOk: long name programmed successfully */
  ARTNET_RC_DMX_ERROR    = 0x0008,  /**< RcDmxError: DMX512 receive error detected */
  ARTNET_RC_DMX_UDP_FULL = 0x0009,  /**< RcDmxUdpFull: DMX transmit buffer exhausted */
  ARTNET_RC_DMX_RX_FULL  = 0x000a,  /**< RcDmxRxFull: DMX receive buffer exhausted */
  ARTNET_RC_SWITCH_ERR   = 0x000b,  /**< RcSwitchErr: universe address conflict */
  ARTNET_RC_CONFIG_ERR   = 0x000c,  /**< RcConfigErr: product config mismatch */
  ARTNET_RC_DMX_SHORT    = 0x000d,  /**< RcDmxShort: DMX output short detected */
  ARTNET_RC_FIRMWARE_FAIL= 0x000e,  /**< RcFirmwareFail: last firmware upload failed */
  ARTNET_RC_USER_FAIL    = 0x000f,  /**< RcUserFail: user changed locked address switch */
  ARTNET_RC_FACTORY_RES  = 0x0010,  /**< RcFactoryRes: factory reset performed */
} artnet_node_report_code;

/*
 * Style codes (Art-Net 4, Table 4)
 * Defines the type of Art-Net product. Returned in ArtPollReply.
 */
typedef enum {
  ARTNET_ST_NODE       = 0x00,  /**< StNode: DMX to/from Art-Net device */
  ARTNET_ST_CONTROLLER = 0x01,  /**< StController: lighting console */
  ARTNET_ST_MEDIA      = 0x02,  /**< StMedia: media server */
  ARTNET_ST_ROUTE      = 0x03,  /**< StRoute: network routing device */
  ARTNET_ST_BACKUP     = 0x04,  /**< StBackup: backup device */
  ARTNET_ST_CONFIG     = 0x05,  /**< StConfig: configuration/diagnostic tool */
  ARTNET_ST_VISUAL     = 0x06,  /**< StVisual: visualizer */
} artnet_style_code_t;

/*
 * ArtPollReply Status1 LED state values (Art-Net 4)
 */
typedef enum {
  ARTNET_LED_UNKNOWN  = 0x00,  /**< 00: LED state unknown */
  ARTNET_LED_LOCATE   = 0x01,  /**< 01: LED in locate/identify mode */
  ARTNET_LED_MUTE     = 0x02,  /**< 10: LED muted / disabled */
  ARTNET_LED_NORMAL   = 0x03,  /**< 11: LED normal operation */
} artnet_led_state_t;

/*
 * ArtDataRequest codes (Art-Net 4, Table 4a)
 * Used in ArtDataRequest and ArtDataReply packets.
 */
typedef enum {
  ARTNET_DR_POLL          = 0x0000,  /**< DrPoll: poll for ArtDataRequest support */
  ARTNET_DR_URL_PRODUCT   = 0x0001,  /**< DrUrlProduct: manufacturer product page URL */
  ARTNET_DR_URL_USER_GUIDE= 0x0002,  /**< DrUrlUserGuide: manufacturer user guide URL */
  ARTNET_DR_URL_SUPPORT   = 0x0003,  /**< DrUrlSupport: manufacturer support page URL */
  ARTNET_DR_URL_PERS_UDR  = 0x0004,  /**< DrUrlPersUdr: manufacturer UDR personality URL */
  ARTNET_DR_URL_PERS_GDTF = 0x0005,  /**< DrUrlPersGdtf: manufacturer GDTF personality URL */
} artnet_data_request_code;

/*
 * ArtAddress BackgroundQueuePolicy values (Art-Net 4)
 * Used in ArtPollReply and ArtAddress command 0xe0-0xef.
 */
typedef enum {
  ARTNET_BQP_NONE     = 0x00,  /**< collect using STATUS_NONE */
  ARTNET_BQP_ADVISORY = 0x01,  /**< collect using STATUS_ADVISORY */
  ARTNET_BQP_WARNING  = 0x02,  /**< collect using STATUS_WARNING */
  ARTNET_BQP_ERROR    = 0x03,  /**< collect using STATUS_ERROR */
  ARTNET_BQP_DISABLED = 0x04,  /**< background discovery collection disabled */
} artnet_bg_queue_policy_t;

/*
 * ArtCommand text commands (Art-Net 4, Table 6)
 * Manufacturer-specific commands use EstaMan = 0xFFFF.
 */
#define ARTNET_CMD_SWOUT_TEXT "SwoutText="
#define ARTNET_CMD_SWIN_TEXT  "SwinText="

/**
 * Enums for the application defined handlers
 */
typedef enum {
  ARTNET_RECV_HANDLER,    /**< Called on reciept of any ArtNet packet */
  ARTNET_SEND_HANDLER,    /**< Called on transmission of any ArtNet packet */
  ARTNET_POLL_HANDLER,    /**< Called on reciept of an ArtPoll packet */
  ARTNET_REPLY_HANDLER,    /**< Called on reciept of an ArtPollReply packet */
  ARTNET_DMX_HANDLER,      /**< Called on reciept of an ArtDMX packet */
  ARTNET_ADDRESS_HANDLER,    /**< Called on reciept of an ArtAddress packet */
  ARTNET_INPUT_HANDLER,    /**< Called on reciept of an ArtInput packet */
  ARTNET_TOD_REQUEST_HANDLER,  /**< Called on reciept of an ArtTodRequest packet */
  ARTNET_TOD_DATA_HANDLER,  /**< Called on reciept of an ArtTodData packet */
  ARTNET_TOD_CONTROL_HANDLER,  /**< Called on reciept of an ArtTodControl packet */
  ARTNET_RDM_HANDLER,      /**< Called on reciept of an ArtRdm packet */
  ARTNET_IPPROG_HANDLER,    /**< Called on reciept of an ArtIPProg packet */
  ARTNET_FIRMWARE_HANDLER,  /**< Called on reciept of an ArtFirmware packet */
  ARTNET_FIRMWARE_REPLY_HANDLER,  /**< Called on reciept of an ArtFirmwareReply packet */
  ARTNET_SYNC_HANDLER,            /**< Called on reciept of an ArtSync packet */
  ARTNET_NZS_HANDLER,             /**< Called on reciept of an ArtNzs packet */
  ARTNET_COMMAND_HANDLER,         /**< Called on reciept of an ArtCommand packet */
  ARTNET_TIMECODE_HANDLER,        /**< Called on reciept of an ArtTimeCode packet */
  ARTNET_TIMESYNC_HANDLER,        /**< Called on reciept of an ArtTimeSync packet */
  ARTNET_TRIGGER_HANDLER,         /**< Called on reciept of an ArtTrigger packet */
  ARTNET_DIRECTORY_HANDLER,       /**< Called on reciept of an ArtDirectory packet */
  ARTNET_DIRECTORY_REPLY_HANDLER, /**< Called on reciept of an ArtDirectoryReply packet */
  ARTNET_FILE_TN_MASTER_HANDLER,  /**< Called on reciept of an ArtFileTnMaster packet */
  ARTNET_FILE_FN_MASTER_HANDLER,  /**< Called on reciept of an ArtFileFnMaster packet */
  ARTNET_FILE_FN_REPLY_HANDLER,   /**< Called on reciept of an ArtFileFnReply packet */
  ARTNET_MEDIAPATCH_HANDLER,      /**< Called on reciept of an ArtMediaPatch packet */
  ARTNET_MEDIA_HANDLER,           /**< Called on reciept of an ArtMedia packet */
  ARTNET_MEDIACONTROL_HANDLER,    /**< Called on reciept of an ArtMediaControl packet */
  ARTNET_DATAREQUEST_HANDLER,     /**< Called on reciept of an ArtDataRequest packet */
  ARTNET_DATAREPLY_HANDLER,       /**< Called on reciept of an ArtDataReply packet */
} artnet_handler_name_t;


/*
 * Describes a remote ArtNet node that has been discovered
 */
typedef struct artnet_node_entry_s {
  uint8_t ip[ARTNET_IP_SIZE];  /**< The IP address, Network byte ordered*/
  int16_t ver;          /**< The firmware version */
  uint8_t netSwitch;    /**< The net address (bits 14-8 of 15-bit port address) */
  uint8_t subSwitch;    /**< The subnet address (bits 7-4 of 15-bit port address, stored in lower nibble per Art-Net 4) */
  int16_t oem;          /**< The OEM value */
  uint8_t ubea;          /**< The UBEA version */
  uint8_t status;
  uint8_t estaMan[ARTNET_ESTA_SIZE];       /**< The ESTA Manufacturer code */
  uint8_t shortName[ARTNET_SHORT_NAME_LENGTH];  /**< The short node name */
  uint8_t longName[ARTNET_LONG_NAME_LENGTH];  /**< The long node name */
  uint8_t nodeReport[ARTNET_REPORT_LENGTH];  /**< The node report */
  int16_t numbports;        /**< The number of ports */
  uint8_t portTypes[ARTNET_MAX_PORTS];    /**< The type of ports */
  uint8_t goodInput[ARTNET_MAX_PORTS];
  uint8_t goodOutputA[ARTNET_MAX_PORTS];   /**< GoodOutputA status (Art-Net 4) */
  uint8_t swIn[ARTNET_MAX_PORTS];
  uint8_t swOut[ARTNET_MAX_PORTS];
  uint8_t acnPriority;      /**< sACN priority value (Art-Net 4, was swvideo) */
  uint8_t swMacro;
  uint8_t swRemote;
  uint8_t style;                        /**< Product style code (StNode, StController, etc.) */
  uint8_t mac[ARTNET_MAC_SIZE];        /**< The MAC address of the node */
  uint8_t bindIp[ARTNET_IP_SIZE];      /**< Bind IP address (Art-Net 4) */
  uint8_t bindIndex;                    /**< BindIndex (Art-Net 4) */
  uint8_t status2;                      /**< Status2 register (Art-Net 4) */
  uint8_t goodOutputB[ARTNET_MAX_PORTS]; /**< GoodOutputB status (Art-Net 4) */
  uint8_t status3;                      /**< Status3 register (Art-Net 4) */
  uint8_t defaultRespUid[ARTNET_RDM_UID_WIDTH]; /**< Default responder UID (Art-Net 4) */
  uint8_t userHi;                       /**< User specific data high byte (Art-Net 4) */
  uint8_t userLo;                       /**< User specific data low byte (Art-Net 4) */
  uint16_t refreshRate;                 /**< Max DMX refresh rate in Hz (Art-Net 4) */
  uint8_t bgQueuePolicy;                /**< BackgroundQueuePolicy (Art-Net 4) */
} artnet_node_entry_t;

/** A pointer to an artnet_node_entry_t */
typedef artnet_node_entry_t *artnet_node_entry;

typedef struct {
  char shortName[ARTNET_SHORT_NAME_LENGTH];
  char longName[ARTNET_LONG_NAME_LENGTH];
  uint8_t netSwitch;
  uint8_t subSwitch;
  uint8_t inPorts[ARTNET_MAX_PORTS];
  uint8_t outPorts[ARTNET_MAX_PORTS];
} artnet_node_config_t;


/** The local ArtNet node */
typedef void *artnet_node;

/** A list of remote ArtNet nodes */
typedef void *artnet_node_list;

#if !defined(WIN32) && !defined(_MSC_VER)
typedef int artnet_socket_t;
#else
typedef SOCKET artnet_socket_t;
#endif

// node control functions
EXTERN artnet_node artnet_new(const char *ip, int verbose);
EXTERN int artnet_setoem(artnet_node vn, uint8_t hi, uint8_t lo);
EXTERN int artnet_setesta(artnet_node vn, char hi, char lo);
EXTERN int artnet_set_bcast_limit(artnet_node vn, int limit);
EXTERN int artnet_start(artnet_node n);
EXTERN int artnet_read(artnet_node n, int timeout);
EXTERN int artnet_stop(artnet_node n);
EXTERN int artnet_destroy(artnet_node n);

int artnet_join(artnet_node vn1, artnet_node vn2);

// handler functions
// these need to be cleaned up into a generic interface
EXTERN int artnet_set_handler(artnet_node vn,
  artnet_handler_name_t handler,
  int (*fh)(artnet_node n, void *pp, void *d),
  void* data);
EXTERN int artnet_set_dmx_handler(artnet_node vn,
  int (*fh)(artnet_node n, int port, void *d),
  void *data);
EXTERN int artnet_set_program_handler(artnet_node vn,
  int (*fh)(artnet_node n, void *d),
  void *data);
EXTERN int artnet_set_firmware_handler(artnet_node vn,
  int (*fh)(artnet_node n, int ubea, uint16_t *data, int length, void *d),
  void *data);
EXTERN int artnet_set_rdm_handler(artnet_node vn,
  int (*fh)(artnet_node n, int address, uint8_t *rdm, int length, void *d),
  void *data);
EXTERN int artnet_set_rdm_initiate_handler(artnet_node vn,
  int (*fh)(artnet_node n, int port, void *d),
  void *data);
EXTERN int artnet_set_rdm_tod_handler(artnet_node vn,
  int (*fh)(artnet_node n, int address, void *d),
  void *data);

// send functions
EXTERN int artnet_send_poll(artnet_node n,
  const char *ip,
  artnet_ttm_value_t talk_to_me);
EXTERN int artnet_send_poll_reply(artnet_node n);
EXTERN int artnet_send_dmx(artnet_node n,
  int port_id,
  int16_t length,
  const uint8_t *data);
EXTERN int artnet_raw_send_dmx(artnet_node vn,
  uint16_t uni,
  int16_t length,
  const uint8_t *data);
EXTERN int artnet_send_address(artnet_node n,
  artnet_node_entry e,
  const char *shortName,
  const char *longName,
  uint8_t inAddr[ARTNET_MAX_PORTS],
  uint8_t outAddr[ARTNET_MAX_PORTS],
  uint8_t netAddr,
  uint8_t subAddr,
  artnet_port_command_t cmd);
EXTERN int artnet_send_input(artnet_node n,
  artnet_node_entry e,
  uint8_t settings[ARTNET_MAX_PORTS]);
EXTERN int artnet_send_firmware(artnet_node vn,
  artnet_node_entry e,
  int ubea,
  uint16_t *data,
  int length,
  int (*fh)(artnet_node n, artnet_firmware_status_code code, void *d),
  void *user_data);
EXTERN int artnet_send_firmware_reply(artnet_node vn,
  artnet_node_entry e,
  artnet_firmware_status_code code);

// rdm functions
EXTERN int artnet_send_tod_request(artnet_node vn);
EXTERN int artnet_send_tod_control(artnet_node vn,
  uint16_t address,
  artnet_tod_command_code action);
EXTERN int artnet_send_tod_data(artnet_node vn, int port);
EXTERN int artnet_send_rdm(artnet_node vn,
  uint16_t address,
  uint8_t *data,
  int length);
EXTERN int artnet_send_rdmsub(artnet_node vn,
  uint8_t uid[ARTNET_RDM_UID_WIDTH],
  uint8_t command_class,
  uint16_t param_id,
  uint16_t sub_device,
  uint16_t sub_count,
  uint8_t *data,
  int length);

// sync functions
EXTERN int artnet_send_sync(artnet_node vn);

// non-zero start code DMX
EXTERN int artnet_send_nzs(artnet_node vn, int port_id, uint8_t start_code,
  int16_t length, const uint8_t *data);

// timecode functions
EXTERN int artnet_send_timecode(artnet_node vn, uint8_t frames, uint8_t seconds,
  uint8_t minutes, uint8_t hours, artnet_timecode_type_t type, uint8_t stream_id);
EXTERN int artnet_send_timesync(artnet_node vn, uint8_t tm_sec, uint8_t tm_min,
  uint8_t tm_hour, uint8_t tm_mday, uint8_t tm_mon, uint8_t tm_year);

// trigger functions
EXTERN int artnet_send_trigger(artnet_node vn, uint8_t oem_hi, uint8_t oem_lo,
  uint8_t key, uint8_t sub_key, const uint8_t *data, int16_t length);

// data request/reply functions
EXTERN int artnet_send_data_reply(artnet_node vn, const char *ip,
  uint16_t request_code, const char *payload, int16_t length);

// diagnostic functions
EXTERN int artnet_send_diagnostic(artnet_node vn,
  artnet_diag_priority_t priority,
  uint8_t port,
  const char *text);
EXTERN int artnet_add_rdm_device(artnet_node vn,
  int port,
  uint8_t uid[ARTNET_RDM_UID_WIDTH]);
EXTERN int artnet_add_rdm_devices(artnet_node vn,
  int port,
  uint8_t *uid,
  int count);
EXTERN int artnet_remove_rdm_device(artnet_node vn,
  int port,
  uint8_t uid[ARTNET_RDM_UID_WIDTH]);

// recv functions
EXTERN uint8_t *artnet_read_dmx(artnet_node n, int port_id, int *length);

// state changing functions
EXTERN int artnet_set_node_type(artnet_node n, artnet_node_type type);
EXTERN int artnet_set_style_code(artnet_node vn, artnet_style_code_t style);
EXTERN int artnet_set_status2(artnet_node vn, uint8_t status2);
EXTERN int artnet_set_short_name(artnet_node vn, const char *name);
EXTERN int artnet_set_long_name(artnet_node n, const char *name);

//port manipulation functions
EXTERN int artnet_set_port_type(artnet_node n,
                                int id,
                                artnet_port_settings_t settings,
                                artnet_port_data_code data);
EXTERN int artnet_set_port_addr(artnet_node n,
                                int id,
                                artnet_port_dir_t dir,
                                uint8_t addr);
EXTERN int artnet_set_net_addr(artnet_node n, uint8_t net);
EXTERN int artnet_set_subnet_addr(artnet_node n, uint8_t subnet);
EXTERN int artnet_set_default_resp_uid(artnet_node n, const uint8_t uid[ARTNET_RDM_UID_WIDTH]);
EXTERN int artnet_set_gateway(artnet_node n, const char *ip);
EXTERN int artnet_get_universe_addr(artnet_node n,
                                    int id,
                                    artnet_port_dir_t dir);

//node list functions
EXTERN artnet_node_list artnet_get_nl(artnet_node n);
EXTERN artnet_node_entry artnet_nl_first(artnet_node_list nl);
EXTERN artnet_node_entry artnet_nl_next(artnet_node_list nl);
EXTERN int artnet_nl_get_length(artnet_node_list nl);
EXTERN int artnet_nl_foreach(artnet_node n,
  int (*cb)(artnet_node_entry entry, void *data), void *data);

// misc
EXTERN int artnet_dump_config(artnet_node n);
EXTERN int artnet_get_config(artnet_node n, artnet_node_config_t *config);
EXTERN artnet_socket_t artnet_get_sd(artnet_node n);
EXTERN int artnet_set_fdset(artnet_node vn, fd_set *fdset);

EXTERN char *artnet_strerror();

#ifdef __cplusplus
}
#endif

#endif
