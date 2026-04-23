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
 * transmit.c
 * Functions to handle sending datagrams
 * Copyright (C) 2004-2005 Simon Newton
 */

#include "private.h"

/*
 * Send an art poll
 *
 * @param ip the ip address to send to
 * @param ttm the talk to me value, either ARTNET_TTM_DEFAULT,
 *   ARTNET_TTM_PRIVATE or ARTNET_TTM_AUTO
 */
int artnet_tx_poll(node n, const char *ip, artnet_ttm_value_t ttm) {
  artnet_packet_t p;
  int ret;

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    if (ip) {
      ret = artnet_net_inet_aton(ip, &p.to);
      if (ret)
        return ret;
    } else {
      p.to.s_addr = n->state.bcast_addr.s_addr;
    }

    memcpy(&p.data.ap.id, ARTNET_STRING, ARTNET_STRING_SIZE);
    p.data.ap.opCode = htols(ARTNET_POLL);
    p.data.ap.verH = 0;
    p.data.ap.ver = ARTNET_VERSION;
    // artnet_ttm_value_t uses inverted logic (~ttm) to map to artnet_poll_flags_t bits:
    //   TTM_PRIVATE (0xFE) -> ~0xFE = 0x01 -> ARTNET_POLL_FLAG_UNICAST_DEPRECATED
    //   TTM_AUTO    (0xFD) -> ~0xFD = 0x02 -> ARTNET_POLL_FLAG_REPLY_ON_CHANGE
    p.data.ap.flags = (artnet_poll_flags_t)~ttm;
    p.data.ap.diagPriority = ARTNET_DIAG_LOW;

    p.length = sizeof(artnet_poll_t);
    return artnet_net_send(n, &p);

  } else {
    artnet_error("Not sending poll, not a server or raw device");
    return ARTNET_EACTION;
  }
}

/*
 * Send an ArtPollReply
 * @param n the node
 * @param response true if this reply is in response to a network packet
 *            false if this reply is due to the node changing it's conditions
 */
int artnet_tx_poll_reply(node n, int response) {
  artnet_packet_t reply;
  int i;

  if (!response && n->state.mode == ARTNET_ON) {
    n->state.ar_count++;
  }

  reply.to = n->state.reply_addr;
  reply.type = ARTNET_REPLY;
  reply.length = sizeof(artnet_reply_t);

  // copy from a poll reply template
  memcpy(&reply.data, &n->ar_temp, sizeof(artnet_reply_t));

  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    reply.data.ar.goodinput[i] = n->ports.in[i].port_status;
    reply.data.ar.goodOutputA[i] = n->ports.out[i].port_status;
  }

  snprintf((char *) &reply.data.ar.nodereport,
           sizeof(reply.data.ar.nodereport),
           "%04x [%04i] libartnet",
           n->state.report_code,
           n->state.ar_count);

  return artnet_net_send(n, &reply);
}


/*
 * Send a tod request
 */
int artnet_tx_tod_request(node n) {
  int i;
  artnet_packet_t todreq;

  todreq.to = n->state.bcast_addr;
  todreq.type = ARTNET_TODREQUEST;
  todreq.length = sizeof(artnet_todrequest_t);
  memset(&todreq.data,0x00, todreq.length);

  // set up the data
  memcpy(&todreq.data.todreq.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  todreq.data.todreq.opCode = htols(ARTNET_TODREQUEST);
  todreq.data.todreq.verH = 0;
  todreq.data.todreq.ver = ARTNET_VERSION;
  todreq.data.todreq.command = ARTNET_TOD_FULL; // todfull
  todreq.data.todreq.adCount = 0;

  // include all enabled ports
  for (i=0; i < ARTNET_MAX_PORTS; i++) {
    if (n->ports.out[i].port_enabled) {
      todreq.data.todreq.address[todreq.data.todreq.adCount++] = addr_port(n->ports.out[i].port_addr);
    }
  }

  return artnet_net_send(n, &todreq);
}


/*
 * Send a tod data for port number id
 * @param id the number of the port to send data for
 */
int artnet_tx_tod_data(node n, int id) {
  artnet_packet_t tod;
  int lim, remaining, bloc, offset;
  int ret = ARTNET_EOK;

  // ok we need to check how many uid's we have,
  // may need to send more than one datagram

  tod.to = n->state.bcast_addr;
  tod.type = ARTNET_TODDATA;
  tod.length = sizeof(artnet_toddata_t);

  memset(&tod.data,0x00, tod.length);

  // set up the data
  memcpy(&tod.data.toddata.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  tod.data.toddata.opCode = htols(ARTNET_TODDATA);
  tod.data.toddata.verH = 0;
  tod.data.toddata.ver = ARTNET_VERSION;
  tod.data.toddata.rdmVer = ARTNET_RDM_VERSION;
  tod.data.toddata.port = id;

  // this is interesting, the spec mentions TOD_ADD and TOD_SUBTRACT, but the
  // codes aren't given. The windows drivers don't have these either....
  tod.data.toddata.cmdRes = ARTNET_TOD_FULL;

  tod.data.toddata.address = addr_port(n->ports.out[id].port_addr);
  tod.data.toddata.net = addr_net(n->ports.out[id].port_addr);
  tod.data.toddata.uidTotalHi = short_get_high_byte(n->ports.out[id].port_tod.length);
  tod.data.toddata.uidTotal = short_get_low_byte(n->ports.out[id].port_tod.length);

  remaining = n->ports.out[id].port_tod.length;
  bloc = 0;

  while (remaining > 0) {
    memset(&tod.data.toddata.tod,0x00, ARTNET_MAX_UID_COUNT * ARTNET_RDM_UID_WIDTH);
    lim = min(ARTNET_MAX_UID_COUNT, remaining);
    tod.data.toddata.blockCount = bloc++;
    tod.data.toddata.uidCount = lim;

    offset = (n->ports.out[id].port_tod.length - remaining) * ARTNET_RDM_UID_WIDTH;
    if (n->ports.out[id].port_tod.data != NULL)
      memcpy(tod.data.toddata.tod,
             n->ports.out[id].port_tod.data + offset,
             lim * ARTNET_RDM_UID_WIDTH);

    ret = ret || artnet_net_send(n, &tod);
    remaining = remaining - lim;
  }
  return ret;
}


/*
 * Send a tod data for port number id
 * @param id the number of the port to send data for
 */
int artnet_tx_tod_control(node n,
                          uint16_t address,
                          artnet_tod_command_code action) {
  artnet_packet_t tod;

  tod.to = n->state.bcast_addr;
  tod.type = ARTNET_TODCONTROL;
  tod.length = sizeof(artnet_todcontrol_t);

  memset(&tod.data,0x00, tod.length);

  // set up the data
  memcpy(&tod.data.todcontrol.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  tod.data.todcontrol.opCode = htols(ARTNET_TODCONTROL);
  tod.data.todcontrol.verH = 0;
  tod.data.todcontrol.ver = ARTNET_VERSION;
  tod.data.todcontrol.cmd = action;
  tod.data.todcontrol.address = addr_port(address);
  tod.data.todcontrol.net = addr_net(address);

  return artnet_net_send(n, &tod);
}


/*
 * Send a RDM message
 * @param address the universe to address this datagram to
 * @param action the action to perform. Either ARTNET_TOD_FULL or
 *   ARTNET_TOD_FLUSH
 */
int artnet_tx_rdm(node n, uint16_t address, uint8_t *data, int length) {
  artnet_packet_t rdm;
  int len;

  // Art-Net 4: ArtRdm must always be unicast
  rdm.to = n->state.rdm_reply_addr;
  rdm.type = ARTNET_RDM;

  memset(&rdm.data, 0x00, sizeof(artnet_rdm_t));

  // set up the data
  memcpy(&rdm.data.rdm.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  rdm.data.rdm.opCode = htols(ARTNET_RDM);
  rdm.data.rdm.verH = 0;
  rdm.data.rdm.ver = ARTNET_VERSION;
  rdm.data.rdm.rdmVer = ARTNET_RDM_VERSION;
  rdm.data.rdm.cmd = 0x00;
  rdm.data.rdm.address = addr_port(address);
  rdm.data.rdm.net = addr_net(address);

  len = min(length, ARTNET_MAX_RDM_DATA);
  memcpy(&rdm.data.rdm.data, data, len);

  // send only actual header + data, not full 512-byte padding
  rdm.length = sizeof(artnet_rdm_t) - ARTNET_MAX_RDM_DATA + len;

  return artnet_net_send(n, &rdm);

}


/*
 * Send a RDM sub packet
 */
int artnet_tx_rdmsub(node n,
                     uint8_t uid[ARTNET_RDM_UID_WIDTH],
                     uint8_t command_class, uint16_t param_id,
                     uint16_t sub_device, uint16_t sub_count,
                     uint8_t *data, int length) {
  artnet_packet_t rdmsub;
  int len;

  rdmsub.to = n->state.rdm_reply_addr;
  rdmsub.type = ARTNET_RDMSUB;

  memset(&rdmsub.data, 0x00, sizeof(artnet_rdm_sub_t));

  memcpy(&rdmsub.data.rdmsub.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  rdmsub.data.rdmsub.opCode = htols(ARTNET_RDMSUB);
  rdmsub.data.rdmsub.verH = 0;
  rdmsub.data.rdmsub.ver = ARTNET_VERSION;
  rdmsub.data.rdmsub.rdmVer = ARTNET_RDM_VERSION;
  memcpy(&rdmsub.data.rdmsub.uid, uid, ARTNET_RDM_UID_WIDTH);
  rdmsub.data.rdmsub.commandClass = command_class;
  rdmsub.data.rdmsub.paramIdHi = short_get_high_byte(param_id);
  rdmsub.data.rdmsub.paramId = short_get_low_byte(param_id);
  rdmsub.data.rdmsub.subDeviceHi = short_get_high_byte(sub_device);
  rdmsub.data.rdmsub.subDevice = short_get_low_byte(sub_device);
  rdmsub.data.rdmsub.subCountHi = short_get_high_byte(sub_count);
  rdmsub.data.rdmsub.subCount = short_get_low_byte(sub_count);

  len = min(length, ARTNET_MAX_RDM_DATA);
  memcpy(&rdmsub.data.rdmsub.data, data, len);

  rdmsub.length = sizeof(artnet_rdm_sub_t) - ARTNET_MAX_RDM_DATA + len;

  return artnet_net_send(n, &rdmsub);
}


/*
 * Send a diagnostic data packet
 */
int artnet_tx_diagdata(node n, uint8_t priority, uint8_t logical_port,
                        const char *text) {
  artnet_packet_t diag;
  int text_len;

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  // Art-Net 4: only send if diagnostics are enabled
  if (!n->state.diag_enabled)
    return ARTNET_EACTION;

  // priority filtering: only send if >= minimum requested priority
  if (priority < n->state.diag_priority)
    return ARTNET_EOK;

  text_len = strlen(text);
  if (text_len > ARTNET_DMX_LENGTH - 1)
    text_len = ARTNET_DMX_LENGTH - 1;

  memset(&diag, 0x00, sizeof(diag));
  diag.type = ARTNET_DIAGDATA;
  diag.length = sizeof(artnet_diagdata_t) - (ARTNET_DMX_LENGTH - text_len - 1);

  memcpy(&diag.data.diagdata.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  diag.data.diagdata.opCode = htols(ARTNET_DIAGDATA);
  diag.data.diagdata.verH = 0;
  diag.data.diagdata.ver = ARTNET_VERSION;
  diag.data.diagdata.diagPriority = priority;
  diag.data.diagdata.logicalPort = logical_port;
  int text_len_total = text_len + 1;
  diag.data.diagdata.lengthHi = short_get_high_byte(text_len_total);
  diag.data.diagdata.length = short_get_low_byte(text_len_total);
  memcpy(&diag.data.diagdata.data, text, text_len);

  // unicast or broadcast depending on ArtPoll Flags
  if (n->state.diag_unicast) {
    diag.to = n->state.reply_addr;
  } else {
    diag.to.s_addr = n->state.bcast_addr.s_addr;
  }

  return artnet_net_send(n, &diag);
}


/*
 * Send a firmware reply
 * @param ip the ip address to send to
 * @param code the response code
 */
int artnet_tx_firmware_reply(node n, in_addr_t ip,
                             artnet_firmware_status_code code) {
  artnet_packet_t p;
  memset(&p, 0x0, sizeof(p));

  p.to.s_addr = ip;
  p.length = sizeof(artnet_firmware_t);
  p.type = ARTNET_FIRMWAREREPLY;

  // now build packet
  memcpy(&p.data.firmware.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.firmware.opCode = htols(ARTNET_FIRMWAREREPLY);
  p.data.firmware.verH = 0;
  p.data.firmware.ver = ARTNET_VERSION;
  p.data.firmware.type = code;

  return artnet_net_send(n, &p);
}


/*
 * Send an firmware data datagram
 *
 * @param firm a pointer to the firmware structure for this transfer
 */
int artnet_tx_firmware_packet(node n, firmware_transfer_t *firm) {
  artnet_packet_t p;
  uint8_t type = 0;
  int data_len, max_len, ret;

  memset(&p, 0x0, sizeof(p));

  // max value of data_len is 1024;
  max_len = ARTNET_FIRMWARE_SIZE * sizeof(p.data.firmware.data[0]);

  // calculate length
  data_len = firm->bytes_total - firm->bytes_current;
  data_len = min(data_len, max_len);

  // work out type - 6 cases
  if(firm->ubea) {
    // ubea upload
    if (firm->bytes_current == 0) {
      // first
      type = ARTNET_FIRMWARE_UBEAFIRST;
    } else if (data_len == max_len) {
      // cont
      type = ARTNET_FIRMWARE_UBEACONT;
    } else if (data_len < max_len) {
      // last
      type = ARTNET_FIRMWARE_UBEALAST;
    } else {
      // this should never happen, something has gone wrong
      artnet_error("Attempting to send %d when the max is %d, very very bad...\n", data_len, max_len);
    }
  } else {
    // firmware upload
    if (firm->bytes_current == 0) {
      // first
      type = ARTNET_FIRMWARE_FIRMFIRST;
    } else if (data_len == max_len) {
      // cont
      type = ARTNET_FIRMWARE_FIRMCONT;
    } else if (data_len < max_len) {
      // last
      type = ARTNET_FIRMWARE_FIRMLAST;
    } else {
      // this should never happen, something has gone wrong
      artnet_error("Attempting to send %d when the max is %d, very very bad...\n", data_len, max_len);
    }
  }

  // set packet properties
  p.to.s_addr = firm->peer.s_addr;
  p.length = sizeof(artnet_firmware_t);
  p.type = ARTNET_FIRMWAREMASTER;

  // now build packet
  memcpy(&p.data.firmware.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.firmware.opCode = htols(ARTNET_FIRMWAREMASTER);
  p.data.firmware.verH = 0;
  p.data.firmware.ver = ARTNET_VERSION;
  p.data.firmware.type =  type;
  p.data.firmware.blockId = firm->expected_block;

  artnet_misc_int_to_bytes(firm->bytes_total / sizeof(uint16_t),
                           p.data.firmware.length);

  memcpy(&p.data.firmware.data,
         firm->data + (firm->bytes_current / sizeof(uint16_t)),
         data_len);

  if ((ret = artnet_net_send(n, &p))) {
    // send failed
    return ret;
  } else {
    // update stats
    firm->bytes_current = firm->bytes_current + data_len;
    firm->last_time = time(NULL);
    firm->expected_block++;
    // limit between 0 and 255 (only 8 bits wide)
    // we dont' actually need this cause it will be shorted when assigned above
    firm->expected_block %= (UINT8_MAX + 1);
  }
  return ARTNET_EOK;
}


/*
 * Send an ArtSync packet
 */
int artnet_tx_sync(node n) {
  artnet_packet_t p;

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  memset(&p, 0x00, sizeof(p));
  p.to.s_addr = n->state.bcast_addr.s_addr;
  p.type = ARTNET_SYNC;
  p.length = sizeof(artnet_sync_t);

  memcpy(&p.data.asyn.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.asyn.opCode = htols(ARTNET_SYNC);
  p.data.asyn.verH = 0;
  p.data.asyn.ver = ARTNET_VERSION;

  return artnet_net_send(n, &p);
}


/*
 * Send an ArtDirectoryReply packet (empty directory)
 */
int artnet_tx_directory_reply(node n) {
  artnet_packet_t p;

  memset(&p, 0x00, sizeof(p));
  p.to = n->state.reply_addr;
  p.type = ARTNET_DIRECTORYREPLY;
  p.length = sizeof(artnet_directory_reply_t);

  memcpy(&p.data.dirr.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.dirr.opCode = htols(ARTNET_DIRECTORYREPLY);
  p.data.dirr.verH = 0;
  p.data.dirr.ver = ARTNET_VERSION;

  return artnet_net_send(n, &p);
}


/*
 * Send an ArtFileFnReply packet
 */
int artnet_tx_file_fn_reply(node n, uint8_t blockId, uint16_t totalLength,
                            uint8_t *data, int dataLen) {
  artnet_packet_t p;
  int len;

  memset(&p, 0x00, sizeof(p));
  p.to = n->state.reply_addr;
  p.type = ARTNET_FILEFNREPLY;

  memcpy(&p.data.filefnr.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.filefnr.opCode = htols(ARTNET_FILEFNREPLY);
  p.data.filefnr.verH = 0;
  p.data.filefnr.ver = ARTNET_VERSION;
  p.data.filefnr.blockId = blockId;
  p.data.filefnr.fileLengthHi = short_get_high_byte(totalLength);
  p.data.filefnr.fileLengthLo = short_get_low_byte(totalLength);

  len = min(dataLen, ARTNET_FIRMWARE_SIZE * (int)sizeof(uint16_t));
  memcpy(&p.data.filefnr.data, data, len);

  p.length = sizeof(artnet_file_fn_reply_t) - ARTNET_FIRMWARE_SIZE * sizeof(uint16_t) + len;

  return artnet_net_send(n, &p);
}


// this is called when the node's state changes to rebuild the
// artpollreply packet
int artnet_tx_build_art_poll_reply(node n) {
  int i;

  // shorten the amount we have to type
  artnet_reply_t *ar = &n->ar_temp;

  memset(ar, 0x00, sizeof(artnet_reply_t));

  memcpy(&ar->id, ARTNET_STRING, ARTNET_STRING_SIZE);
  ar->opCode = htols(ARTNET_REPLY);
  memcpy(&ar->ip, &n->state.ip_addr.s_addr, 4);
  ar->port = htols(ARTNET_PORT);
  ar->verH = 0;
  ar->ver = 0;
  ar->netSwitch = n->state.net;
  ar->sub = n->state.subnet;
  ar->oemH = n->state.oem_hi;
  ar->oem = n->state.oem_lo;
  ar->ubea = 0;
  // ar->status - recalc every time

  // ESTA Manufacturer ID
  ar->etsaman[0] = n->state.esta_hi;
  ar->etsaman[1] = n->state.esta_lo;

  memcpy(&ar->shortname, &n->state.short_name, sizeof(n->state.short_name));
  memcpy(&ar->longname, &n->state.long_name, sizeof(n->state.long_name));

  // port stuff here
  ar->numbportsH = 0;

  for (i = ARTNET_MAX_PORTS; i > 0; i--) {
    if (n->ports.out[i-1].port_enabled || n->ports.in[i-1].port_enabled)
      break;
  }

  ar->numbports = i;

  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    ar->porttypes[i] = n->ports.types[i];
    ar->goodinput[i] = n->ports.in[i].port_status;
    ar->goodOutputA[i] = n->ports.out[i].port_status;
    ar->swin[i] = addr_port(n->ports.in[i].port_addr);
    ar->swout[i] = addr_port(n->ports.out[i].port_addr);
  }

  ar->acnPriority = 0;
  ar->swmacro = 0;
  ar->swremote = 0;

  // spares
  ar->sp1 = 0;
  ar->sp2 = 0;

  // hw address
  memcpy(&ar->mac, &n->state.hw_addr, ARTNET_MAC_SIZE);

  // bind index: 1 = root device
  ar->bindIndex = 1;

  // Status1: LED state in bits 7-6
  ar->status = n->state.led_state << 6;

  // Status3: fail-safe mode in bits 7-6
  ar->status3 = n->state.failsafe_mode;

  // GoodOutputB: RDM disabled (bit 7), output style (bit 6)
  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    ar->goodOutputB[i] = 0;
    if (!n->ports.out[i].rdm_enabled)
      ar->goodOutputB[i] |= ARTNET_GOODB_RDM_DISABLED;
    if (n->ports.out[i].output_style)
      ar->goodOutputB[i] |= ARTNET_GOODB_STYLE_CONSTANT;
  }

  return ARTNET_EOK;
}
