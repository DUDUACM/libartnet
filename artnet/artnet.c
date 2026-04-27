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
 * artnet.c
 * Implementes the external functions for libartnet
 * Copyright (C) 2004-2007 Simon Newton
 */
#include "private.h"

// various constants used everywhere
int ARTNET_ADDRESS_NO_CHANGE = 0x7f;
int ARTNET_PORT = 6454;
int ARTNET_STRING_SIZE = 8;
char ARTNET_STRING[] = "Art-Net";
uint8_t ARTNET_VERSION = 14;
uint8_t OEM_HI = 0x04;
uint8_t OEM_LO = 0x30;
char ESTA_HI = 'z';
char ESTA_LO = 'p';
uint8_t TTM_BEHAVIOUR_MASK = 0x02;
uint8_t TTM_REPLY_MASK = 0x01;
uint8_t PROGRAM_NO_CHANGE = 0x7f;
uint8_t PROGRAM_DEFAULTS = 0x00;
uint8_t PROGRAM_CHANGE_MASK = 0x80;
uint8_t HIGH_NIBBLE = 0xF0;
uint8_t LOW_NIBBLE = 0x0F;
uint8_t STATUS_PROG_AUTH_MASK = 0x30;
uint8_t PORT_STATUS_LPT_MODE = 0x02;
uint8_t PORT_STATUS_SHORT = 0x04;
uint8_t PORT_STATUS_ERROR = 0x04;
uint8_t PORT_STATUS_DISABLED_MASK = 0x08;
uint8_t PORT_STATUS_MERGE = 0x08;
uint8_t PORT_STATUS_DMX_TEXT = 0x10;
uint8_t PORT_STATUS_DMX_SIP = 0x20;
uint8_t PORT_STATUS_DMX_TEST = 0x40;
uint8_t PORT_STATUS_ACT_MASK = 0x80;
uint8_t PORT_DISABLE_MASK = 0x01;
uint8_t TOD_RESPONSE_FULL = 0x00;
uint8_t TOD_RESPONSE_NAK = 0xff;
uint8_t MIN_PACKET_SIZE = 10;
int MERGE_TIMEOUT_MS = 10000;
int NODELIST_TIMEOUT_MS = 15000;
int DMX_FAILSAFE_TIMEOUT_MS = 800;
int FIRMWARE_TIMEOUT_MS = 30000;
int ARTSYNC_TIMEOUT_MS = 4000;
int DMX_KEEPALIVE_INTERVAL_MS = 1000;
uint8_t RECV_NO_DATA = 1;
uint8_t MAX_NODE_BCAST_LIMIT = 30; // always bcast after this point

#ifndef TRUE
int TRUE = 1;
int FALSE = 0;
#endif

uint16_t LOW_BYTE = 0x00FF;
uint16_t HIGH_BYTE = 0xFF00;

void copy_apr_to_node_entry(artnet_node_entry e, artnet_reply_t *reply);
int find_nodes_from_uni(node n, node_list_t *nl, uint16_t uni, SI *ips, int size);

/**
 * Creates a new ArtNet node.
 * Takes a string containing the ip address to bind to, if the string is NULL
 * it uses the first non loopback address
 *
 * @param ip the IP address to bind to
 * @param debug level of logging provided 0: none
 * @return an artnet_node, or NULL on failure
 */
artnet_node artnet_new(const char *ip, int verbose) {
  node n = NULL;
  int i = 0;

  n = malloc(sizeof(artnet_node_t));

  if (!n) {
    artnet_error("malloc failure");
    return NULL;
  }

  memset(n, 0x0, sizeof(artnet_node_t));

  // init node listing
  n->node_list.first = NULL;
  n->node_list.current = NULL;
  n->node_list.last = NULL;
  n->node_list.length = 0;
  n->state.verbose = verbose;
  n->state.oem_hi = OEM_HI;
  n->state.oem_lo = OEM_LO;
  n->state.esta_hi = ESTA_HI;
  n->state.esta_lo = ESTA_LO;
  n->state.bcast_limit = 0;

  n->peering.peer = NULL;
  n->peering.master = TRUE;

  n->sd = INVALID_SOCKET;

  if (artnet_net_init(n, ip)) {
    free(n);
    return NULL;
  }

  // now setup the default parameters
  n->state.send_apr_on_change = FALSE;
  n->state.ar_count = 0;
  n->state.report_code = ARTNET_RC_POWER_OK;
  n->state.reply_addr.s_addr = 0;
  n->state.mode = ARTNET_STANDBY;
  n->state.status2 = ARTNET_STATUS2_15BIT_ADDR | ARTNET_STATUS2_RDM_CONTROL;
  n->state.acn_priority = 0xFF;  // 0xFF = no change
  memset(n->state.default_resp_uid, 0, ARTNET_RDM_UID_WIDTH);

  // set all ports to MERGE HTP mode and disable
  for (i=0; i < ARTNET_MAX_PORTS; i++) {
    n->ports.out[i].merge_mode = ARTNET_MERGE_HTP;
    n->ports.out[i].port_enabled = FALSE;
    n->ports.out[i].last_dmx_time = 0;
    n->ports.out[i].failsafe_triggered = FALSE;
    n->ports.out[i].failsafe_length = 0;
    memset(n->ports.out[i].failsafe_data, 0, ARTNET_DMX_LENGTH);
    n->ports.in[i].port_enabled = FALSE;
    n->ports.in[i].last_dmx_send_time = 0;
    n->ports.in[i].last_dmx_length = 0;
    memset(n->ports.in[i].last_dmx_data, 0, ARTNET_DMX_LENGTH);

    // reset tods
    reset_tod(&n->ports.in[i].port_tod);
    reset_tod(&n->ports.out[i].port_tod);
  }
  return n;
}


/**
 * Starts the ArtNet node.
 * Binds the network socket and sends an ArtPoll
 * @param vn the artnet_node
 * @return 0 on success, non 0 on failure
 *
 */
int artnet_start(artnet_node vn) {
  node n = (node) vn;
  int ret = 0;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_STANDBY) {
    return ARTNET_ESTATE;
  }

  if ((ret = artnet_net_start(n)) != 0) {
    return ret;
  }

  n->state.mode = ARTNET_ON;

  if (n->state.reply_addr.s_addr == 0) {
    n->state.reply_addr = n->state.bcast_addr;
  }

  // build the initial reply
  if ((ret = artnet_tx_build_art_poll_reply(n)) != 0) {
    return ret;
  }

  if (n->state.node_type == ARTNET_SRV) {
    // poll the network
    if ((ret = artnet_tx_poll(n,NULL, ARTNET_TTM_AUTO)) != 0) {
      return ret;
    }

    if ((ret = artnet_tx_tod_request(n)) != 0) {
      return ret;
    }
  }
  // Art-Net 4: nodes must not send unsolicited ArtPollReply at startup
  // wait for ArtPoll before replying
  return ret;
}


/**
 * Stops the ArtNet node. This closes the network sockets held by the node
 * @param vn the artnet_node
 * @return 0 on success, non-0 on failure
 */
int artnet_stop(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  artnet_net_close(n->sd);
  n->state.mode = ARTNET_STANDBY;
  return ARTNET_EOK;
}


/**
 * Free the memory associated with this node
 */
int artnet_destroy(artnet_node vn) {
  node n = (node) vn;
  node_entry_private_t *ent = NULL, *tmp = NULL;
  int i = 0;

  check_nullnode(vn);

  // free any memory associated with firmware transfers
  for (ent = n->node_list.first; ent != NULL; ent = tmp) {
    if (ent->firmware.data != NULL) {
      free(ent->firmware.data);
      ent->firmware.data = NULL;
    }
    tmp = ent->next;
    free(ent);
    ent = NULL;
  }

  // free node's own firmware transfer buffer if any
  if (n->firmware.data != NULL) {
    free(n->firmware.data);
    n->firmware.data = NULL;
  }

  for (i =0; i < ARTNET_MAX_PORTS; i++) {
    flush_tod(&n->ports.in[i].port_tod);
    flush_tod(&n->ports.out[i].port_tod);
  }

  // close socket if node is still running
  if (n->state.mode == ARTNET_ON) {
    artnet_net_close(n->sd);
  }

  free(vn);
  return ARTNET_EOK;
}


/**
 * Set the OEM code
 * This can only be done in the standby state
 */
int artnet_setoem(artnet_node vn, uint8_t hi, uint8_t lo) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_STANDBY) {
    return ARTNET_ESTATE;
  }

  n->state.oem_hi = hi;
  n->state.oem_lo = lo;
  return ARTNET_EOK;
}


/**
 * Set the ESTA code
 * This can only be done in the standby state
 */
int artnet_setesta(artnet_node vn, char hi, char lo) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_STANDBY) {
    return ARTNET_ESTATE;
  }

  n->state.esta_hi = hi;
  n->state.esta_lo = lo;
  return ARTNET_EOK;
}


/**
 * Set the number of nodes above which we start to bcast data
 * @param vn the artnet_node
 * @param limit 0 to always broadcast
 */
int artnet_set_bcast_limit(artnet_node vn, int limit) {
  node n = (node) vn;
  check_nullnode(vn);

  if (limit > MAX_NODE_BCAST_LIMIT) {
    artnet_error("attempt to set bcast limit > %d", MAX_NODE_BCAST_LIMIT);
    return ARTNET_EARG;
  }

  n->state.bcast_limit = limit;
  return ARTNET_EOK;
}

/**
 * Handle any received packets.
 * This function is the workhorse of libartnet. You have a couple of options:
 *   - use artnet_get_sd() to retrieve the socket descriptors and select to
 *     detect network activity. Then call artnet_read(node,0)
 *     when activity is detected.
 *   - call artnet_read repeatedly from within a loop with an appropriate
 *     timeout
 *
 * @param vn the artnet_node
 * @param timeout the number of milliseconds to block for if nothing is pending
 * @return ARTNET_EOK on success, or a negative ARTNET_E* error code on failure
 */
int artnet_read(artnet_node vn, int timeout) {
  node n = (node) vn;
  node tmp = NULL;
  artnet_packet_t p = {0};
  int ret = 0;

  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  while (1) {
    memset(&p.data, 0x0, sizeof(p.data));

    // check timeouts now, else this packet may update the timestamps
    check_timeouts(n);

    if ((ret = artnet_net_recv(n, &p, timeout)) < 0) {
      return ret;
    }

    // nothing to read
    if (ret == RECV_NO_DATA) {
      break;
    }

    // skip this packet (filtered)
    if (p.length == 0) {
      continue;
    }

    for (tmp = n->peering.peer; tmp != NULL && tmp != n; tmp = tmp->peering.peer) {
      check_timeouts(tmp);
    }

    if (p.length > MIN_PACKET_SIZE && get_type(&p)) {
      handle(n, &p);
      for (tmp = n->peering.peer; tmp != NULL && tmp != n; tmp = tmp->peering.peer) {
        handle(tmp, &p);
      }
    }
  }
  return ARTNET_EOK;
}


/**
 * To get around the 4 universes per node limitation , we can start more than
 * one node on different ip addresses - you'll need to add aliases to your
 * network interface something like:
 *
 * $ ifconfig eth0:1 10.0.0.10 netmask 255.255.255.0
 *
 * Then the nodes must be joined so that they can share the socket
 * bound to the broadcast address.
 * Uses IP_PKTINFO to ensure packets are sent from the correct source IP.
 *
 * @param vn1 The artnet node
 * @param vn2 The second artnet node
 *
 * @return 0 on sucess, non 0 on failure
 */
int artnet_join(artnet_node vn1, artnet_node vn2) {

  check_nullnode(vn1);
  check_nullnode(vn2);

  node n1 = (node) vn1;
  node n2 = (node) vn2;
  node tmp = NULL, n = NULL;

  if (n1->state.mode == ARTNET_ON || n2->state.mode == ARTNET_ON) {
    artnet_error("%s called after artnet_start", __FUNCTION__);
    return ARTNET_EACTION;
  }

  tmp = n1->peering.peer == NULL ? n1 : n1->peering.peer;
  n1->peering.peer = n2;
  for (n = n2; n->peering.peer != NULL && n->peering.peer != n2; n = n->peering.peer) { }
  n->peering.peer = tmp;

  // make sure there is only 1 master
  for (n = n1->peering.peer; n != n1; n = n->peering.peer) {
    n->peering.master = FALSE;
  }

  n1->peering.master = TRUE;

  return ARTNET_EOK;
}


/**
 * This is used to set handlers for sent/received artnet packets.
 * If you're using a stock standard node you more than likely don't want
 * to set these. See the artnet_set_dmx_callback and artnet_set_firmware_callback.
 * If you want to get down and dirty with artnet packets, you can set this
 * read / manipulate packets as they arrive (or get sent)
 *
 * @param vn The artnet_node
 * @param handler The handler to set
 * @param fh A pointer to a function, set to NULL to turn off
 *           The function should return 0,
 * @param data Data to be passed to the handler when its called
 * @return 0 on sucess, non 0 on failure
 */
int artnet_set_handler(artnet_node vn,
                       artnet_handler_name_t handler,
                       int (*fh)(artnet_node n, void *pp, void * d),
                       void *data) {
  node n = (node) vn;
  callback_t *callback = NULL;
  check_nullnode(vn);

  switch(handler) {
    case ARTNET_RECV_HANDLER:
      callback = &n->callbacks.recv;
      break;
    case ARTNET_SEND_HANDLER:
      callback = &n->callbacks.send;
      break;
    case ARTNET_POLL_HANDLER:
      callback = &n->callbacks.poll;
      break;
    case ARTNET_REPLY_HANDLER:
      callback = &n->callbacks.reply;
      break;
    case ARTNET_ADDRESS_HANDLER:
      callback = &n->callbacks.address;
      break;
    case ARTNET_INPUT_HANDLER:
      callback = &n->callbacks.input;
      break;
    case ARTNET_DMX_HANDLER:
      callback = &n->callbacks.dmx;
      break;
    case ARTNET_TOD_REQUEST_HANDLER:
      callback = &n->callbacks.todrequest;
      break;
    case ARTNET_TOD_DATA_HANDLER:
      callback = &n->callbacks.toddata;
      break;
    case ARTNET_TOD_CONTROL_HANDLER:
      callback = &n->callbacks.todcontrol;
      break;
    case ARTNET_RDM_HANDLER:
      callback = &n->callbacks.rdm;
      break;
    case ARTNET_IPPROG_HANDLER:
      callback = &n->callbacks.ipprog;
      break;
    case ARTNET_FIRMWARE_HANDLER:
      callback = &n->callbacks.firmware;
      break;
    case ARTNET_FIRMWARE_REPLY_HANDLER:
      callback = &n->callbacks.firmware_reply;
      break;
    case ARTNET_SYNC_HANDLER:
      callback = &n->callbacks.sync;
      break;
    case ARTNET_NZS_HANDLER:
      callback = &n->callbacks.nzs;
      break;
    case ARTNET_DIAGDATA_HANDLER:
      callback = &n->callbacks.diagdata;
      break;
    case ARTNET_COMMAND_HANDLER:
      callback = &n->callbacks.command;
      break;
    case ARTNET_TIMECODE_HANDLER:
      callback = &n->callbacks.timecode;
      break;
    case ARTNET_TIMESYNC_HANDLER:
      callback = &n->callbacks.timesync;
      break;
    case ARTNET_TRIGGER_HANDLER:
      callback = &n->callbacks.trigger;
      break;
    case ARTNET_DIRECTORY_HANDLER:
      callback = &n->callbacks.directory;
      break;
    case ARTNET_DIRECTORY_REPLY_HANDLER:
      callback = &n->callbacks.directory_reply;
      break;
    case ARTNET_FILE_TN_MASTER_HANDLER:
      callback = &n->callbacks.file_tn_master;
      break;
    case ARTNET_FILE_FN_MASTER_HANDLER:
      callback = &n->callbacks.file_fn_master;
      break;
    case ARTNET_FILE_FN_REPLY_HANDLER:
      callback = &n->callbacks.file_fn_reply;
      break;
    case ARTNET_MEDIAPATCH_HANDLER:
      callback = &n->callbacks.mediapatch;
      break;
    case ARTNET_MEDIA_HANDLER:
      callback = &n->callbacks.media;
      break;
    case ARTNET_MEDIACONTROL_HANDLER:
      callback = &n->callbacks.mediacontrol;
      break;
    case ARTNET_DATAREQUEST_HANDLER:
      callback = &n->callbacks.datareq;
      break;
    case ARTNET_DATAREPLY_HANDLER:
      callback = &n->callbacks.datarep;
      break;
    default:
      artnet_error("%s : Invalid handler defined", __FUNCTION__);
      return ARTNET_EARG;
  }
  callback->fh = fh;
  callback->data = data;
  return ARTNET_EOK;
}


/**
 * Register a DMX receive handler callback.
 *
 * @param vn   The artnet_node
 * @param fh   The callback to invoke (parameters passed are the artnet_node, the port_id
 *             that received the dmx, and user data)
 * @param data Data to be passed to the handler when it's called
 */
int artnet_set_dmx_handler(artnet_node vn,
                           int (*fh)(artnet_node n, int port, void *d),
                           void *data) {
  node n = (node) vn;
  check_nullnode(vn);

  n->callbacks.dmx_c.fh = fh;
  n->callbacks.dmx_c.data = data;
  return ARTNET_EOK;
}


/**
 * Register a firmware upload handler callback.
 *
 * @param vn   The artnet_node
 * @param fh   The callback to invoke (parameters passed are the artnet_node, a value which
 *             is true if this was a ubea upload, and user data)
 * @param data Data to be passed to the handler when it's called
 */
int artnet_set_firmware_handler(
    artnet_node vn,
    int (*fh)(artnet_node n, int ubea, uint16_t *data, int length, void *d),
    void *data) {
  node n = (node) vn;
  check_nullnode(vn);
  n->callbacks.firmware_c.fh = fh;
  n->callbacks.firmware_c.data = data;
  return ARTNET_EOK;
}


/**
 * Register a program change handler (invoked on ArtAddress reprogramming).
 *
 * @param vn   The artnet_node
 * @param fh   The callback to invoke (parameters passed are the artnet_node and user data)
 * @param data Data to be passed to the handler when it's called
 */
int artnet_set_program_handler(artnet_node vn,
                               int (*fh)(artnet_node n, void *d),
                               void *data) {
  node n = (node) vn;
  check_nullnode(vn);
  n->callbacks.program_c.fh = fh;
  n->callbacks.program_c.data = data;
  return ARTNET_EOK;
}


/**
 *
 * @param vn     The artnet_node
 * @param fh    The callback to invoke (parameters passed are the artnet_node, pointer to the
 *             rdm data, the length of the data and the user data
 * @param data    Data to be passed to the handler when its called
 *
 */
int artnet_set_rdm_handler(
    artnet_node vn,
    int (*fh)(artnet_node n, int address, uint8_t *rdm, int length, void *d),
    void *data) {
  node n = (node) vn;
  check_nullnode(vn);
  n->callbacks.rdm_c.fh = fh;
  n->callbacks.rdm_c.data = data;
  return ARTNET_EOK;
}
/**
 * Register an RDM initiate handler callback.
 *
 * @param vn   The artnet_node
 * @param fh   The callback to invoke (parameters passed are the artnet_node, the port id, and user data)
 * @param data Data to be passed to the handler when it's called
 */
int artnet_set_rdm_initiate_handler(
    artnet_node vn,
    int (*fh)(artnet_node n, int port, void *d),
    void *data) {
  node n = (node) vn;
  check_nullnode(vn);

  n->callbacks.rdm_init_c.fh = fh;
  n->callbacks.rdm_init_c.data = data;
  return ARTNET_EOK;
}
/**
 * Register an RDM TOD (Table of Devices) handler callback.
 *
 * @param vn   The artnet_node
 * @param fh   The callback to invoke (parameters passed are the artnet_node, the universe address, and user data)
 * @param data Data to be passed to the handler when it's called
 */
int artnet_set_rdm_tod_handler(
    artnet_node vn,
    int (*fh)(artnet_node n, int address, void *d),
    void *data) {
  node n = (node) vn;
  check_nullnode(vn);

  n->callbacks.rdm_tod_c.fh = fh;
  n->callbacks.rdm_tod_c.data = data;
  return ARTNET_EOK;
}


// sends a poll to the specified ip, or if null, will broadcast
// talk_to_me - modify remote nodes behaviour, see spec
// stale nodes are cleaned up in check_timeouts() after NODELIST_TIMEOUT_MS

/**
 *
 * @param vn the artnet_node
 * @param ip the ip address to send to, NULL will broadcast the ArtPoll
 * @param talk_to_me the value for the talk to me
 */
int artnet_send_poll(artnet_node vn,
                     const char *ip,
                     artnet_ttm_value_t talk_to_me) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    return artnet_tx_poll(n, ip, talk_to_me);
  }

  artnet_error("%s : Not sending poll, not a server or raw device", __FUNCTION__);
  return ARTNET_ESTATE;
}


/**
 * Sends an artpoll reply
 *
 * @param vn the artnet_node
 */
int artnet_send_poll_reply(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  return artnet_tx_poll_reply(n, FALSE);
}


/**
 * Send a diagnostic message
 *
 * @param vn the artnet_node
 * @param priority the diagnostic priority level
 * @param port the logical port (0 for general messages)
 * @param text the diagnostic text message (null-terminated)
 */
int artnet_send_diagnostic(artnet_node vn,
                           artnet_diag_priority_t priority,
                           uint8_t port,
                           const char *text) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_diagdata(n, (uint8_t) priority, port, text);
}

/**
 * Sends some dmx data
 *
 * @param vn the artnet_node
 */
int artnet_send_dmx(artnet_node vn,
                    int port_id,
                    int16_t length,
                    const uint8_t *data) {
  node n = (node) vn;
  artnet_packet_t p = {0};
  input_port_t *port = NULL;

  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port_id);
    return ARTNET_EARG;
  }
  port = &n->ports.in[port_id];

  if (length < 2 || length > ARTNET_DMX_LENGTH) {
    artnet_error("%s : Length of dmx data out of bounds (%i < 2 || %i > ARTNET_MAX_DMX)", __FUNCTION__, length);
    return ARTNET_EARG;
  }

  if (port->port_status & PORT_STATUS_DISABLED_MASK) {
    artnet_error("%s : attempt to send on a disabled port (id:%i)", __FUNCTION__, port_id);
    return ARTNET_EARG;
  }

  // ok we're going to send now, make sure we turn the activity bit on
  port->port_status = port->port_status | PORT_STATUS_ACT_MASK;

  p.length = sizeof(artnet_dmx_t) - (ARTNET_DMX_LENGTH - length);

  // now build packet
  memcpy(&p.data.admx.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.admx.opCode =  htols(ARTNET_DMX);
  p.data.admx.verH = 0;
  p.data.admx.ver = ARTNET_VERSION;
  p.data.admx.sequence = port->seq;
  p.data.admx.physical = (uint8_t)port_id;
  p.data.admx.universe = htols(port->port_addr);

  // set length
  p.data.admx.lengthHi = short_get_high_byte(length);
  p.data.admx.length = short_get_low_byte(length);
  memcpy(&p.data.admx.data, data, length);

  // default to bcast
  p.to.s_addr = n->state.bcast_addr.s_addr;

  if (n->state.bcast_limit == 0) {
    // Art-Net 4: ArtDmx must always be unicast, broadcast is not permitted
    int nodes = 0, i = 0;
    int limit = n->node_list.length ? n->node_list.length : 1;
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
  } else {
    int nodes = 0, i = 0;
    SI *ips = malloc(sizeof(SI) * n->state.bcast_limit);

    if (!ips) {
      return ARTNET_EACTION;
    }

    nodes = find_nodes_from_uni(n, &n->node_list,
                                port->port_addr,
                                ips,
                                n->state.bcast_limit);

    // Art-Net 4: unicast to subscribers, never broadcast
    for (i = 0; i < nodes && i < n->state.bcast_limit; i++) {
      p.to = ips[i];
      artnet_net_send(n, &p);
    }
    free(ips);
  }
  port->seq++;

  // Store data for input keepalive retransmission (Art-Net 4)
  memcpy(port->last_dmx_data, data, length);
  port->last_dmx_length = length;
  port->last_dmx_send_time = time(NULL);

  return ARTNET_EOK;
}


/**
 * Use for performance testing.
 * This allows data to be sent on any universe, not just the ones that have
 * ports configured.
 */
int artnet_raw_send_dmx(artnet_node vn,
                        uint16_t uni,
                        int16_t length,
                        const uint8_t *data) {
  node n = (node) vn;
  artnet_packet_t p = {0};

  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (n->state.node_type != ARTNET_RAW) {
    return ARTNET_ESTATE;
  }

  uni &= 0x7FFF; // mask to valid 15-bit universe address

  if ( length < 1 || length > ARTNET_DMX_LENGTH) {
    artnet_error("%s : Length of dmx data out of bounds (%i < 1 || %i > ARTNET_MAX_DMX)", __FUNCTION__, length);
    return ARTNET_EARG;
  }

  // set dst addr and length
  p.to.s_addr = n->state.bcast_addr.s_addr;

  p.length = sizeof(artnet_dmx_t) - (ARTNET_DMX_LENGTH - length);

  // now build packet
  memcpy( &p.data.admx.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.admx.opCode = htols(ARTNET_DMX);
  p.data.admx.verH = 0;
  p.data.admx.ver = ARTNET_VERSION;
  p.data.admx.sequence = 0;
  p.data.admx.physical = 0;
  p.data.admx.universe = htols(uni);

  // set length
  p.data.admx.lengthHi = short_get_high_byte(length);
  p.data.admx.length = short_get_low_byte(length);
  memcpy(&p.data.admx.data, data, length);

  return artnet_net_send(n, &p);
}
/**
 * Send an ArtNZs (ArtNet Zero Start) packet with a custom start code.
 * Only available when node type is ARTNET_RAW.
 *
 * @param vn          the artnet_node
 * @param uni         the 15-bit universe address
 * @param start_code  the DMX start code (must not be 0x00 or 0xCC)
 * @param length      the length of the data (1-512)
 * @param data        the data to send
 */
int artnet_send_nzs(artnet_node vn,
                    uint16_t uni,
                    uint8_t start_code,
                    int16_t length,
                    const uint8_t *data) {
  node n = (node) vn;
  artnet_packet_t p = {0};

  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (n->state.node_type != ARTNET_RAW) {
    return ARTNET_ESTATE;
  }

  if (start_code == 0x00 || start_code == 0xCC) {
    artnet_error("%s : Invalid start code (0x%02x)", __FUNCTION__, start_code);
    return ARTNET_EARG;
  }

  if (length < 1 || length > ARTNET_DMX_LENGTH) {
    artnet_error("%s : Length of data out of bounds (%i < 1 || %i > 512)", __FUNCTION__, length, length);
    return ARTNET_EARG;
  }

  uni &= 0x7FFF;

  p.to.s_addr = n->state.bcast_addr.s_addr;
  p.length = sizeof(artnet_nzs_t) - (ARTNET_DMX_LENGTH - length);

  memcpy(&p.data.nzs.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.nzs.opCode = htols(ARTNET_NZS);
  p.data.nzs.verH = 0;
  p.data.nzs.ver = ARTNET_VERSION;
  p.data.nzs.sequence = 0;
  p.data.nzs.startCode = start_code;
  p.data.nzs.universe = htols(uni);
  p.data.nzs.lengthHi = short_get_high_byte(length);
  p.data.nzs.length = short_get_low_byte(length);
  memcpy(&p.data.nzs.data, data, length);

  return artnet_net_send(n, &p);
}
/**
 * Send an ArtAddress packet to reprogram a remote node.
 *
 * @param vn         the artnet_node
 * @param e          the node entry to address
 * @param shortName  the new short name
 * @param longName   the new long name
 * @param inAddr     array of input port addresses
 * @param outAddr    array of output port addresses
 * @param netAddr    the net address
 * @param subAddr    the subnet address
 * @param cmd        the port command
 * @param acnPriority the sACN priority value
 */
int artnet_send_address(artnet_node vn,
                        artnet_node_entry e,
                        const char *shortName,
                        const char *longName,
                        uint8_t inAddr[ARTNET_MAX_PORTS],
                        uint8_t outAddr[ARTNET_MAX_PORTS],
                        uint8_t netAddr,
                        uint8_t subAddr, artnet_port_command_t cmd,
                        uint8_t acnPriority) {
  node n = (node) vn;
  artnet_packet_t p = {0};
  node_entry_private_t *ent = find_private_entry(n,e);

  check_nullnode(vn);

  if (e == NULL || ent == NULL) {
    return ARTNET_EARG;
  }

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    p.to.s_addr = ent->ip.s_addr;

    p.length = sizeof(artnet_address_t);
    p.type = ARTNET_ADDRESS;

    // now build packet, copy the number of ports from the reply recieved from this node
    memcpy( &p.data.addr.id, ARTNET_STRING, ARTNET_STRING_SIZE);
    p.data.addr.opCode = htols(ARTNET_ADDRESS);
    p.data.addr.verH = 0;
    p.data.addr.ver = ARTNET_VERSION;
    p.data.addr.netSwitch = (netAddr != PROGRAM_NO_CHANGE) ? (netAddr | PROGRAM_CHANGE_MASK) : netAddr;
    p.data.addr.bindIndex = 1;

    memset(p.data.addr.shortName, 0, ARTNET_SHORT_NAME_LENGTH);
    size_t len = strnlen(shortName, ARTNET_SHORT_NAME_LENGTH);
    memcpy(p.data.addr.shortName, shortName, len);

    memset(p.data.addr.longName, 0, ARTNET_LONG_NAME_LENGTH);
    len = strnlen(longName, ARTNET_LONG_NAME_LENGTH);
    memcpy(p.data.addr.longName, longName, len);

    memcpy(&p.data.addr.swIn, inAddr, ARTNET_MAX_PORTS);
    memcpy(&p.data.addr.swOut, outAddr, ARTNET_MAX_PORTS);

    p.data.addr.subSwitch = (subAddr != PROGRAM_NO_CHANGE) ? ((subAddr & 0x0F) | PROGRAM_CHANGE_MASK) : subAddr;
    p.data.addr.acnPriority = acnPriority;
    p.data.addr.command = cmd;

    return artnet_net_send(n, &p);
  }
  return ARTNET_ESTATE;
}


/**
 * Sends an ArtInput packet to the specified node, this packet is used to
 * enable/disable the input ports on the remote node.
 *
 * 0x01 disable port
 * 0x00 enable port
 *
 * NOTE: should have enums here instead of uint8_t for settings
 *
 */
int artnet_send_input(artnet_node vn,
                      artnet_node_entry e,
                      uint8_t settings[ARTNET_MAX_PORTS]) {
  node n = (node) vn;
  artnet_packet_t p = {0};
  node_entry_private_t *ent = find_private_entry(n,e);

  check_nullnode(vn);

  if (e == NULL) {
    return ARTNET_EARG;
  }

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    // set dst, type and length
    p.to.s_addr = ent->ip.s_addr;

    p.length = sizeof(artnet_input_t);
    p.type = ARTNET_INPUT;

    // now build packet, copy the number of ports from the reply recieved from this node
    memcpy( &p.data.ainput.id, ARTNET_STRING, ARTNET_STRING_SIZE);
    p.data.ainput.opCode = htols(ARTNET_INPUT);
    p.data.ainput.verH = 0;
    p.data.ainput.ver = ARTNET_VERSION;
    p.data.ainput.filler1 = 0;
    p.data.ainput.bindIndex = 1;
    p.data.ainput.numbportsH = short_get_high_byte(e->numbports);
    p.data.ainput.numbports = short_get_low_byte(e->numbports);
    memcpy(&p.data.ainput.input, settings, ARTNET_MAX_PORTS);

    return artnet_net_send(n, &p);
  }
  return ARTNET_ESTATE;
}


/**
 *
 * Sends a series of ArtFirmwareMaster packets to the specified node, these are used to
 * upload firmware to the remote node.
 *
 * We send the first packet now, and another one every time we get a ArtFirmwareReply from
 * the node
 *
 * @param vn the artnet_node
 * @param e the node entry to send firmware to
 * @param ubea set to a true value if this is a ubea upload
 * @param data pointer to the firmware
 * @param length the number of 16bit words of the firmware (length * 2 = size in bytes)
 * @param fh the callback that is invoked when the transfer is complete
 * @param user_data data to be passed to the callback
 */
int artnet_send_firmware(
    artnet_node vn,
    artnet_node_entry e,
    int ubea,
    uint16_t *data,
    int length,
    int (*fh)(artnet_node n, artnet_firmware_status_code code, void *d),
    void *user_data) {
  node n = (node) vn;
  node_entry_private_t *ent = find_private_entry(n,e);
  int blen = 0;

  check_nullnode(vn);
  if (e == NULL || ent == NULL) {
    return ARTNET_EARG;
  }

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {

    // length in bytes
    blen = length * sizeof(uint16_t);

    // store the parameters for this transfer
    ent->firmware.data = malloc(blen);

    if ( ent->firmware.data == NULL) {
      artnet_error_malloc();
      return ARTNET_EMEM;
    }

    ent->firmware.bytes_current = 0;
    ent->firmware.bytes_total = blen;
    ent->firmware.peer = ent->ip;
    ent->firmware.ubea = ubea;
    // entry->firmware.last_time set upon sending a packet
    // id of the current block
    ent->firmware.expected_block = 0;
    ent->firmware.callback = fh;
    ent->firmware.user_data = user_data;

    memcpy(ent->firmware.data, data, blen);

    return artnet_tx_firmware_packet(n, &ent->firmware);
  }

  return ARTNET_ESTATE;
}


/**
 *
 * Sends an ArtFirmwareReply packet to the specified node,
 * this packet is used to acknowledge firmware master packets.
 *
 * Note, you should never call this function directly, it is provided for
 * completness and will only work if the node type is ARTNET_RAW
 *
 * @param vn the artnet_node
 * @param e the node entry to send firmware to
 * @param code the status code to send
 */
int artnet_send_firmware_reply(artnet_node vn,
                               artnet_node_entry e,
                               artnet_firmware_status_code code) {
  node n = (node) vn;
  node_entry_private_t *ent = find_private_entry(n,e);

  check_nullnode(vn);

  if (e == NULL || ent == NULL) {
    return ARTNET_EARG;
  }

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  return artnet_tx_firmware_reply(n, ent->ip.s_addr, code);
}


/**
 * Sends a tod request
 *
 * @param vn the artnet node
 *
 */
int artnet_send_tod_request(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_tod_request(n);
}


/**
 * Sends a tod control datagram
 *
 * @param vn the artnet node
 * @param address the universe address to control
 * @param action the action to take
 *
 */
int artnet_send_tod_control(artnet_node vn,
                            uint16_t address,
                            artnet_tod_command_code action) {
  node n = (node) vn;
  check_nullnode(vn);
  return artnet_tx_tod_control(n, address, action);
}


/**
 * Send a tod data datagram
 * Note you should not use this, a TodData message should only be sent in response
 * to a request, or when a RDM device is added. Use artnet_add_rdm_device() or artnet_add_rdm_devices() instead
 *
 * @param
 */
int artnet_send_tod_data(artnet_node vn, int port) {
  node n = (node) vn;
  check_nullnode(vn);

  // should update the check for enabled port here
  if (port < 0 || port >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port);
    return ARTNET_EARG;
  }

  return artnet_tx_tod_data(n, port);
}


/**
 * Send an ArtSync packet
 */
int artnet_send_sync(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_sync(n);
}


/**
 * Send an ArtTimeCode packet
 */
int artnet_send_timecode(artnet_node vn, uint8_t frames, uint8_t seconds,
                         uint8_t minutes, uint8_t hours,
                         artnet_timecode_type_t type, uint8_t stream_id) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_timecode(n, frames, seconds, minutes, hours, type, stream_id);
}


/**
 * Send an ArtTimeSync packet
 */
int artnet_send_timesync(artnet_node vn, uint8_t tm_sec, uint8_t tm_min,
                         uint8_t tm_hour, uint8_t tm_mday,
                         uint8_t tm_mon, uint8_t tm_year) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_timesync(n, tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year);
}


/**
 * Send an ArtTrigger packet
 */
int artnet_send_trigger(artnet_node vn, uint8_t oem_hi, uint8_t oem_lo,
                        uint8_t key, uint8_t sub_key,
                        const uint8_t *data, int16_t length) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_trigger(n, oem_hi, oem_lo, key, sub_key, data, length);
}


/**
 * Send an ArtDataReply packet (Art-Net 4)
 */
int artnet_send_data_reply(artnet_node vn, const char *ip,
                           uint16_t request_code, const char *payload,
                           int16_t length) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_data_reply(n, ip, request_code, payload, length);
}


/**
 * Send an ArtDirectory request (broadcast) to discover node file lists.
 */
int artnet_send_directory(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_directory(n);
}


/**
 * Send an ArtDirectoryReply with directory entries.
 */
int artnet_send_directory_reply(artnet_node vn,
                                const uint8_t *entries,
                                int entry_count,
                                int entry_total) {
  node n = (node) vn;
  artnet_packet_t p = {0};
  int len;

  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  len = entry_count > 2048 ? 2048 : entry_count;

  memset(&p, 0x00, sizeof(p));
  p.to = n->state.reply_addr;
  p.type = ARTNET_DIRECTORYREPLY;

  memcpy(&p.data.dirr.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.dirr.opCode = htols(ARTNET_DIRECTORYREPLY);
  p.data.dirr.verH = 0;
  p.data.dirr.ver = ARTNET_VERSION;

  p.data.dirr.dirCountHi = short_get_high_byte((uint16_t)len);
  p.data.dirr.dirCountLo = short_get_low_byte((uint16_t)len);
  p.data.dirr.dirTotalHi = short_get_high_byte((uint16_t)entry_total);
  p.data.dirr.dirTotalLo = short_get_low_byte((uint16_t)entry_total);

  if (entries && len > 0) {
    memcpy(&p.data.dirr.dirEntry, entries, len);
  }

  p.length = sizeof(artnet_directory_reply_t) - 2048 + len;

  return artnet_net_send(n, &p);
}


/**
 * Upload a file block to a node via ArtFileTnMaster.
 */
int artnet_send_file_tn_master(artnet_node vn,
                                artnet_node_entry e,
                                uint8_t type,
                                uint8_t blockId,
                                uint32_t totalLength,
                                const uint16_t *data,
                                int dataLen) {
  node n = (node) vn;
  node_entry_private_t *ent = find_private_entry(n, e);

  check_nullnode(vn);

  if (e == NULL || ent == NULL) {
    return ARTNET_EARG;
  }

  return artnet_tx_file_tn_master(n, ent->ip.s_addr, type, blockId,
                                   totalLength, data, dataLen);
}


/**
 * Request a file download from a node via ArtFileFnMaster.
 */
int artnet_send_file_fn_master(artnet_node vn,
                                artnet_node_entry e,
                                const char *filename) {
  node n = (node) vn;
  node_entry_private_t *ent = find_private_entry(n, e);

  check_nullnode(vn);

  if (e == NULL || ent == NULL) {
    return ARTNET_EARG;
  }

  return artnet_tx_file_fn_master(n, ent->ip.s_addr, filename);
}


/**
 * Send an ArtFileFnReply (file data block in response to ArtFileFnMaster).
 */
int artnet_send_file_fn_reply(artnet_node vn,
                               uint8_t blockId,
                               uint16_t totalLength,
                               uint8_t *data,
                               int dataLen) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_file_fn_reply(n, blockId, totalLength, data, dataLen);
}


/**
 * Send a rdm datagram
 * @param address the universe address to send to
 * @param data the rdm data to send
 * @param length the length of the rdm data
 */
int artnet_send_rdm(artnet_node vn,
                    uint16_t address,
                    uint8_t *data,
                    int length) {
  node n = (node) vn;
  check_nullnode(vn);

  //we check that we are using this address
  return artnet_tx_rdm(n, address, data, length);
}


/**
 * Send a RDM sub packet
 */
int artnet_send_rdmsub(artnet_node vn,
                       uint8_t uid[ARTNET_RDM_UID_WIDTH],
                       uint8_t command_class,
                       uint16_t param_id,
                       uint16_t sub_device,
                       uint16_t sub_count,
                       uint8_t *data,
                       int length) {
  node n = (node) vn;
  check_nullnode(vn);

  return artnet_tx_rdmsub(n, uid, command_class, param_id,
                           sub_device, sub_count, data, length);
}


/**
 * Add a rdm device to our tod.
 * @param port the port the device is connected to
 * @param the uid of the device
 */
int artnet_add_rdm_device(artnet_node vn,
                          int port,
                          uint8_t uid[ARTNET_RDM_UID_WIDTH]) {
  node n = (node) vn;
  check_nullnode(vn);

  if (port < 0 || port >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port);
    return ARTNET_EARG;
  }

  // add uid to tod for this port
  add_tod_uid(&n->ports.out[port].port_tod, uid);

  // notify everyone our tod changed
  return artnet_tx_tod_data(n, port);
}


/**
 * add a list of rdm devices to our tod, use this for full discovery
 *
 * @param port the port the device is connected to
 * @param uid pointer to the uids
 * @param count number of uids
 */
int artnet_add_rdm_devices(artnet_node vn, int port, uint8_t *uid, int count) {
  node n = (node) vn;
  int i = 0;

  check_nullnode(vn);

  if (port < 0 || port >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port);
    return ARTNET_EARG;
  }

  if (count < 0) {
    return ARTNET_EARG;
  }

  for (i = 0; i < count; i++) {
    // add uid to tod for this port
    add_tod_uid(&n->ports.out[port].port_tod, uid);
    uid += ARTNET_RDM_UID_WIDTH;
  }
  // notify everyone  our tod changed
  return artnet_tx_tod_data(n, port);

}


/**
 * Remove an rdm device from our tod.
 *
 * @param port the port the device was connected to
 * @param uid  the uid of the device
 */
int artnet_remove_rdm_device(artnet_node vn,
                             int port,
                             uint8_t uid[ARTNET_RDM_UID_WIDTH]) {
  node n = (node) vn;
  check_nullnode(vn);

  if (port < 0 || port >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port);
    return ARTNET_EARG;
  }

  // remove uid to tod for this port
  remove_tod_uid(&n->ports.out[port].port_tod, uid);

  // notify everyone our tod changed
  return artnet_tx_tod_data(n, port);
}


/**
 * Reads the latest dmx data
 * @param vn the artnet node
 * @param port_id the port to read data from
 * @param length
 * @return a pointer to the dmx data, NULL on error
 */

uint8_t *artnet_read_dmx(artnet_node vn, int port_id, int *length) {
  node n = (node) vn;

  if (n == NULL) {
    return NULL;
  }

  if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port_id);
    return NULL;
  }

  *length = n->ports.out[port_id].length;
  return &n->ports.out[port_id].data[0];
}


//--------------------------------------
// Functions to change the node state (setters)

/**
 * Set the node type (server, node, mserver, raw).
 *
 * @param vn   the artnet_node
 * @param type the node type
 */
int artnet_set_node_type(artnet_node vn, artnet_node_type type) {
  node n = (node) vn;
  check_nullnode(vn);

  n->state.node_type = type;
  return ARTNET_EOK;
}


/**
 * Set the style code for the node.
 * The style code describes the product type and is sent in ArtPollReply Status1 bits 3-0.
 * Can only be set in STANDBY mode.
 *
 * @param vn the artnet_node
 * @param style the style code (artnet_style_code_t)
 */
int artnet_set_style_code(artnet_node vn, artnet_style_code_t style) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_STANDBY) {
    return ARTNET_ESTATE;
  }

  n->state.style_code = (uint8_t) style;
  return ARTNET_EOK;
}
/**
 * Set the Status2 field for the node (ArtPollReply Status2).
 *
 * @param vn      the artnet_node
 * @param status2 the Status2 byte value
 */
int artnet_set_status2(artnet_node vn, uint8_t status2) {
  node n = (node) vn;
  check_nullnode(vn);

  n->state.status2 = status2;
  return ARTNET_EOK;
}


/**
 * Sets the artnet subnet address for this node.
 * The subnet address has nothing to do with IP addresses). An ArtNet subnet is a grouping of 16 DMX universes
 * (ie. ports)
 *
 * The subnet address is between 0 and 15. If the supplied address is larger than 15, the
 * lower 4 bits will be used in setting the address.
 *
 * It will have no effect if the node is under network control.
 *
 * Note that changing the subnet address will cause the universe addresses of all ports to change.
 *
 * @param vn the artnet_node
 * @param subnet new subnet address
 */
/**
 * Sets the net address of the node.
 * The net address is bits 14-8 of the 15-bit Art-Net port address (0-127).
 *
 * @param vn the artnet_node
 * @param net the net address (0-127)
 */
int artnet_set_net_addr(artnet_node vn, uint8_t net) {
  node n = (node) vn;

  check_nullnode(vn);

  if (net > 0x7F) {
    artnet_error("Net address out of range, max is 127");
    n->state.report_code = ARTNET_RC_USER_FAIL;
    return ARTNET_EARG;
  }

  n->state.default_netSwitch = net;

  if (!n->state.netSwitch_net_ctl && net != n->state.netSwitch) {
    n->state.netSwitch = net;

    // redo the addresses for each port
    for (int i = 0; i < ARTNET_MAX_PORTS; i++) {
      n->ports.in[i].port_addr = make_addr(net, n->state.subSwitch, addr_port(n->ports.in[i].port_addr));
      n->ports.out[i].port_addr = make_addr(net, n->state.subSwitch, addr_port(n->ports.out[i].port_addr));
    }

    if (n->state.mode == ARTNET_ON) {
      int ret = artnet_tx_build_art_poll_reply(n);
      if (ret) {
        return ret;
      }
      return artnet_tx_poll_reply(n, FALSE);
    }
  } else if (n->state.netSwitch_net_ctl) {
    n->state.report_code = ARTNET_RC_USER_FAIL;
  }

  return ARTNET_EOK;
}
/**
 * Sets the artnet subnet address for this node.
 * The subnet address has nothing to do with IP addresses. An ArtNet subnet is a grouping of 16 DMX universes
 * (ie. ports).
 *
 * The subnet address is between 0 and 15. If the supplied address is larger than 15, the
 * lower 4 bits will be used in setting the address.
 *
 * It will have no effect if the node is under network control.
 *
 * Note that changing the subnet address will cause the universe addresses of all ports to change.
 *
 * @param vn     the artnet_node
 * @param subnet new subnet address
 */
int artnet_set_subnet_addr(artnet_node vn, uint8_t subnet) {
  node n = (node) vn;
  int i = 0, ret = 0;

  check_nullnode(vn);

  n->state.default_subSwitch = subnet;

  // if not under network control, and the subnet is different from the current one
  if (!n->state.subSwitch_net_ctl && subnet != n->state.subSwitch) {
    n->state.subSwitch = subnet;

    // redo the addresses for each port
    for (i =0; i < ARTNET_MAX_PORTS; i++) {
      n->ports.in[i].port_addr = make_addr(n->state.netSwitch, subnet, addr_port(n->ports.in[i].port_addr));
      // reset dmx sequence number
      n->ports.in[i].seq = 0;

      n->ports.out[i].port_addr = make_addr(n->state.netSwitch, subnet, addr_port(n->ports.out[i].port_addr));
    }

    if (n->state.mode == ARTNET_ON) {

      if ((ret = artnet_tx_build_art_poll_reply(n)) != 0) {
        return ret;
      }

      return artnet_tx_poll_reply(n,FALSE);
    }
  } else if (n->state.subSwitch_net_ctl ) {
    //  trying to change subnet addr while under network control
    n->state.report_code = ARTNET_RC_USER_FAIL;
  }

  return ARTNET_EOK;
}


/**
 * Sets the RDMnet & LLRP default responder UID (Art-Net 4).
 *
 * @param n the artnet_node
 * @param uid pointer to 6-byte UID array
 */
int artnet_set_default_resp_uid(artnet_node vn, const uint8_t uid[ARTNET_RDM_UID_WIDTH]) {
  node n = (node) vn;
  int ret = 0;

  check_nullnode(vn);

  memcpy(n->state.default_resp_uid, uid, ARTNET_RDM_UID_WIDTH);

  if (n->state.mode == ARTNET_ON) {
    if ((ret = artnet_tx_build_art_poll_reply(n)) != 0) {
      return ret;
    }
  }

  return ARTNET_EOK;
}
/**
 * Set the gateway IP address for the node.
 *
 * @param vn the artnet_node
 * @param ip the gateway IP address as a dotted string
 */
int artnet_set_gateway(artnet_node vn, const char *ip) {
  node n = (node) vn;

  check_nullnode(vn);

  n->state.gateway.s_addr = inet_addr(ip);
  if (n->state.gateway.s_addr == INADDR_NONE) {
    artnet_error("Invalid gateway address %s", ip);
    return ARTNET_EARG;
  }

  return ARTNET_EOK;
}


/**
 * Sets the short name of the node.
 * The string should be null terminated and a maxmium of 18 Characters will be used
 *
 * @param vn the artnet_node
 * @param name the short name of the node.
 */
int artnet_set_short_name(artnet_node vn, const char *name) {
  node n = (node) vn;
  check_nullnode(vn);

  strncpy((char *) &n->state.shortName, name, ARTNET_SHORT_NAME_LENGTH);
  n->state.shortName[ARTNET_SHORT_NAME_LENGTH-1] = 0x00;
  return artnet_tx_build_art_poll_reply(n);
}


/**
 * Sets the long name of the node.
 * The string should be null terminated and a maximium of 64 characters will be used
 *
 * @param vn the artnet_node
 * @param name the node's long name
 */
int artnet_set_long_name(artnet_node vn, const char *name) {
  node n = (node) vn;
  check_nullnode(vn);

  strncpy((char *) &n->state.longName, name, ARTNET_LONG_NAME_LENGTH);
  n->state.longName[ARTNET_LONG_NAME_LENGTH-1] = 0x00;
  return artnet_tx_build_art_poll_reply(n);
}


/**
 * Sets the direction and type of port
 * @param vn the artnet_node
 * @param id
 * @param direction
 * @param data
 */
int artnet_set_port_type(artnet_node vn,
                         int port_id,
                         artnet_port_settings_t settings,
                         artnet_port_data_code data) {
  node n = (node) vn;
  check_nullnode(vn);

  if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port_id);
    return ARTNET_EARG;
  }

  n->ports.types[port_id] = (uint8_t)(settings | data);
  return ARTNET_EOK;
}


/**
 * Sets the port address of the port.
 *
 * The full 15-bit port address is composed of three parts:
 *  - net address, 7 bits, set on a per-node basis
 *  - subnet address, 4 bits, set on a per-node basis
 *  - port address, 4 bits, set on a per-port basis
 *
 * The resulting port address is: (net << 8) | (subnet << 4) | port
 *
 * As the port address is between 0 and 15, only the lower 4 bits of the addr argument
 * will be used.
 *
 * The operation may have no effect if the port is under network control.
 *
 * @param vn the artnet_node
 * @param id the phyiscal port number (from 0 to ARTNET_MAX_PORTS-1 )
 * @param dir either ARTNET_INPUT_PORT or ARTNET_OUTPUT_PORT
 * @param addr the new port address
 */
int artnet_set_port_addr(artnet_node vn,
                         int id,
                         artnet_port_dir_t dir,
                         uint8_t addr) {
  node n = (node) vn;
  int ret = 0;
  int changed = 0;

  g_port_t *port = NULL;

  check_nullnode(vn);

  if (id < 0 || id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, id);
    return ARTNET_EARG;
  }

  if (addr > 15) {
    artnet_error("%s : Attempt to set port %i to invalid address %#hhx\n", __FUNCTION__, id, addr);
    return ARTNET_EARG;
  }

  if (dir == ARTNET_INPUT_PORT) {
    port = &n->ports.in[id].port;
    changed = n->ports.in[id].port_enabled ? 0 : 1;
    n->ports.in[id].port_enabled = TRUE;
  } else if (dir == ARTNET_OUTPUT_PORT) {
    port = &n->ports.out[id].port;
    changed = n->ports.out[id].port_enabled ? 0 : 1;
    n->ports.out[id].port_enabled = TRUE;
  } else {
    artnet_error("%s : Invalid port direction\n", __FUNCTION__);
    return ARTNET_EARG;
  }

  port->default_addr = addr;

  // if not under network control and address is changing
  if (!port->net_ctl &&
      (changed || (addr & LOW_NIBBLE) != addr_port(port->addr))) {
    port->addr = make_addr(n->state.netSwitch, n->state.subSwitch, addr);

    // reset seq if input port
    if (dir == ARTNET_INPUT_PORT) {
      n->ports.in[id].seq = 0;
    }

    if (n->state.mode == ARTNET_ON) {
      if ((ret = artnet_tx_build_art_poll_reply(n)) != 0) {
        return ret;
      }

      return artnet_tx_poll_reply(n,FALSE);
    }
  } else if (port->net_ctl) {
    //  trying to change port addr while under network control
    n->state.report_code = ARTNET_RC_USER_FAIL;
  }
  return ARTNET_EOK;
}


/**
 * Returns the universe address of this port
 *
 * @param vn the artnet_node
 * @param id the phyiscal port number (from 0 to ARTNET_MAX_PORTS-1 )
 * @param dir either ARTNET_INPUT_PORT or ARTNET_OUTPUT_PO
 *
 * @return the universe address, or < 0 on error
 */
int artnet_get_universe_addr(artnet_node vn, int id, artnet_port_dir_t dir) {
  node n = (node) vn;
  check_nullnode(vn);

  if (id < 0 || id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, id);
    return ARTNET_EARG;
  }

  if (dir == ARTNET_INPUT_PORT) {
    return n->ports.in[id].port.addr;
  } else if (dir == ARTNET_OUTPUT_PORT) {
    return n->ports.out[id].port.addr;
  } else {
    artnet_error("%s : Invalid port direction\n", __FUNCTION__);
    return ARTNET_EARG;
  }
}/**
 * Retrieve the current node configuration.
 *
 * @param vn     the artnet_node
 * @param config pointer to an artnet_node_config_t to populate
 * @return ARTNET_EOK on success
 */
int artnet_get_config(artnet_node vn, artnet_node_config_t *config) {
  int i = 0;
  node n = (node) vn;
  check_nullnode(vn);

  strncpy(config->shortName, n->state.shortName, ARTNET_SHORT_NAME_LENGTH);
  strncpy(config->longName, n->state.longName, ARTNET_LONG_NAME_LENGTH);
  config->netSwitch = n->state.netSwitch;
  config->subSwitch = n->state.subSwitch;

  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    config->inPorts[i] = n->ports.in[i].port.addr & LOW_NIBBLE;
    config->outPorts[i] = n->ports.out[i].port.addr & LOW_NIBBLE;
  }

  return ARTNET_EOK;
}


/**
 * Dumps the node config to stdout.
 *
 * @param vn the artnet_node
 */
int artnet_dump_config(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  printf("#### NODE CONFIG ####\n");
  printf("Node Type: %i\n", n->state.node_type);
  printf("Short Name: %s\n", n->state.shortName);
  printf("Long Name: %s\n", n->state.longName);
  printf("Net: %#02x\n", n->state.netSwitch);
  printf("Subnet: %#02x\n", n->state.subSwitch);
  printf("Default Subnet: %#02x\n", n->state.default_subSwitch);
  printf("Net Ctl: %i\n", n->state.subSwitch_net_ctl);
  printf("#####################\n");

  return ARTNET_EOK;
}


/**
 * Returns the socket descriptor associated with this artnet_node.
 * Peered (joined) nodes share the same socket descriptor.
 *
 * @param vn the artnet_node
 * @return the socket descriptor, or a negative value on error
 */
artnet_socket_t artnet_get_sd(artnet_node vn) {
  node n = (node) vn;
  if (vn == NULL) {
    artnet_error("%s : argument 1 (artnet_node) was null", __FUNCTION__);
    return (artnet_socket_t)ARTNET_EARG;
  }

  if (n->state.mode != ARTNET_ON) {
    return (artnet_socket_t)ARTNET_EACTION;
  }

  return n->sd;
}


/**
 * Sets the file descriptors in the fdset that we are interested in.
 *
 * @param vn    the artnet_node
 * @param fdset pointer to the fdset to change
 * @return ARTNET_EOK on success, or a negative error code on failure
 */
int artnet_set_fdset(artnet_node vn, fd_set *fdset) {
  node n = (node) vn;
  check_nullnode(vn);

  if (!fdset) {
    return ARTNET_EARG;
  }

  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  return artnet_net_set_fdset(n, fdset);
}


/**
 * Returns the artnet_node_list.
 * The artnet_node_list holds artnet_node_entry(s) that represent the discovered
 * remote nodes on the network
 * NOTE: this function is not THREAD SAFE
 *
 * @param vn the artnet_node
 * @return the artnet_node_list
 */
artnet_node_list artnet_get_nl(artnet_node vn) {
  node n = (node) vn;

  if (!vn) {
    return NULL;
  }

  return &n->node_list;
}


/**
 * Repositions the pointer to the first entry in the artnet_node_list
 * NOTE: this function is not THREAD SAFE
 *
 * @param vnl the artnet_node_list
 * @return the first artnet_node_entry in the list, or NULL if the list is empty
 */
artnet_node_entry artnet_nl_first(artnet_node_list vnl) {
  node_list_t *nl = (node_list_t*) vnl;

  if (!nl) {
    return NULL;
  }

  nl->current = nl->first;
  return &nl->current->pub;
}


/**
 * Moves the pointer to the next element in the artnet_node_list
 * NOTE: this function is not THREAD SAFE
 *
 * @param vnl the artnet_node_list
 * @return the next artnet_node_entry, or NULL if the end of the list is reached
 */
artnet_node_entry artnet_nl_next(artnet_node_list vnl) {
  node_list_t *nl = (node_list_t*) vnl;

  if (!nl) {
    return NULL;
  }

  nl->current = nl->current->next;
  return &nl->current->pub;
}


/**
 * Returns the length of the artnet_node_list
 * NOTE: this function is not THREAD SAFE
 *
 * @param vnl the artnet_node_list
 * @return the length of the list
 */
int artnet_nl_get_length(artnet_node_list vnl) {
  node_list_t *nl = (node_list_t*) vnl;

  if (!nl) {
    return 0;
  }

  return nl->length;
}


/**
 * Thread-safe node list iteration.
 * The entire traversal is protected by seqlock.
 * The callback receives a stack copy of the entry, safe to use after the call.
 *
 * @param vn the artnet_node
 * @param cb callback function, return non-zero to stop iteration
 * @param data user data passed to callback
 * @return 0 on success, or the non-zero return value from callback
 */
int artnet_nl_foreach(artnet_node vn,
                      int (*cb)(artnet_node_entry entry, void *data),
                      void *data) {
  node n = (node) vn;
  node_list_t *nl = NULL;
  node_entry_private_t *tmp = NULL;

  if (!vn) {
    return ARTNET_EARG;
  }

  nl = &n->node_list;

  uint32_t seq = 0;
  do {
    seq = NL_READ_BEGIN(n);
    if (seq & 1) { continue; }
    for (tmp = nl->first; tmp; tmp = tmp->next) {
      artnet_node_entry_t copy = tmp->pub;
      int ret = cb(&copy, data);
      if (ret) { return ret; }
    }
  } while (NL_READ_RETRY(n, seq));

  return ARTNET_EOK;
}


/**
 * Return a pointer to the staticly allocated error string
 */
char *artnet_strerror() {
  return artnet_errstr;
}


//-----------------------------------------------------------------------------
// Private functions follow
//-----------------------------------------------------------------------------

int artnet_nl_update(node n, node_list_t *nl, artnet_packet reply) {
  node_entry_private_t *entry = NULL;
  uint8_t swOut0 = reply->data.ar.swOut[0];

  NL_WRITE_BEGIN(n);

  // find entry matching both IP and first port address (joined nodes share IP)
  entry = find_entry_from_ip(nl, reply->from);
  if (entry && entry->pub.swOut[0] != swOut0) {
    // same IP but different port set, look for matching port
    node_entry_private_t *tmp = NULL;
    for (tmp = nl->first; tmp; tmp = tmp->next) {
      if (tmp->ip.s_addr == reply->from.s_addr && tmp->pub.swOut[0] == swOut0) {
        entry = tmp;
        break;
      }
      entry = NULL;
    }
  }

  if (!entry) {
    // add to list
    entry = (node_entry_private_t*) malloc(sizeof(node_entry_private_t));

    if (!entry) {
      artnet_error_malloc();
      return ARTNET_EMEM;
    }

    memset(entry, 0x00, sizeof(node_entry_private_t));

    copy_apr_to_node_entry(&entry->pub, &reply->data.ar);
    entry->ip = reply->from;
    entry->last_seen = time(NULL);
    entry->next = NULL;

    if (!nl->first) {
      nl->first = entry;
      nl->last = entry;
    } else {
      nl->last->next = entry;
      nl->last = entry;
    }
    nl->length++;
  } else {
    // update entry
    copy_apr_to_node_entry(&entry->pub, &reply->data.ar);
    entry->last_seen = time(NULL);
  }
  NL_WRITE_END(n);
  return ARTNET_EOK;
}


/**
 * check if this packet is in list
 */
node_entry_private_t *find_entry_from_ip(node_list_t *nl, SI ip) {
  node_entry_private_t *tmp = NULL;

  for (tmp = nl->first; tmp; tmp = tmp->next) {
    if (ip.s_addr == tmp->ip.s_addr) {
      break;
    }
  }
  return tmp;
}


/**
 * Find all nodes with a port bound to a particular universe
 * @param nl the node list
 * @param uni the universe to search for
 * @param ips store matching node ips here
 * @param size size of ips
 * @return number of nodes matched
 */
int find_nodes_from_uni(node n, node_list_t *nl, uint16_t uni, SI *ips, int size) {
  node_entry_private_t *tmp = NULL;
  int count = 0;
  int i = 0, j = 0;
  uint32_t seq = 0;

  do {
    seq = NL_READ_BEGIN(n);
    if (seq & 1) { continue; }  // writer active, retry
    count = 0;
    j = 0;
    for (tmp = nl->first; tmp; tmp = tmp->next) {
      int added = FALSE;
      for (i = 0; i < tmp->pub.numbports; i++) {
        uint16_t entry_uni = make_addr(tmp->pub.netSwitch, tmp->pub.subSwitch & 0x0F, tmp->pub.swOut[i] & 0x0F);
        if (entry_uni == uni && ips) {
          if (j < size && !added) {
            ips[j++] = tmp->ip;
            added = TRUE;
          }
          count++;
        }
      }
    }
  } while (NL_READ_RETRY(n, seq));

  return count;
}


/**
 * Add a node to the node list from an ArtPollReply msg
 */
void copy_apr_to_node_entry(artnet_node_entry e, artnet_reply_t *reply) {

  // the ip is network byte ordered
  memcpy(&e->ip, &reply->ip, 4);
  e->ver = bytes_to_short(reply->verH, reply->ver);
  e->netSwitch = reply->netSwitch;
  e->subSwitch = reply->subSwitch;
  e->oem = bytes_to_short(reply->oemH, reply->oem);
  e->ubea = reply->ubea;
  e->status = reply->status;
  memcpy(&e->estaMan, &reply->estaMan, 2);
  memcpy(&e->shortName, &reply->shortName,  sizeof(e->shortName));
  memcpy(&e->longName, &reply->longName, sizeof(e->longName));
  memcpy(&e->nodeReport, &reply->nodeReport, sizeof(e->nodeReport));
  e->numbports = bytes_to_short(reply->numbportsH, reply->numbports);
  memcpy(&e->portTypes, &reply->portTypes, ARTNET_MAX_PORTS);
  memcpy(&e->goodInput, &reply->goodInput, ARTNET_MAX_PORTS);
  memcpy(&e->goodOutputA, &reply->goodOutputA, ARTNET_MAX_PORTS);
  memcpy(&e->swIn, &reply->swIn, ARTNET_MAX_PORTS);
  memcpy(&e->swOut, &reply->swOut, ARTNET_MAX_PORTS);
  e->acnPriority = reply->acnPriority;
  e->swMacro = reply->swMacro;
  e->swRemote = reply->swRemote;
  e->style = reply->style;
  memcpy(&e->mac, &reply->mac, ARTNET_MAC_SIZE);
  memcpy(&e->bindIp, &reply->bindIp, ARTNET_IP_SIZE);
  e->bindIndex = reply->bindIndex;
  e->status2 = reply->status2;
  memcpy(&e->goodOutputB, &reply->goodOutputB, ARTNET_MAX_PORTS);
  e->status3 = reply->status3;
  memcpy(&e->defaultRespUid, &reply->defaultRespUid, ARTNET_RDM_UID_WIDTH);
  e->userHi = reply->userHi;
  e->userLo = reply->userLo;
  e->refreshRate = bytes_to_short(reply->refreshRateHi, reply->refreshRateLo);
  e->bgQueuePolicy = reply->bgQueuePolicy;
}

/**
 * find a node_entry in the node list
 */
node_entry_private_t *find_private_entry(node n, artnet_node_entry e) {
  node_entry_private_t *tmp = NULL;
  if (!e) {
    return NULL;
  }

  // check if this packet is in list
  for (tmp = n->node_list.first; tmp; tmp = tmp->next) {
    if (!memcmp(&e->ip, &tmp->pub.ip, 4)) {
      break;
    }
  }
  return tmp;
}


void check_timeouts(node n) {
  clock_t now = clock();
  time_t now_time = time(NULL);
  int i = 0;

  // fail-safe: check for DMX data loss on enabled output ports
  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    output_port_t *port = &n->ports.out[i];

    if (!port->port_enabled || port->last_dmx_time == 0) {
      continue;
    }

    if (!port->failsafe_triggered &&
        ((int)((now - port->last_dmx_time) * 1000 / CLOCKS_PER_SEC) >= DMX_FAILSAFE_TIMEOUT_MS)) {

      switch (n->state.failsafe_mode) {
        case ARTNET_FAILSAFE_ZERO:
          memset(port->data, 0x00, ARTNET_DMX_LENGTH);
          port->length = ARTNET_DMX_LENGTH;
          break;
        case ARTNET_FAILSAFE_FULL:
          memset(port->data, 0xFF, ARTNET_DMX_LENGTH);
          port->length = ARTNET_DMX_LENGTH;
          break;
        case ARTNET_FAILSAFE_SCENE:
          if (port->failsafe_length > 0) {
            memcpy(port->data, port->failsafe_data, ARTNET_DMX_LENGTH);
            port->length = port->failsafe_length;
          }
          // fall through
        case ARTNET_FAILSAFE_HOLD:
        default:
          // HOLD: keep last data, no action needed
          break;
      }

      port->failsafe_triggered = TRUE;
    }
  }

  // ArtSync timeout: revert to non-sync mode after ARTSYNC_TIMEOUT_MS without ArtSync
  if (n->state.sync_mode && (now_time - n->state.last_sync_time >= ARTSYNC_TIMEOUT_MS / 1000)) {
    n->state.sync_mode = 0;
  }

  // DMX input keepalive: retransmit last DMX data every DMX_KEEPALIVE_INTERVAL_MS
  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    input_port_t *in_port = &n->ports.in[i];
    if (!in_port->port_enabled || in_port->last_dmx_send_time == 0) {
      continue;
    }
    if (now_time - in_port->last_dmx_send_time >= DMX_KEEPALIVE_INTERVAL_MS / 1000) {
      if (in_port->last_dmx_length > 0) {
        artnet_send_dmx((artnet_node)n, i, (int16_t)in_port->last_dmx_length, in_port->last_dmx_data);
      }
    }
  }

  // node list cleanup: remove nodes that haven't responded in NODELIST_TIMEOUT_MS
  NL_WRITE_BEGIN(n);
  {
    node_entry_private_t **pp = &n->node_list.first;
    while (*pp) {
      if (time(NULL) - (*pp)->last_seen > NODELIST_TIMEOUT_MS / 1000) {
        node_entry_private_t *dead = *pp;
        *pp = dead->next;
        if (dead->firmware.data != NULL) {
          free(dead->firmware.data);
          dead->firmware.data = NULL;
        }
        free(dead);
        dead = NULL;
        n->node_list.length--;
      } else {
        pp = &(*pp)->next;
      }
    }
    // update last pointer
    n->node_list.last = NULL;
    for (node_entry_private_t *tmp = n->node_list.first; tmp; tmp = tmp->next) {
      n->node_list.last = tmp;
    }
    n->node_list.current = n->node_list.first;
  }
  NL_WRITE_END(n);

  if (n->firmware.peer.s_addr != 0
      && (now_time - n->firmware.last_time >= FIRMWARE_TIMEOUT_MS / 1000)) {

    printf("firmware timeout\n");
    reset_firmware_upload(n);

    n->state.report_code = ARTNET_RC_FIRMWARE_FAIL;
    // spec says to set ArtPollReply->Status here, but don't know to what value
  }

  // ArtPollReply random delay: send pending reply after scheduled time
  if (n->state.apr_pending && now_time >= n->state.apr_pending_time) {
    n->state.apr_pending = FALSE;
    artnet_tx_poll_reply(n, TRUE);
  }
}
