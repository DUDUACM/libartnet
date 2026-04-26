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
  artnet_packet_t p = {0};
  int ret = 0;

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    if (ip) {
      ret = artnet_net_inet_aton(ip, &p.to);
      if (ret) {
        return ret;
      }
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

    // Art-Net 4: ESTA manufacturer and OEM code
    p.data.ap.estaMan[0] = n->state.esta_hi;
    p.data.ap.estaMan[1] = n->state.esta_lo;
    p.data.ap.oem[0] = n->state.oem_hi;
    p.data.ap.oem[1] = n->state.oem_lo;

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
  artnet_packet_t reply = {0};
  int i = 0;

  n->state.ar_count++;
  n->state.ar_count %= 10000;

  if (!response && n->state.mode == ARTNET_ON) {
  }

  printf("[ArtPollReply] to=%s, ip=%s, shortName=%s, status=0x%02X, status2=0x%02X, "
         "net=%d, sub=%d, mac=%02X:%02X:%02X:%02X:%02X:%02X, ports=",
         inet_ntoa(n->state.reply_addr),
         inet_ntoa(n->state.ip_addr),
         n->state.shortName,
         (n->state.led_state << 6) | 0x20 | 0x02,
         n->state.status2,
         n->state.netSwitch,
         n->state.subSwitch,
         n->state.hw_addr[0], n->state.hw_addr[1], n->state.hw_addr[2],
         n->state.hw_addr[3], n->state.hw_addr[4], n->state.hw_addr[5]);
  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    if (n->ports.out[i].port_enabled) {
      uint16_t addr = n->ports.out[i].port_addr;
      printf("%d:0x%04X ", i, addr);
    }
  }
  printf("\n");

  reply.to = n->state.reply_addr;
  reply.type = ARTNET_REPLY;
  reply.length = sizeof(artnet_reply_t);

  // copy from a poll reply template
  memcpy(&reply.data, &n->ar_temp, sizeof(artnet_reply_t));

  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    reply.data.ar.goodInput[i] = n->ports.in[i].port_status;
    reply.data.ar.goodOutputA[i] = n->ports.out[i].port_status | n->ports.out[i].proto_sel;
  }

  snprintf((char *) &reply.data.ar.nodeReport,
           sizeof(reply.data.ar.nodeReport),
           "#%04x [%04i] libartnet",
           n->state.report_code,
           n->state.ar_count);

  return artnet_net_send(n, &reply);
}


/*
 * Send a tod request.
 * Ports may span multiple nets (e.g. when using artnet_join), so we group
 * enabled ports by net and send a separate ArtTodRequest per net group.
 */
int artnet_tx_tod_request(node n) {
  int i, ret = ARTNET_EOK;

  // collect nets from enabled output ports
  uint8_t nets[ARTNET_MAX_PORTS];
  int net_count = 0;

  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    if (!n->ports.out[i].port_enabled) {
      continue;
    }
    uint8_t net = addr_net(n->ports.out[i].port_addr);
    // check if we already have this net
    int found = 0;
    int j = 0;
    for (j = 0; j < net_count; j++) {
      if (nets[j] == net) {
        found = 1;
        break;
      }
    }
    if (!found) {
      nets[net_count++] = net;
    }
  }

  // send one ArtTodRequest per unique net
  for (i = 0; i < net_count; i++) {
    artnet_packet_t todreq = {0};
    int p = 0;

    todreq.to = n->state.bcast_addr;
    todreq.type = ARTNET_TODREQUEST;
    todreq.length = sizeof(artnet_todrequest_t);
    memset(&todreq.data, 0x00, todreq.length);

    memcpy(&todreq.data.todreq.id, ARTNET_STRING, ARTNET_STRING_SIZE);
    todreq.data.todreq.opCode = htols(ARTNET_TODREQUEST);
    todreq.data.todreq.verH = 0;
    todreq.data.todreq.ver = ARTNET_VERSION;
    todreq.data.todreq.command = ARTNET_TOD_FULL;
    todreq.data.todreq.net = nets[i];
    todreq.data.todreq.adCount = 0;

    // include all enabled ports belonging to this net
    for (p = 0; p < ARTNET_MAX_PORTS; p++) {
      if (n->ports.out[p].port_enabled &&
          addr_net(n->ports.out[p].port_addr) == nets[i]) {
        todreq.data.todreq.address[todreq.data.todreq.adCount++] =
          (uint8_t)((addr_subnet(n->ports.out[p].port_addr) << 4) | addr_port(n->ports.out[p].port_addr));
      }
    }

    ret = ret || artnet_net_send(n, &todreq);
  }

  return ret;
}


/*
 * Send a tod data for port number id
 * @param id the number of the port to send data for
 */
int artnet_tx_tod_data(node n, int id) {
  artnet_packet_t tod = {0};
  int lim = 0, remaining = 0, bloc = 0, offset = 0;
  int ret = ARTNET_EOK;

  // ok we need to check how many uid's we have,
  // may need to send more than one datagram

  // Art-Net 4: TodData must be unicast to the requester
  tod.to = n->state.tod_reply_addr;
  tod.type = ARTNET_TODDATA;
  tod.length = sizeof(artnet_toddata_t);

  memset(&tod.data,0x00, tod.length);

  // set up the data
  memcpy(&tod.data.toddata.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  tod.data.toddata.opCode = htols(ARTNET_TODDATA);
  tod.data.toddata.verH = 0;
  tod.data.toddata.ver = ARTNET_VERSION;
  tod.data.toddata.rdmVer = ARTNET_RDM_VERSION;
  tod.data.toddata.port = (uint8_t)(id + 1);
  tod.data.toddata.bindIndex = 1;

  // this is interesting, the spec mentions TOD_ADD and TOD_SUBTRACT, but the
  // codes aren't given. The windows drivers don't have these either....
  tod.data.toddata.cmdRes = ARTNET_TOD_FULL;

  tod.data.toddata.address = (uint8_t)((addr_subnet(n->ports.out[id].port_addr) << 4) | addr_port(n->ports.out[id].port_addr));
  tod.data.toddata.net = addr_net(n->ports.out[id].port_addr);
  tod.data.toddata.uidTotalHi = short_get_high_byte(n->ports.out[id].port_tod.length);
  tod.data.toddata.uidTotal = short_get_low_byte(n->ports.out[id].port_tod.length);

  remaining = n->ports.out[id].port_tod.length;
  bloc = 0;

  while (remaining > 0) {
    memset(&tod.data.toddata.tod,0x00, ARTNET_MAX_UID_COUNT * ARTNET_RDM_UID_WIDTH);
    lim = min(ARTNET_MAX_UID_COUNT, remaining);
    tod.data.toddata.blockCount = (uint8_t)bloc++;
    tod.data.toddata.uidCount = (uint8_t)lim;

    offset = (n->ports.out[id].port_tod.length - remaining) * ARTNET_RDM_UID_WIDTH;
    if (n->ports.out[id].port_tod.data != NULL) {
      memcpy(tod.data.toddata.tod,
             n->ports.out[id].port_tod.data + offset,
             lim * ARTNET_RDM_UID_WIDTH);
    }

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
  artnet_packet_t tod = {0};

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
  tod.data.todcontrol.address = (uint8_t)((addr_subnet(address) << 4) | addr_port(address));
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
  artnet_packet_t rdm = {0};
  int len = 0;

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
  rdm.data.rdm.address = (uint8_t)((addr_subnet(address) << 4) | addr_port(address));
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
  artnet_packet_t rdmsub = {0};
  int len = 0;

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
  artnet_packet_t diag = {0};
  int text_len = 0;

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  // Art-Net 4: only send if diagnostics are enabled
  if (!n->state.diag_enabled) {
    return ARTNET_EACTION;
  }

  // priority filtering: only send if >= minimum requested priority
  if (priority < n->state.diag_priority) {
    return ARTNET_EOK;
  }

  text_len = (int)strlen(text);
  if (text_len > ARTNET_DMX_LENGTH - 1) {
    text_len = ARTNET_DMX_LENGTH - 1;
  }

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
  artnet_packet_t p = {0};
  memset(&p, 0x0, sizeof(p));

  p.to.s_addr = ip;
  p.length = sizeof(artnet_firmware_reply_t);
  p.type = ARTNET_FIRMWAREREPLY;

  // now build packet
  memcpy(&p.data.firmwarer.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.firmwarer.opCode = htols(ARTNET_FIRMWAREREPLY);
  p.data.firmwarer.verH = 0;
  p.data.firmwarer.ver = ARTNET_VERSION;
  p.data.firmwarer.type = code;

  return artnet_net_send(n, &p);
}


/*
 * Send an firmware data datagram
 *
 * @param firm a pointer to the firmware structure for this transfer
 */
int artnet_tx_firmware_packet(node n, firmware_transfer_t *firm) {
  artnet_packet_t p = {0};
  uint8_t type = 0;
  int data_len = 0, max_len = 0, ret = 0;

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
  p.data.firmware.blockId = (uint8_t)firm->expected_block;

  artnet_misc_int_to_bytes(firm->bytes_total / sizeof(uint16_t),
                           p.data.firmware.length);

  memcpy(&p.data.firmware.data,
         firm->data + (firm->bytes_current / sizeof(uint16_t)),
         data_len);

  if ((ret = artnet_net_send(n, &p)) != 0) {
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
  artnet_packet_t p = {0};

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

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
 * Send an ArtNzs packet (non-zero start code DMX)
 */
int artnet_tx_nzs(node n, int port_id, uint8_t start_code,
                  int16_t length, const uint8_t *data) {
  artnet_packet_t p = {0};
  input_port_t *port = NULL;

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
    return ARTNET_EARG;
  }
  port = &n->ports.in[port_id];

  if (length < 1 || length > ARTNET_DMX_LENGTH) {
    return ARTNET_EARG;
  }

  if (port->port_status & PORT_STATUS_DISABLED_MASK) {
    return ARTNET_EARG;
  }

  port->port_status = port->port_status | PORT_STATUS_ACT_MASK;

  p.length = sizeof(artnet_nzs_t) - (ARTNET_DMX_LENGTH - length);

  memcpy(&p.data.nzs.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.nzs.opCode = htols(ARTNET_NZS);
  p.data.nzs.verH = 0;
  p.data.nzs.ver = ARTNET_VERSION;
  p.data.nzs.sequence = port->seq;
  p.data.nzs.startCode = start_code;
  p.data.nzs.universe = htols(port->port_addr);
  p.data.nzs.lengthHi = short_get_high_byte(length);
  p.data.nzs.length = short_get_low_byte(length);
  memcpy(&p.data.nzs.data, data, length);

  // Art-Net 4: unicast to subscribers
  p.to.s_addr = n->state.bcast_addr.s_addr;

  int nodes = 0, i = 0;
  int limit = n->state.bcast_limit > 0 ? n->state.bcast_limit : (n->node_list.length ? n->node_list.length : 1);
  SI *ips = malloc(sizeof(SI) * limit);

  if (!ips) {
    return ARTNET_EACTION;
  }

  nodes = find_nodes_from_uni(n, &n->node_list,
                              port->port_addr,
                              ips,
                              limit);

  for (i = 0; i < nodes; i++) {
    p.to = ips[i];
    artnet_net_send(n, &p);
  }
  free(ips);

  port->seq++;
  return ARTNET_EOK;
}


/*
 * Send an ArtDataReply packet (Art-Net 4)
 */
int artnet_tx_data_reply(node n, const char *ip, uint8_t request_code,
                         const char *payload, int16_t length) {
  artnet_packet_t p = {0};

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (length > 512) {
    length = 512;
  }

  memset(&p, 0x00, sizeof(p));
  if (ip && artnet_net_inet_aton(ip, &p.to) == 0) {
    return ARTNET_EARG;
  }

  p.type = ARTNET_DATAREPLY;
  p.length = sizeof(artnet_data_reply_t);

  memcpy(&p.data.datarep.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.datarep.opCode = htols(ARTNET_DATAREPLY);
  p.data.datarep.verH = 0;
  p.data.datarep.ver = ARTNET_VERSION;
  p.data.datarep.estaManHi = n->state.esta_hi;
  p.data.datarep.estaManLo = (uint8_t)n->state.esta_lo;
  p.data.datarep.oemHi = n->state.oem_hi;
  p.data.datarep.oemLo = n->state.oem_lo;
  p.data.datarep.requestHi = short_get_high_byte(request_code);
  p.data.datarep.requestLo = short_get_low_byte(request_code);
  p.data.datarep.payLenHi = short_get_high_byte(length);
  p.data.datarep.payLenLo = short_get_low_byte(length);
  if (payload && length > 0) {
    memcpy(p.data.datarep.payLoad, payload, length);
  }

  return artnet_net_send(n, &p);
}


/*
 * Send an ArtTimeCode packet
 */
int artnet_tx_timecode(node n, uint8_t frames, uint8_t seconds,
                       uint8_t minutes, uint8_t hours,
                       artnet_timecode_type_t type, uint8_t stream_id) {
  artnet_packet_t p = {0};

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  memset(&p, 0x00, sizeof(p));
  p.to.s_addr = n->state.bcast_addr.s_addr;
  p.type = ARTNET_TIMECODE;
  p.length = sizeof(artnet_timecode_t);

  memcpy(&p.data.tc.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.tc.opCode = htols(ARTNET_TIMECODE);
  p.data.tc.verH = 0;
  p.data.tc.ver = ARTNET_VERSION;
  p.data.tc.streamId = stream_id;
  p.data.tc.frames = frames;
  p.data.tc.seconds = seconds;
  p.data.tc.minutes = minutes;
  p.data.tc.hours = hours;
  p.data.tc.type = (uint8_t)type;

  return artnet_net_send(n, &p);
}


/*
 * Send an ArtTimeSync packet
 */
int artnet_tx_timesync(node n, uint8_t tm_sec, uint8_t tm_min,
                       uint8_t tm_hour, uint8_t tm_mday,
                       uint8_t tm_mon, uint8_t tm_year) {
  artnet_packet_t p = {0};

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  memset(&p, 0x00, sizeof(p));
  p.to.s_addr = n->state.bcast_addr.s_addr;
  p.type = ARTNET_TIMESYNC;
  p.length = sizeof(artnet_timesync_t);

  memcpy(&p.data.tsync.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.tsync.opCode = htols(ARTNET_TIMESYNC);
  p.data.tsync.verH = 0;
  p.data.tsync.ver = ARTNET_VERSION;
  p.data.tsync.tm_sec = tm_sec;
  p.data.tsync.tm_min = tm_min;
  p.data.tsync.tm_hour = tm_hour;
  p.data.tsync.tm_mday = tm_mday;
  p.data.tsync.tm_mon = tm_mon;
  p.data.tsync.tm_year = tm_year;

  return artnet_net_send(n, &p);
}


/*
 * Send an ArtTrigger packet
 */
int artnet_tx_trigger(node n, uint8_t oem_hi, uint8_t oem_lo,
                      uint8_t key, uint8_t sub_key,
                      const uint8_t *data, int16_t length) {
  artnet_packet_t p = {0};

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (length < 0 || length > ARTNET_DMX_LENGTH) {
    return ARTNET_EARG;
  }

  memset(&p, 0x00, sizeof(p));
  p.to.s_addr = n->state.bcast_addr.s_addr;
  p.type = ARTNET_TRIGGER;
  p.length = sizeof(artnet_trigger_t);

  memcpy(&p.data.trigger.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.trigger.opCode = htols(ARTNET_TRIGGER);
  p.data.trigger.verH = 0;
  p.data.trigger.ver = ARTNET_VERSION;
  p.data.trigger.oemCodeHi = oem_hi;
  p.data.trigger.oemCodeLo = oem_lo;
  p.data.trigger.key = key;
  p.data.trigger.subKey = sub_key;

  if (length > 0 && data) {
    memcpy(&p.data.trigger.data, data, length);
  }

  return artnet_net_send(n, &p);
}


/*
 * Send an ArtDirectoryReply packet (empty directory)
 */
int artnet_tx_directory_reply(node n) {
  artnet_packet_t p = {0};

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
 * Send an ArtIpProgReply packet
 */
int artnet_tx_ipprog_reply(node n) {
  artnet_packet_t p = {0};

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  memset(&p, 0x00, sizeof(p));
  p.to = n->state.reply_addr;
  p.type = ARTNET_IPREPLY;
  p.length = sizeof(artnet_ipprog_reply_t);

  memcpy(&p.data.aipr.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.aipr.OpCode = htols(ARTNET_IPREPLY);
  p.data.aipr.ProVerHi = 0;
  p.data.aipr.ProVerLo = ARTNET_VERSION;

  // Status: bit 6 = DHCP enabled
  p.data.aipr.Status = 0x00;

  // Report current IP settings
  uint32_t ip = ntohl(n->state.ip_addr.s_addr);
  p.data.aipr.ProgIpHi = (uint8_t)((ip >> 24) & 0xFF);
  p.data.aipr.ProgIp2  = (uint8_t)((ip >> 16) & 0xFF);
  p.data.aipr.ProgIp1  = (uint8_t)((ip >> 8) & 0xFF);
  p.data.aipr.ProgIpLo = (uint8_t)(ip & 0xFF);

  uint32_t subnet = ntohl(n->state.subnet_mask.s_addr);
  p.data.aipr.ProgSmHi = (uint8_t)((subnet >> 24) & 0xFF);
  p.data.aipr.ProgSm2  = (uint8_t)((subnet >> 16) & 0xFF);
  p.data.aipr.ProgSm1  = (uint8_t)((subnet >> 8) & 0xFF);
  p.data.aipr.ProgSmLo = (uint8_t)(subnet & 0xFF);

  uint32_t gw = ntohl(n->state.gateway.s_addr);
  p.data.aipr.ProgDgHi = (uint8_t)((gw >> 24) & 0xFF);
  p.data.aipr.ProgDg2  = (uint8_t)((gw >> 16) & 0xFF);
  p.data.aipr.ProgDg1  = (uint8_t)((gw >> 8) & 0xFF);
  p.data.aipr.ProgDgLo = (uint8_t)(gw & 0xFF);

  return artnet_net_send(n, &p);
}


/*
 * Send an ArtFileFnReply packet
 */
int artnet_tx_file_fn_reply(node n, uint8_t blockId, uint16_t totalLength,
                            uint8_t *data, int dataLen) {
  artnet_packet_t p = {0};
  int len = 0;

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
  int i = 0;

  // shorten the amount we have to type
  artnet_reply_t *ar = &n->ar_temp;

  memset(ar, 0x00, sizeof(artnet_reply_t));

  memcpy(&ar->id, ARTNET_STRING, ARTNET_STRING_SIZE);
  ar->opCode = htols(ARTNET_REPLY);
  memcpy(&ar->ip, &n->state.ip_addr.s_addr, 4);
  ar->port = (uint16_t)htols(ARTNET_PORT);
  ar->verH = 0;
  ar->ver = ARTNET_VERSION;
  ar->netSwitch = n->state.netSwitch;
  ar->subSwitch = n->state.subSwitch;
  ar->oemH = n->state.oem_hi;
  ar->oem = n->state.oem_lo;
  ar->ubea = 0;
  // ar->status - recalc every time

  // ESTA Manufacturer ID
  ar->estaMan[0] = n->state.esta_hi;
  ar->estaMan[1] = n->state.esta_lo;

  memcpy(&ar->shortName, &n->state.shortName, sizeof(n->state.shortName));
  memcpy(&ar->longName, &n->state.longName, sizeof(n->state.longName));

  // port stuff here
  ar->numbportsH = 0;

  for (i = ARTNET_MAX_PORTS; i > 0; i--) {
    if (n->ports.out[i-1].port_enabled || n->ports.in[i-1].port_enabled) {
      break;
    }
  }

  ar->numbports = (uint8_t)i;

  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    ar->portTypes[i] = n->ports.types[i];
    ar->goodInput[i] = n->ports.in[i].port_status;
    ar->goodOutputA[i] = n->ports.out[i].port_status | n->ports.out[i].proto_sel;
    ar->swIn[i] = addr_port(n->ports.in[i].port_addr);
    ar->swOut[i] = addr_port(n->ports.out[i].port_addr);
  }

  ar->acnPriority = n->state.acn_priority;
  ar->swMacro = 0;
  ar->swRemote = 0;

  // spares
  ar->sp1 = 0;
  ar->sp2 = 0;
  ar->sp3 = 0;

  // style: product type (StNode, StController, etc.)
  ar->style = n->state.style_code;

  // hw address
  memcpy(&ar->mac, &n->state.hw_addr, ARTNET_MAC_SIZE);

  // bind index: 1 = root device
  ar->bindIndex = 1;

  // bind IP: root device IP address
  memcpy(&ar->bindIp, &n->state.ip_addr.s_addr, 4);

  // Status1: LED state in bits 7-6, bits 5-4 = port address programming (0b10 = network), bit 2 = ROM boot, bit 1 = RDM support, bit 0 = UBEA
  ar->status = (n->state.led_state << 6) | 0x20 | 0x02;

  // Status3: fail-safe mode in bits 7-6, bit 3 = port direction switching supported
  ar->status3 = n->state.failsafe_mode | 0x08;

  // Status2: default includes 15-bit port addressing support
  ar->status2 = n->state.status2;

  // GoodOutputB: RDM disabled (bit 7), output style (bit 6)
  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    ar->goodOutputB[i] = 0;
    if (!n->ports.out[i].rdm_enabled) {
      ar->goodOutputB[i] |= ARTNET_GOODB_RDM_DISABLED;
    }
    if (n->ports.out[i].output_style) {
      ar->goodOutputB[i] |= ARTNET_GOODB_STYLE_CONSTANT;
    }
  }

  // DefaultRespUID: RDMnet & LLRP default responder UID
  memcpy(&ar->defaultRespUid, n->state.default_resp_uid, ARTNET_RDM_UID_WIDTH);

  // Art-Net 4: user data and refresh rate
  ar->userHi = 0;
  ar->userLo = 0;
  ar->refreshRateHi = 0;
  ar->refreshRateLo = 0;  // 0 = max DMX512 rate (44Hz)
  ar->bgQueuePolicy = 0;  // collect using STATUS_NONE

  return ARTNET_EOK;
}
