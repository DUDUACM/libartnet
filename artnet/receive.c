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
 * receive.c
 * Handles the receiving of datagrams
 * Copyright (C) 2004-2007 Simon Newton
 */

#include "private.h"

void check_merge_timeouts(node n, int port);
void merge(node n, int port, int length, uint8_t *latest);

/*
 * Checks if the callback is defined, if so call it passing the packet and
 * the user supplied data.
 * If the callbacks return a non-zero result, further processing is canceled.
 */
int check_callback(node n, artnet_packet p, callback_t callback) {
  if (callback.fh != NULL) {
    return callback.fh(n, p, callback.data);
  }

  return 0;
}


/*
 * Handle an artpoll packet
 */
int handle_poll(node n, artnet_packet p) {
  // run callback if defined
  if (check_callback(n, p, n->callbacks.poll)) {
    return ARTNET_EOK;
  }

  if (n->state.node_type != ARTNET_RAW) {
    // Art-Net 4: ArtPollReply must always be unicast to the poller
    // (bit 0 UNICAST_DEPRECATED is ignored, broadcast is no longer permitted)
    n->state.reply_addr = p->from;

    // if we are told to send updates when node conditions change
    if (p->data.ap.flags & ARTNET_POLL_FLAG_REPLY_ON_CHANGE) {
      n->state.send_apr_on_change = TRUE;
    } else {
      n->state.send_apr_on_change = FALSE;
    }

    // Art-Net 4: Target Mode filtering
    if (p->data.ap.flags & ARTNET_POLL_FLAG_TARGET_MODE) {
      uint16_t top = (p->data.ap.targetPortAddressTopHi << 8) |
                      p->data.ap.targetPortAddressTopLo;
      uint16_t bottom = (p->data.ap.targetPortAddressBottomHi << 8) |
                        p->data.ap.targetPortAddressBottomLo;
      int i, match = 0;

      if (bottom == 0 && top == 0) {
        // 0x0000 means "match all"
        match = 1;
      } else {
        for (i = 0; i < ARTNET_MAX_PORTS; i++) {
          uint16_t addr = n->ports.out[i].port_addr;
          if (!n->ports.out[i].port_enabled) {
            addr = n->ports.in[i].port_addr;
          }
          if (addr >= bottom && addr <= top) {
            match = 1;
            break;
          }
        }
      }

      if (!match) {
        return ARTNET_EOK;
      }
    }

    // Art-Net 4: diagnostic configuration from ArtPoll Flags
    if (p->data.ap.flags & ARTNET_POLL_FLAG_DIAG_ENABLE) {
      n->state.diag_enabled = TRUE;
      n->state.diag_unicast = (p->data.ap.flags & ARTNET_POLL_FLAG_DIAG_UNICAST) != 0;
      n->state.diag_priority = p->data.ap.diagPriority;
    } else {
      n->state.diag_enabled = FALSE;
    }

    return artnet_tx_poll_reply(n, TRUE);

  }
  return ARTNET_EOK;
}

/*
 * handle an art poll reply
 */
void handle_reply(node n, artnet_packet p) {
  // update the node list
  artnet_nl_update(n, &n->node_list, p);

  // run callback if defined
  if (check_callback(n, p, n->callbacks.reply)) {
    return;
  }
}


/*
 * handle a art dmx packet
 */
void handle_dmx(node n, artnet_packet p) {
  int i = 0, data_length = 0;
  output_port_t *port = NULL;
  in_addr_t ipA = 0, ipB = 0;

  // run callback if defined
  if (check_callback(n, p, n->callbacks.dmx)) {
    return;
  }

  // Art-Net 4: record source IP for ArtSync validation
  n->state.last_dmx_source = p->from;

  data_length = (int) bytes_to_short(p->data.admx.lengthHi,
                                     p->data.admx.length);
  data_length = min(data_length, ARTNET_DMX_LENGTH);

  // find matching output ports
  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    // if the addr matches and this port is enabled
    if (p->data.admx.universe == htols(n->ports.out[i].port_addr) &&
        n->ports.out[i].port_enabled) {

      port = &n->ports.out[i];
      ipA = port->ipA.s_addr;
      ipB = port->ipB.s_addr;

      // ok packet matches this port
      n->ports.out[i].port_status = n->ports.out[i].port_status | PORT_STATUS_ACT_MASK;

      /**
       * 9 cases for merging depending on what the stored ips are.
       * here's the truth table
       *
       *
       * \   ipA   #           #            #             #
       *  ------   #   empty   #            #             #
       *   ipB  \  #   ( 0 )   #    p.from  #   ! p.from  #
       * ##################################################
       *           # new node  # continued  # start       #
       *  empty    # first     #  trans-    #  merge      #
       *   (0)     #   packet  #   mission  #             #
       * ##################################################
       *           #continued  #            # cont        #
       *  p.from   # trans-    # invalid!   #  merge      #
       *           #  mission  #            #             #
       * ##################################################
       *           # start     # cont       #             #
       * ! p.from  #  merge    #   merge    # discard     #
       *           #           #            #             #
       * ##################################################
       *
       * The merge exits when:
       *   o ACCancel command is received in an ArtAddress packet
       *       (this is done in handle_address )
       *   o no data is recv'ed from one source in 10 seconds
       *
       */

      check_merge_timeouts(n,i);

      if (ipA == 0 && ipB == 0) {
        // first packet recv on this port
        port->ipA.s_addr = p->from.s_addr;
        port->timeA = clock();

        memcpy(&port->dataA, &p->data.admx.data, data_length);
        port->length = data_length;
        memcpy(&port->data, &p->data.admx.data, data_length);
      }
      else if (ipA == p->from.s_addr && ipB == 0) {
        //continued transmission from the same ip (source A)

        port->timeA = clock();
        memcpy(&port->dataA, &p->data.admx.data, data_length);
        port->length = data_length;
        memcpy(&port->data, &p->data.admx.data, data_length);
      }
      else if (ipA == 0 && ipB == p->from.s_addr) {
        //continued transmission from the same ip (source B)

        port->timeB = clock();
        memcpy(&port->dataB, &p->data.admx.data, data_length);
        port->length = data_length;
        memcpy(&port->data, &p->data.admx.data, data_length);
      }
      else if (ipA != p->from.s_addr  && ipB == 0) {
        // new source, start the merge (A exists, new source becomes B)
        port->ipB.s_addr = p->from.s_addr;
        port->timeB = clock();
        memcpy(&port->dataB, &p->data.admx.data,data_length);
        port->length = data_length;

        // merge, newest data is port B
        merge(n,i,data_length, port->dataB);

        // notify controller: merge started
        port->port_status |= PORT_STATUS_MERGE;
        if (n->state.send_apr_on_change) {
          artnet_tx_poll_reply(n, TRUE);
        }

      }
      else if (ipA == 0 && ipB != 0 && ipB != p->from.s_addr) {
        // new source, start the merge (B exists, new source becomes A)
        port->ipA.s_addr = p->from.s_addr;
        port->timeA = clock();
        memcpy(&port->dataA, &p->data.admx.data, data_length);
        port->length = data_length;

        // merge, newest data is port A
        merge(n, i, data_length, port->dataA);

        // notify controller: merge started
        port->port_status |= PORT_STATUS_MERGE;
        if (n->state.send_apr_on_change) {
          artnet_tx_poll_reply(n, TRUE);
        }

      }
      else if (ipA == p->from.s_addr && ipB != p->from.s_addr) {
        // continue merge
        port->timeA = clock();
        memcpy(&port->dataA, &p->data.admx.data,data_length);
        port->length = data_length;

        // merge, newest data is portA
        merge(n,i,data_length, port->dataA);

      }
      else if (ipA != p->from.s_addr && ipB == p->from.s_addr) {
        // continue merge
        port->timeB = clock();
        memcpy(&port->dataB, &p->data.admx.data,data_length);
        port->length = data_length;

        // merge newest data is portB
        merge(n,i,data_length, port->dataB);

      }
      else if (ipA == p->from.s_addr && ipB == p->from.s_addr) {
//        err_warn("In handle_dmx, source matches both buffers, this shouldn't be happening!\n");

      }
      else if (ipA != p->from.s_addr && ipB != p->from.s_addr) {
//        err_warn("In handle_dmx, more than two sources, discarding data\n");

      }
      else {
//        err_warn("In handle_dmx, no cases matched, this shouldn't happen!\n");

      }

      // do the dmx callback here
      if (n->callbacks.dmx_c.fh != NULL) {
        n->callbacks.dmx_c.fh(n,i, n->callbacks.dmx_c.data);
      }

      // Art-Net 4: record last DMX receive time for fail-safe
      port->last_dmx_time = clock();
      port->failsafe_triggered = FALSE;
    }
  }
  return;
}


/**
 * handle art address packet.
 * This can reprogram certain nodes settings such as short/long name, port
 * addresses, subnet address etc.
 *
 */
int handle_address(node n, artnet_packet p) {
  int i = 0, old_subnet = 0;
  int addr[ARTNET_MAX_PORTS] = {0};
  int ret = 0;

  if (check_callback(n, p, n->callbacks.address)) {
    return ARTNET_EOK;
  }

  // servers (and raw nodes) don't respond to address packets
  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    return ARTNET_EOK;
  }

  // reprogram shortName if required
  if (p->data.addr.shortName[0] != PROGRAM_DEFAULTS &&
      p->data.addr.shortName[0] != PROGRAM_NO_CHANGE) {
    memcpy(&n->state.shortName, &p->data.addr.shortName, ARTNET_SHORT_NAME_LENGTH);
    n->state.report_code = ARTNET_RC_SHNAME_OK;
  }
  // reprogram long name if required
  if (p->data.addr.longName[0] != PROGRAM_DEFAULTS &&
      p->data.addr.longName[0] != PROGRAM_NO_CHANGE) {
    memcpy(&n->state.longName, &p->data.addr.longName, ARTNET_LONG_NAME_LENGTH);
    n->state.report_code = ARTNET_RC_LONAME_OK;
  }

  // first of all store existing port addresses
  // then we can work out if they change
  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    addr[i] = n->ports.in[i].port_addr;
  }

  // program subnet
  old_subnet = p->data.addr.subSwitch & 0x0F;
  if (p->data.addr.subSwitch == PROGRAM_DEFAULTS) {
    // reset to defaults
    n->state.subSwitch = n->state.default_subSwitch;
    n->state.subSwitch_net_ctl = FALSE;

  } else if (p->data.addr.subSwitch & PROGRAM_CHANGE_MASK) {
    n->state.subSwitch = p->data.addr.subSwitch & 0x0F;
    n->state.subSwitch_net_ctl = TRUE;
  }

  // program net (Art-Net 4)
  if (p->data.addr.netSwitch == PROGRAM_DEFAULTS) {
    n->state.netSwitch = n->state.default_netSwitch;
    n->state.netSwitch_net_ctl = FALSE;
  } else if (p->data.addr.netSwitch & PROGRAM_CHANGE_MASK) {
    n->state.netSwitch = p->data.addr.netSwitch & ~PROGRAM_CHANGE_MASK;
    n->state.netSwitch_net_ctl = TRUE;
  }

  // check if subnet has actually changed
  if (old_subnet != n->state.subSwitch) {
    // if it does we need to change all port addresses
    for(i=0; i< ARTNET_MAX_PORTS; i++) {
      n->ports.in[i].port_addr = make_addr(n->state.netSwitch, n->state.subSwitch, addr_port(n->ports.in[i].port_addr));
      n->ports.out[i].port_addr = make_addr(n->state.netSwitch, n->state.subSwitch, addr_port(n->ports.out[i].port_addr));
    }
  }

  // program swIns
  for (i =0; i < ARTNET_MAX_PORTS; i++) {
    if (p->data.addr.swIn[i] == PROGRAM_NO_CHANGE)  {
      continue;
    } else if (p->data.addr.swIn[i] == PROGRAM_DEFAULTS) {
      // reset to defaults
      n->ports.in[i].port_addr = make_addr(n->state.netSwitch, n->state.subSwitch, n->ports.in[i].port_default_addr);
      n->ports.in[i].port_net_ctl = FALSE;

    } else if ( p->data.addr.swIn[i] & PROGRAM_CHANGE_MASK) {
      n->ports.in[i].port_addr = make_addr(n->state.netSwitch, n->state.subSwitch, p->data.addr.swIn[i]);
      n->ports.in[i].port_net_ctl = TRUE;
    }
  }

  // program swOuts
  for (i =0; i < ARTNET_MAX_PORTS; i++) {
    if (p->data.addr.swOut[i] == PROGRAM_NO_CHANGE) {
      continue;
    } else if (p->data.addr.swOut[i] == PROGRAM_DEFAULTS) {
      // reset to defaults
      n->ports.out[i].port_addr = make_addr(n->state.netSwitch, n->state.subSwitch, n->ports.out[i].port_default_addr);
      n->ports.out[i].port_net_ctl = FALSE;
      n->ports.out[i].port_enabled = TRUE;
    } else if ( p->data.addr.swOut[i] & PROGRAM_CHANGE_MASK) {
      n->ports.out[i].port_addr = make_addr(n->state.netSwitch, n->state.subSwitch, p->data.addr.swOut[i]);
      n->ports.out[i].port_net_ctl = TRUE;
      n->ports.out[i].port_enabled = TRUE;
    }
  }

  // reset sequence numbers if the addresses change
  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    if (addr[i] != n->ports.in[i].port_addr) {
      n->ports.in[i].seq = 0;
    }
  }

  // check command
  int port_idx = 0;
  uint8_t cmd = p->data.addr.command;

  switch (cmd) {
    case ARTNET_PC_NONE:
      break;

    case ARTNET_PC_CANCEL:
      for (i = 0; i < ARTNET_MAX_PORTS; i++) {
        n->ports.out[i].ipA.s_addr = 0;
        n->ports.out[i].ipB.s_addr = 0;
        n->ports.out[i].timeA = 0;
        n->ports.out[i].timeB = 0;
        n->ports.out[i].port_status &= ~PORT_STATUS_MERGE;
      }
      break;

    case ARTNET_PC_LED_NORMAL:
      n->state.led_state = ARTNET_LED_NORMAL;
      break;
    case ARTNET_PC_LED_MUTE:
      n->state.led_state = ARTNET_LED_MUTE;
      break;
    case ARTNET_PC_LED_LOCATE:
      n->state.led_state = ARTNET_LED_LOCATE;
      break;

    case ARTNET_PC_RESET:
      for (i = 0; i < ARTNET_MAX_PORTS; i++) {
        n->ports.out[i].port_status = n->ports.out[i].port_status &
          ~PORT_STATUS_DMX_SIP & ~PORT_STATUS_DMX_TEST & ~PORT_STATUS_DMX_TEXT;
      }
      break;

    case ARTNET_PC_ANALYSIS_ON:
    case ARTNET_PC_ANALYSIS_OFF:
      break;

    case ARTNET_PC_FAIL_HOLD:
      n->state.failsafe_mode = ARTNET_FAILSAFE_HOLD;
      break;
    case ARTNET_PC_FAIL_ZERO:
      n->state.failsafe_mode = ARTNET_FAILSAFE_ZERO;
      break;
    case ARTNET_PC_FAIL_FULL:
      n->state.failsafe_mode = ARTNET_FAILSAFE_FULL;
      break;
    case ARTNET_PC_FAIL_SCENE:
      n->state.failsafe_mode = ARTNET_FAILSAFE_SCENE;
      break;
    case ARTNET_PC_FAIL_RECORD:
      // Record current output data as fail-safe scene
      for (i = 0; i < ARTNET_MAX_PORTS; i++) {
        memcpy(n->ports.out[i].failsafe_data, n->ports.out[i].data, ARTNET_DMX_LENGTH);
        n->ports.out[i].failsafe_length = n->ports.out[i].length;
      }
      break;

    case ARTNET_PC_MERGE_LTP_O:
    case ARTNET_PC_MERGE_LTP_1:
    case ARTNET_PC_MERGE_LTP_2:
    case ARTNET_PC_MERGE_LTP_3:
      port_idx = cmd & 0x03;
      n->ports.out[port_idx].merge_mode = ARTNET_MERGE_LTP;
      n->ports.out[port_idx].port_status |= PORT_STATUS_LPT_MODE;
      break;

    case ARTNET_PC_DIRECTION_TX_0:
    case ARTNET_PC_DIRECTION_TX_1:
    case ARTNET_PC_DIRECTION_TX_2:
    case ARTNET_PC_DIRECTION_TX_3:
      port_idx = cmd & 0x03;
      n->ports.types[port_idx] |= ARTNET_ENABLE_OUTPUT;
      n->ports.types[port_idx] &= ~ARTNET_ENABLE_INPUT;
      n->ports.out[port_idx].ipA.s_addr = 0;
      n->ports.out[port_idx].ipB.s_addr = 0;
      n->ports.out[port_idx].timeA = 0;
      n->ports.out[port_idx].timeB = 0;
      n->ports.out[port_idx].port_status &= ~PORT_STATUS_MERGE;
      break;

    case ARTNET_PC_DIRECTION_RX_0:
    case ARTNET_PC_DIRECTION_RX_1:
    case ARTNET_PC_DIRECTION_RX_2:
    case ARTNET_PC_DIRECTION_RX_3:
      port_idx = cmd & 0x03;
      n->ports.types[port_idx] |= ARTNET_ENABLE_INPUT;
      n->ports.types[port_idx] &= ~ARTNET_ENABLE_OUTPUT;
      n->ports.out[port_idx].ipA.s_addr = 0;
      n->ports.out[port_idx].ipB.s_addr = 0;
      n->ports.out[port_idx].timeA = 0;
      n->ports.out[port_idx].timeB = 0;
      n->ports.out[port_idx].port_status &= ~PORT_STATUS_MERGE;
      break;

    case ARTNET_PC_MERGE_HTP_0:
    case ARTNET_PC_MERGE_HTP_1:
    case ARTNET_PC_MERGE_HTP_2:
    case ARTNET_PC_MERGE_HTP_3:
      port_idx = cmd & 0x03;
      n->ports.out[port_idx].merge_mode = ARTNET_MERGE_HTP;
      n->ports.out[port_idx].port_status &= ~PORT_STATUS_LPT_MODE;
      break;

    case ARTNET_PC_ARTNET_SEL_0:
    case ARTNET_PC_ARTNET_SEL_1:
    case ARTNET_PC_ARTNET_SEL_2:
    case ARTNET_PC_ARTNET_SEL_3:
      port_idx = cmd & 0x03;
      n->ports.out[port_idx].proto_sel = 0;
      break;

    case ARTNET_PC_ACN_SEL_0:
    case ARTNET_PC_ACN_SEL_1:
    case ARTNET_PC_ACN_SEL_2:
    case ARTNET_PC_ACN_SEL_3:
      port_idx = cmd & 0x03;
      n->ports.out[port_idx].proto_sel = 1;
      break;

    case ARTNET_PC_CLR_0:
    case ARTNET_PC_CLR_1:
    case ARTNET_PC_CLR_2:
    case ARTNET_PC_CLR_3:
      port_idx = cmd & 0x03;
      memset(n->ports.out[port_idx].data, 0x00, ARTNET_DMX_LENGTH);
      n->ports.out[port_idx].length = 0;
      break;

    case ARTNET_PC_STYLE_DELTA_0:
    case ARTNET_PC_STYLE_DELTA_1:
    case ARTNET_PC_STYLE_DELTA_2:
    case ARTNET_PC_STYLE_DELTA_3:
      port_idx = cmd & 0x03;
      n->ports.out[port_idx].output_style = 0;
      break;

    case ARTNET_PC_STYLE_CONST_0:
    case ARTNET_PC_STYLE_CONST_1:
    case ARTNET_PC_STYLE_CONST_2:
    case ARTNET_PC_STYLE_CONST_3:
      port_idx = cmd & 0x03;
      n->ports.out[port_idx].output_style = 1;
      break;

    case ARTNET_PC_RDM_ENABLED_0:
    case ARTNET_PC_RDM_ENABLED_1:
    case ARTNET_PC_RDM_ENABLED_2:
    case ARTNET_PC_RDM_ENABLED_3:
      port_idx = cmd & 0x03;
      n->ports.out[port_idx].rdm_enabled = 1;
      break;

    case ARTNET_PC_RDM_DISABLED_0:
    case ARTNET_PC_RDM_DISABLED_1:
    case ARTNET_PC_RDM_DISABLED_2:
    case ARTNET_PC_RDM_DISABLED_3:
      port_idx = cmd & 0x03;
      n->ports.out[port_idx].rdm_enabled = 0;
      break;

    default:
      break;
  }

  // Art-Net 4: handle sACN priority field (0xFF = no change)
  if (p->data.addr.acnPriority != 0xFF) {
    n->state.acn_priority = p->data.addr.acnPriority;
  }

  if (n->callbacks.program_c.fh != NULL) {
    n->callbacks.program_c.fh(n , n->callbacks.program_c.data);
  }

  if ((ret = artnet_tx_build_art_poll_reply(n)) != 0) {
    return ret;
  }

  return artnet_tx_poll_reply(n, TRUE);
}


/*
 * handle art input.
 * ArtInput packets can disable input ports.
 */
int _artnet_handle_input(node n, artnet_packet p) {
  int i = 0, ports = 0, ret = 0;

  if (check_callback(n, p, n->callbacks.input)) {
    return ARTNET_EOK;
  }

  // servers (and raw nodes) don't respond to input packets
  if (n->state.node_type != ARTNET_NODE && n->state.node_type != ARTNET_MSRV) {
    return ARTNET_EOK;
  }

  ports = min( p->data.ainput.numbports, ARTNET_MAX_PORTS);
  for (i =0; i < ports; i++) {
    if (p->data.ainput.input[i] & PORT_DISABLE_MASK) {
      // disable
      n->ports.in[i].port_status = n->ports.in[i].port_status | PORT_STATUS_DISABLED_MASK;
    } else {
      // enable
      n->ports.in[i].port_status = n->ports.in[i].port_status & ~PORT_STATUS_DISABLED_MASK;
    }
  }

  if ((ret = artnet_tx_build_art_poll_reply(n)) != 0) {
    return ret;
  }

  return artnet_tx_poll_reply(n, TRUE);
}
int handle_tod_request(node n, artnet_packet p) {
  int i = 0, j = 0, limit = 0;
  int ret = ARTNET_EOK;

  if (check_callback(n, p, n->callbacks.todrequest)) {
    return ARTNET_EOK;
  }

  if (n->state.node_type != ARTNET_NODE && n->state.node_type != ARTNET_MSRV) {
    return ARTNET_EOK;
  }

  // limit to 32
  limit = min(ARTNET_MAX_RDM_ADCOUNT, p->data.todreq.adCount);

  // this should always be true
  if (p->data.todreq.command == 0x00) {
    for (i=0; i < limit; i++) {
      for (j=0; j < ARTNET_MAX_PORTS; j++) {
        if (n->ports.out[j].port_addr == make_addr(p->data.todreq.net, (p->data.todreq.address[i] >> 4) & 0x0F, p->data.todreq.address[i] & 0x0F) &&
            n->ports.out[j].port_enabled) {
          // reply with tod
          int tx_ret = artnet_tx_tod_data(n, j);
          if (tx_ret) ret = tx_ret;
        }
      }
    }
  }

//  err_warn("tod request received but command is 0x%02hhx rather than 0x00\n", p->data.todreq.command);
  return ret;
}

/**
 * handle tod data packet
 *
 * we don't maintain a tod of whats out on the network,
 * the calling app can deal with this.
 */
void handle_tod_data(node n, artnet_packet p) {

  if (check_callback(n, p, n->callbacks.toddata)) {
    return;
  }

  if (n->callbacks.rdm_tod_c.fh != NULL) {
    int addr = make_addr(p->data.toddata.net, (p->data.toddata.address >> 4) & 0x0F, p->data.toddata.address & 0x0F);
    n->callbacks.rdm_tod_c.fh(n, addr, n->callbacks.rdm_tod_c.data);
  }
  return;
}



int handle_tod_control(node n, artnet_packet p) {
  int i = 0;
  int ret = ARTNET_EOK;

  if (check_callback(n, p, n->callbacks.todcontrol)) {
    return ARTNET_EOK;
  }

  for (i=0; i < ARTNET_MAX_PORTS; i++) {
    if (n->ports.out[i].port_addr == make_addr(p->data.todcontrol.net, (p->data.todcontrol.address >> 4) & 0x0F, p->data.todcontrol.address & 0x0F) &&
        n->ports.out[i].port_enabled) {

      switch (p->data.todcontrol.cmd) {
        case ARTNET_TOD_FULL:
          // AtcNone: no action, just reply with current TOD
          break;
        case ARTNET_TOD_FLUSH:
          flush_tod(&n->ports.out[i].port_tod);
          // initiate full RDM discovery
          if (n->callbacks.rdm_init_c.fh != NULL) {
            n->callbacks.rdm_init_c.fh(n, i, n->callbacks.rdm_init_c.data);
          }
          break;
      }
      // reply with current TOD for all commands
      ret = ret || artnet_tx_tod_data(n, i);
    }
  }
  return ret;
}

/**
 * handle rdm packet
 *
 */
void handle_rdm(node n, artnet_packet p) {

  // Art-Net 4: store requester IP for unicast RDM replies
  n->state.rdm_reply_addr = p->from;

  if (check_callback(n, p, n->callbacks.rdm)) {
    return;
  }

  int rdm_data_len = p->length - (sizeof(artnet_rdm_t) - ARTNET_MAX_RDM_DATA);
  if (rdm_data_len < 0) {
    rdm_data_len = 0;
  }

  if (n->callbacks.rdm_c.fh != NULL) {
    int addr = make_addr(p->data.rdm.net, (p->data.rdm.address >> 4) & 0x0F, p->data.rdm.address & 0x0F);
    n->callbacks.rdm_c.fh(n, addr, p->data.rdm.data, rdm_data_len, n->callbacks.rdm_c.data);
  }

  return;
}

/**
 * handle ArtSync packet
 * Flushes all buffered ArtDmx data to output simultaneously.
 * Enters sync mode; reverts to non-sync after 4s timeout (handled in check_timeouts).
 * Art-Net 4: ignores ArtSync if source IP doesn't match last ArtDmx source.
 */
void handle_sync(node n, artnet_packet p) {
  if (check_callback(n, p, n->callbacks.sync)) {
    return;
  }

  // Art-Net 4: ignore ArtSync if source IP doesn't match last ArtDmx source
  if (n->state.last_dmx_source.s_addr != 0 &&
      n->state.last_dmx_source.s_addr != p->from.s_addr) {
    return;
  }

  // Art-Net 4: ignore ArtSync if any output port is merging from different sources
  for (int i = 0; i < ARTNET_MAX_PORTS; i++) {
    if (n->ports.out[i].port_status & PORT_STATUS_MERGE &&
        n->ports.out[i].ipA.s_addr != n->ports.out[i].ipB.s_addr) {
      return;
    }
  }

  n->state.sync_mode = 1;
  n->state.last_sync_time = time(NULL);
}

/**
 * handle ArtNzs packet
 * Non-zero start code DMX data, similar to ArtDmx but with a start code field.
 */
void handle_nzs(node n, artnet_packet p) {
  int i = 0;

  if (check_callback(n, p, n->callbacks.nzs)) {
    return;
  }

  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    if (n->ports.out[i].port_enabled &&
        p->data.admx.universe == htols(n->ports.out[i].port_addr)) {
      if (n->callbacks.dmx_c.fh != NULL) {
        n->callbacks.dmx_c.fh(n, i, n->callbacks.dmx_c.data);
      }
      return;
    }
  }
}

/**
 * handle ArtCommand packet
 * Text-based command. Only processes if OEM matches or is 0xFFFF.
 */
void handle_command(node n, artnet_packet p) {
  uint16_t oem = 0;

  if (check_callback(n, p, n->callbacks.command)) {
    return;
  }

  oem = (p->data.cmd.estaManHi << 8) | p->data.cmd.estaManLo;
  if (oem != 0xFFFF &&
      oem != ((n->state.oem_hi << 8) | n->state.oem_lo)) {
    return;
  }
}

/**
 * handle ArtTimeCode packet
 */
void handle_timecode(node n, artnet_packet p) {
  if (check_callback(n, p, n->callbacks.timecode)) {
    return;
  }
}

/**
 * handle ArtTimeSync packet
 */
void handle_timesync(node n, artnet_packet p) {
  if (check_callback(n, p, n->callbacks.timesync)) {
    return;
  }
}

/**
 * handle ArtTrigger packet
 * Only processes if OEM matches or is 0xFFFF.
 */
void handle_trigger(node n, artnet_packet p) {
  uint16_t oem = 0;

  if (check_callback(n, p, n->callbacks.trigger)) {
    return;
  }

  oem = (p->data.trigger.oemCodeHi << 8) | p->data.trigger.oemCodeLo;
  if (oem != 0xFFFF &&
      oem != ((n->state.oem_hi << 8) | n->state.oem_lo)) {
    return;
  }
}

/**
 * handle ArtDirectory packet
 * Reply with ArtDirectoryReply containing node's file list.
 */
void handle_directory(node n, artnet_packet p) {
  if (check_callback(n, p, n->callbacks.directory)) {
    return;
  }

  artnet_tx_directory_reply(n);
}

/**
 * handle ArtDirectoryReply packet
 */
void handle_directory_reply(node n, artnet_packet p) {
  if (check_callback(n, p, n->callbacks.directory_reply)) {
    return;
  }
}

/**
 * handle ArtFileTnMaster packet
 * File upload to node. Reply with ArtFirmwareReply to acknowledge.
 */
int handle_file_tn_master(node n, artnet_packet p) {
  artnet_firmware_status_code code = ARTNET_FIRMWARE_FAIL;

  if (check_callback(n, p, n->callbacks.file_tn_master)) {
    return ARTNET_EOK;
  }

  if (n->callbacks.firmware_c.fh != NULL) {
    uint16_t data[ARTNET_FIRMWARE_SIZE];
    int length = (int)sizeof(data);
    memcpy(data, p->data.filetn.data, length);
    if (n->callbacks.firmware_c.fh(n, 0, data, length, n->callbacks.firmware_c.data)) {
      code = ARTNET_FIRMWARE_ALLGOOD;
    }
  } else {
    code = ARTNET_FIRMWARE_ALLGOOD;
  }

  return artnet_tx_firmware_reply(n, p->from.s_addr, code);
}

/**
 * handle ArtFileFnMaster packet
 * File download request. Notify application to send ArtFileFnReply.
 */
void handle_file_fn_master(node n, artnet_packet p) {
  if (check_callback(n, p, n->callbacks.file_fn_master)) {
    return;
  }
}

/**
 * handle ArtFileFnReply packet
 * File data received from node.
 */
void handle_file_fn_reply(node n, artnet_packet p) {
  if (check_callback(n, p, n->callbacks.file_fn_reply)) {
    return;
  }
}

/**
 * handle ArtMediaPatch packet
 */
void handle_media_patch(node n, artnet_packet p) {
  if (check_callback(n, p, n->callbacks.mediapatch)) {
    return;
  }
}

/**
 * handle ArtMediaControl packet
 */
void handle_media_control(node n, artnet_packet p) {
  if (check_callback(n, p, n->callbacks.mediacontrol)) {
    return;
  }
}

/**
 * handle a firmware master
 */

// THIS NEEDS TO BE CHECKED FOR BUFFER OVERFLOWS
// IMPORTANT!!!!
int handle_firmware(node n, artnet_packet p) {
  int length = 0, offset = 0, block_length = 0, total_blocks = 0, block_id = 0;
  artnet_firmware_status_code response_code = ARTNET_FIRMWARE_FAIL;

  // run callback if defined
  if (check_callback(n, p, n->callbacks.firmware)) {
    return ARTNET_EOK;
  }

  /*
   * What happens if an upload is less than 512 bytes ?????
   */

  if ( p->data.firmware.type == ARTNET_FIRMWARE_FIRMFIRST ||
       p->data.firmware.type == ARTNET_FIRMWARE_UBEAFIRST) {
    // a new transfer is initiated

    if (n->firmware.peer.s_addr == 0) {
      //new transfer
      // these are 2 byte words, so we get a total of 1k of data per packet
      length = artnet_misc_nbytes_to_32( p->data.firmware.length ) *
        sizeof(p->data.firmware.data[0]);

      // set parameters
      n->firmware.peer.s_addr = p->from.s_addr;
      n->firmware.data = malloc(length);

      if (n->firmware.data  == NULL) {
        artnet_error_malloc();
        return ARTNET_EMEM;
      }
      n->firmware.bytes_total = length;
      n->firmware.last_time = time(NULL);
      n->firmware.expected_block = 1;

      // check if this is a ubea upload or not
      if (p->data.firmware.type == ARTNET_FIRMWARE_FIRMFIRST) {
        n->firmware.ubea = 0;
      } else {
        n->firmware.ubea = 1;
      }

      // take the minimum of the total length and the max packet size
      block_length = min((unsigned int) length, ARTNET_FIRMWARE_SIZE *
        sizeof(p->data.firmware.data[0]));

      memcpy(n->firmware.data, p->data.firmware.data, block_length);
      n->firmware.bytes_current = block_length;

      if (block_length == length) {
        // this is the first and last packet
        // upload was less than 1k bytes
        // this behaviour isn't in the spec, presumably no firmware will be less that 1k
        response_code = ARTNET_FIRMWARE_ALLGOOD;

        // do the callback here
        if (n->callbacks.firmware_c.fh != NULL) {
          n->callbacks.firmware_c.fh(n,
                                     n->firmware.ubea,
                                     n->firmware.data,
                                     n->firmware.bytes_total,
                                     n->callbacks.firmware_c.data);
        }

      } else {
        response_code = ARTNET_FIRMWARE_BLOCKGOOD;
      }

    } else {
      // already in a transfer
      printf("First, but already for a packet\n");

      // send a failure
      response_code = ARTNET_FIRMWARE_FAIL;
    }

  } else if (p->data.firmware.type == ARTNET_FIRMWARE_FIRMCONT ||
             p->data.firmware.type == ARTNET_FIRMWARE_UBEACONT) {
    // continued transfer
    length = artnet_misc_nbytes_to_32(p->data.firmware.length) *
      sizeof(p->data.firmware.data[0]);
    total_blocks = length / ARTNET_FIRMWARE_SIZE / 2 + 1;
    block_length = ARTNET_FIRMWARE_SIZE * sizeof(uint16_t);
    block_id = p->data.firmware.blockId;

    // ok the blockid field is only 1 byte, so it wraps back to 0x00 we
    // need to watch for this
    if (n->firmware.expected_block > UINT8_MAX &&
       (n->firmware.expected_block % (UINT8_MAX+1)) == p->data.firmware.blockId) {

      block_id = n->firmware.expected_block;
    }
    offset = block_id * ARTNET_FIRMWARE_SIZE;

    if (n->firmware.peer.s_addr == p->from.s_addr &&
        length == n->firmware.bytes_total &&
        block_id < total_blocks-1) {

      memcpy(n->firmware.data + offset, p->data.firmware.data, block_length);
      n->firmware.bytes_current += block_length;
      n->firmware.expected_block++;

      response_code = ARTNET_FIRMWARE_BLOCKGOOD;
    } else {
      printf("cont, ips don't match or length has changed or out of range block num\n" );

      // in a transfer not from this ip
      response_code = ARTNET_FIRMWARE_FAIL;
    }

  } else if (p->data.firmware.type == ARTNET_FIRMWARE_FIRMLAST ||
             p->data.firmware.type == ARTNET_FIRMWARE_UBEALAST) {
    length = artnet_misc_nbytes_to_32( p->data.firmware.length) *
      sizeof(p->data.firmware.data[0]);
    total_blocks = length / ARTNET_FIRMWARE_SIZE / 2 + 1;

    // length should be the remaining data
    block_length = n->firmware.bytes_total % (ARTNET_FIRMWARE_SIZE * sizeof(uint16_t));
    block_id = p->data.firmware.blockId;

    // ok the blockid field is only 1 byte, so it wraps back to 0x00 we
    // need to watch for this
    if (n->firmware.expected_block > UINT8_MAX &&
       (n->firmware.expected_block % (UINT8_MAX+1)) == p->data.firmware.blockId) {

      block_id = n->firmware.expected_block;
    }
    offset = block_id * ARTNET_FIRMWARE_SIZE;

    if (n->firmware.peer.s_addr == p->from.s_addr &&
        length == n->firmware.bytes_total &&
        block_id == total_blocks-1) {

      // all the checks work out
      memcpy(n->firmware.data + offset, p->data.firmware.data, block_length);
      n->firmware.bytes_current += block_length;

      // do the callback here
      if (n->callbacks.firmware_c.fh != NULL) {
        n->callbacks.firmware_c.fh(n, n->firmware.ubea,
          n->firmware.data,
          n->firmware.bytes_total / sizeof(p->data.firmware.data[0]),
          n->callbacks.firmware_c.data);
      }

      // reset values and free
      reset_firmware_upload(n);

      response_code = ARTNET_FIRMWARE_ALLGOOD;
      printf("Firmware upload complete\n");

    } else if (n->firmware.peer.s_addr != p->from.s_addr) {
      // in a transfer not from this ip
      printf("last, ips don't match\n" );
      response_code = ARTNET_FIRMWARE_FAIL;
    } else if (length != n->firmware.bytes_total) {
      // they changed the length mid way thru a transfer
      printf("last, lengths have changed %d %d\n", length, n->firmware.bytes_total);
      response_code = ARTNET_FIRMWARE_FAIL;
    } else if (block_id != total_blocks -1) {
      // the blocks don't match up
      printf("This is the last block, but not according to the lengths %d %d\n", block_id, total_blocks -1);
      response_code = ARTNET_FIRMWARE_FAIL;
    }
  }

  return artnet_tx_firmware_reply(n, p->from.s_addr, response_code);
}

/**
 * handle an firmware reply
 */
int handle_firmware_reply(node n, artnet_packet p) {
  node_entry_private_t *ent = NULL;

  // run callback if defined
  if (check_callback(n, p, n->callbacks.firmware_reply)) {
    return ARTNET_EOK;
  }

  ent = find_entry_from_ip(&n->node_list, p->from);

  // node doesn't exist in our list, or we're not doing a transfer to this node
  if (ent== NULL || ent->firmware.bytes_total == 0) {
    return ARTNET_EOK;
  }

  // three types of response, ALLGOOD,  BLOCKGOOD and FIRMFAIL
  if (p->data.firmwarer.type == ARTNET_FIRMWARE_ALLGOOD) {

    if (ent->firmware.bytes_total == ent->firmware.bytes_current) {
      // transfer complete

      // do the callback
      if (ent->firmware.callback != NULL) {
        ent->firmware.callback(n, ARTNET_FIRMWARE_ALLGOOD, ent->firmware.user_data);
      }

      memset(&ent->firmware, 0x0, sizeof(firmware_transfer_t));

    } else {
      // random ALLGOOD received, don't let this abort the transfer
      printf("FIRMWARE_ALLGOOD received before transfer completed\n");
    }

  } else if (p->data.firmwarer.type == ARTNET_FIRMWARE_FAIL) {

    // do the callback
    if (ent->firmware.callback != NULL) {
        ent->firmware.callback(n, ARTNET_FIRMWARE_FAIL, ent->firmware.user_data);
    }

    // cancel transfer
    memset(&ent->firmware, 0x0, sizeof(firmware_transfer_t));

  } else if (p->data.firmwarer.type == ARTNET_FIRMWARE_BLOCKGOOD) {
    // send the next block (only if we're not done yet)
    if (ent->firmware.bytes_total != ent->firmware.bytes_current) {
      return artnet_tx_firmware_packet(n, &ent->firmware);
    }
  }
  return ARTNET_EOK;
}


/*
 * have to sort this one out.
 */
void handle_ipprog(node n, artnet_packet p) {
  uint8_t cmd = p->data.aip.Command;

  if (check_callback(n, p, n->callbacks.ipprog)) {
    return;
  }

  // Art-Net 4 Command field bits (from spec):
  //   bit 7: Enable any programming (gate for all other bits)
  //   bit 6: Enable DHCP (if set, ignore lower bits)
  //   bit 5: unused
  //   bit 4: Program default gateway
  //   bit 3: Reset all to defaults
  //   bit 2: Program IP address
  //   bit 1: Program subnet mask
  //   bit 0: Program port (deprecated)

  // If bit 7 is not set, this is a query only (no programming)
  if (!(cmd & 0x80)) {
    artnet_tx_build_art_poll_reply(n);
    artnet_tx_ipprog_reply(n);
    return;
  }

  // DHCP enable
  if (cmd & 0x40) {
    n->state.report_code = ARTNET_RC_IP_PROG_OK;
    snprintf(n->state.report, ARTNET_REPORT_LENGTH,
             "DHCP Enable requested (not supported in software)");
  }
  // Reset all to defaults
  else if (cmd & 0x08) {
    n->state.report_code = ARTNET_RC_IP_PROG_OK;
    snprintf(n->state.report, ARTNET_REPORT_LENGTH,
             "Reset to defaults requested");
  }
  // Program IP address (bit 2)
  else if (cmd & 0x04) {
    struct in_addr new_ip;
    new_ip.s_addr = (p->data.aip.ProgIpHi << 24) | (p->data.aip.ProgIp2 << 16) |
                    (p->data.aip.ProgIp1 << 8) | p->data.aip.ProgIpLo;
    n->state.ip_addr = new_ip;
    n->state.reply_addr = new_ip;
    // Recompute broadcast address from new IP and subnet mask
    n->state.bcast_addr.s_addr =
        (new_ip.s_addr & n->state.subnet_mask.s_addr) | (~n->state.subnet_mask.s_addr);
    n->state.report_code = ARTNET_RC_IP_PROG_OK;
    snprintf(n->state.report, ARTNET_REPORT_LENGTH,
             "IP Programmed [%d.%d.%d.%d]",
             p->data.aip.ProgIpHi, p->data.aip.ProgIp2,
             p->data.aip.ProgIp1, p->data.aip.ProgIpLo);
  }
  // Program subnet mask (bit 1)
  else if (cmd & 0x02) {
    struct in_addr new_mask;
    new_mask.s_addr = (p->data.aip.ProgSmHi << 24) | (p->data.aip.ProgSm2 << 16) |
                      (p->data.aip.ProgSm1 << 8) | p->data.aip.ProgSmLo;
    n->state.subnet_mask = new_mask;
    // Recompute broadcast address from current IP and new subnet mask
    n->state.bcast_addr.s_addr =
        (n->state.ip_addr.s_addr & new_mask.s_addr) | (~new_mask.s_addr);
    n->state.report_code = ARTNET_RC_IP_PROG_OK;
    snprintf(n->state.report, ARTNET_REPORT_LENGTH,
             "Subnet Programmed [%d.%d.%d.%d]",
             p->data.aip.ProgSmHi, p->data.aip.ProgSm2,
             p->data.aip.ProgSm1, p->data.aip.ProgSmLo);
  }
  // Program default gateway (bit 4)
  else if (cmd & 0x10) {
    n->state.report_code = ARTNET_RC_IP_PROG_OK;
    snprintf(n->state.report, ARTNET_REPORT_LENGTH,
             "Gateway Programmed [%d.%d.%d.%d]",
             p->data.aip.ProgDgHi, p->data.aip.ProgDg2,
             p->data.aip.ProgDg1, p->data.aip.ProgDgLo);
  }

  // Fire program callback to notify application
  if (n->callbacks.program_c.fh != NULL) {
    n->callbacks.program_c.fh(n, n->callbacks.program_c.data);
  }

  // Rebuild and send ArtPollReply with updated report
  artnet_tx_build_art_poll_reply(n);
  artnet_tx_poll_reply(n, TRUE);

  // Send ArtIpProgReply
  artnet_tx_ipprog_reply(n);
}


/*
 * The main handler for an artnet packet. calls
 * the appropriate handler function
 */
int handle(node n, artnet_packet p) {

  if (check_callback(n, p, n->callbacks.recv)) {
    return 0;
  }

  switch (p->type) {
    case ARTNET_POLL:
      handle_poll(n, p);
      break;
    case ARTNET_REPLY:
      handle_reply(n,p);
      break;
    case ARTNET_DMX:
      handle_dmx(n, p);
      break;
    case ARTNET_ADDRESS:
      handle_address(n, p);
      break;
    case ARTNET_INPUT:
      _artnet_handle_input(n, p);
      break;
    case ARTNET_TODREQUEST:
      handle_tod_request(n, p);
      break;
    case ARTNET_TODDATA:
      handle_tod_data(n, p);
      break;
    case ARTNET_TODCONTROL:
      handle_tod_control(n, p);
      break;
    case ARTNET_RDM:
      handle_rdm(n, p);
      break;
    case ARTNET_RDMSUB:
      // ArtRdmSub: compressed RDM sub-device data
      // store requester IP for unicast replies
      n->state.rdm_reply_addr = p->from;
      if (check_callback(n, p, n->callbacks.rdm)) {
        break;
      }
      break;
    case ARTNET_DIAGDATA:
      // generic recv callback already checked above, no specific handler needed
      break;
    case ARTNET_SYNC:
      handle_sync(n, p);
      break;
    case ARTNET_NZS:
      handle_nzs(n, p);
      break;
    case ARTNET_COMMAND:
      handle_command(n, p);
      break;
    case ARTNET_TIMECODE:
      handle_timecode(n, p);
      break;
    case ARTNET_TIMESYNC:
      handle_timesync(n, p);
      break;
    case ARTNET_TRIGGER:
      handle_trigger(n, p);
      break;
    case ARTNET_DIRECTORY:
      handle_directory(n, p);
      break;
    case ARTNET_DIRECTORYREPLY:
      handle_directory_reply(n, p);
      break;
    case ARTNET_FILETNMASTER:
      handle_file_tn_master(n, p);
      break;
    case ARTNET_FILEFNMASTER:
      handle_file_fn_master(n, p);
      break;
    case ARTNET_FILEFNREPLY:
      handle_file_fn_reply(n, p);
      break;
    case ARTNET_MEDIA:
      if (check_callback(n, p, n->callbacks.mediapatch)) {
        break;
      }
      break;
    case ARTNET_MEDIAPATCH:
      handle_media_patch(n, p);
      break;
    case ARTNET_MEDIACONTROL:
      handle_media_control(n, p);
      break;
    case ARTNET_MEDIACONTROLREPLY:
      if (check_callback(n, p, n->callbacks.mediacontrol)) {
        break;
      }
      break;
    case ARTNET_VIDEOSTEUP:
      printf("vid setup\n");
      break;
    case ARTNET_VIDEOPALETTE:
      printf("video palette\n");
      break;
    case ARTNET_VIDEODATA:
      printf("video data\n");
      break;
    case ARTNET_MACMASTER:
      printf("mac master\n");
      break;
    case ARTNET_MACSLAVE:
      printf("mac slave\n");
      break;
    case ARTNET_FIRMWAREMASTER:
      handle_firmware(n, p);
      break;
    case ARTNET_FIRMWAREREPLY:
      handle_firmware_reply(n, p);
      break;
    case ARTNET_IPPROG :
      handle_ipprog(n, p);
      break;
    case ARTNET_IPREPLY:
      // generic recv callback already checked above
      break;
    default:
      n->state.report_code = ARTNET_RC_PARSE_FAIL;
      printf("artnet but not yet implemented!, op was %x\n", (int) p->type);
  }
  return 0;
}

/**
 * this gets the opcode from a packet
 */
int16_t get_type(artnet_packet p) {
  uint8_t *data;

  if (p->length < 10) {
    return 0;
  }
  if (!memcmp(&p->data, "Art-Net\0", 8)) {
    // not the best here, this needs to be tested on different arch
    data = (uint8_t *) &p->data;

    p->type = (data[9] << 8) + data[8];
    return p->type;
  } else {
    return 0;
  }
}

/*
 *
 */
void check_merge_timeouts(node n, int port_id) {
  output_port_t *port = NULL;
  clock_t now = 0;
  clock_t timeoutA = 0, timeoutB = 0;
  int was_merging = 0;
  port = &n->ports.out[port_id];
  now = clock();
  timeoutA = (now - port->timeA) * 1000 / CLOCKS_PER_SEC;
  timeoutB = (now - port->timeB) * 1000 / CLOCKS_PER_SEC;
  was_merging = (port->port_status & PORT_STATUS_MERGE) && port->ipA.s_addr && port->ipB.s_addr;

  if ((int)timeoutA > MERGE_TIMEOUT_MS) {
    // A is old, stop the merge
    port->ipA.s_addr = 0;
  }

  if ((int)timeoutB > MERGE_TIMEOUT_MS) {
    // B is old, stop the merge
    port->ipB.s_addr = 0;
  }

  // merge ended: only one or zero sources remain
  if (was_merging && !(port->ipA.s_addr && port->ipB.s_addr)) {
    port->port_status &= ~PORT_STATUS_MERGE;
    if (n->state.send_apr_on_change) {
      artnet_tx_poll_reply(n, TRUE);
    }
  }
}


/*
 * merge the data from two sources
 */
void merge(node n, int port_id, int length, uint8_t *latest) {
  int i = 0;
  output_port_t *port = NULL;
  port = &n->ports.out[port_id];

  if (port->merge_mode == ARTNET_MERGE_HTP) {
    for (i=0; i< length; i++) {
      port->data[i] = max(port->dataA[i], port->dataB[i]);
    }
  } else {
    memcpy(port->data, latest, length);
  }
}


void reset_firmware_upload(node n) {
  n->firmware.bytes_current = 0;
  n->firmware.bytes_total = 0;
  n->firmware.peer.s_addr = 0;
  n->firmware.ubea = 0;
  n->firmware.last_time = 0;
  free(n->firmware.data);
}
