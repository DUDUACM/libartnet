#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"

static int g_failures = 0;

#define ASSERT_TRUE(cond, msg) \
  do { \
    if (!(cond)) { \
      fprintf(stderr, "FAIL: %s\n", (msg)); \
      g_failures++; \
    } \
  } while (0)

typedef struct {
  int called;
  artnet_packet_type_t type;
  struct in_addr to;
  artnet_packet_union_t data;
} send_capture_t;

typedef struct {
  int called;
  artnet_packet_type_t types[8];
  struct in_addr to[8];
  artnet_packet_union_t data[8];
} send_list_capture_t;

typedef struct {
  int called;
} simple_capture_t;

typedef struct {
  int called;
  int port;
} rdm_init_capture_t;

typedef struct {
  int called;
  int address;
  int length;
  uint8_t data[ARTNET_MAX_RDM_DATA];
} rdm_capture_t;

typedef struct {
  int called;
  int ubea;
  int length;
  uint16_t data[ARTNET_FIRMWARE_SIZE * 2];
} firmware_capture_t;

typedef struct {
  int called;
  artnet_firmware_status_code code;
} firmware_status_capture_t;

int handle_poll(node n, artnet_packet p);
int handle_address(node n, artnet_packet p);
int _artnet_handle_input(node n, artnet_packet p);
void handle_sync(node n, artnet_packet p);
void handle_dmx(node n, artnet_packet p);
void handle_rdm(node n, artnet_packet p);
void handle_rdm_sub(node n, artnet_packet p);
void check_merge_timeouts(node n, int port_id);
int handle_firmware(node n, artnet_packet p);
int handle_firmware_reply(node n, artnet_packet p);
int handle_file_tn_master(node n, artnet_packet p);
int handle_tod_request(node n, artnet_packet p);
int handle_tod_control(node n, artnet_packet p);
static struct in_addr ip4(const char *text);

static node_entry_private_t *add_stub_node_entry(node n,
                                                 const char *ip,
                                                 uint8_t net,
                                                 uint8_t subnet,
                                                 uint8_t port) {
  node_entry_private_t *entry = (node_entry_private_t *)calloc(1, sizeof(*entry));
  if (!entry) {
    fprintf(stderr, "Failed to allocate node entry\n");
    exit(2);
  }

  entry->ip = ip4(ip);
  entry->pub.numbports = 1;
  entry->pub.netSwitch = net;
  entry->pub.subSwitch = subnet;
  entry->pub.portTypes[0] = (uint8_t)((uint8_t)ARTNET_ENABLE_OUTPUT | (uint8_t)ARTNET_PORT_DMX);
  entry->pub.swOut[0] = port;
  entry->last_seen = artnet_gettime_ms();
  entry->next = NULL;

  if (!n->node_list.first) {
    n->node_list.first = entry;
  } else {
    n->node_list.last->next = entry;
  }
  n->node_list.last = entry;
  n->node_list.length++;

  return entry;
}

static struct in_addr ip4(const char *text) {
  unsigned int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
  uint32_t host_value = 0;
  struct in_addr addr = {0};

  if (sscanf(text, "%u.%u.%u.%u", &b0, &b1, &b2, &b3) != 4 ||
      b0 > 255 || b1 > 255 || b2 > 255 || b3 > 255) {
    fprintf(stderr, "Invalid IPv4 literal: %s\n", text);
    exit(2);
  }

  host_value = ((uint32_t)b0 << 24) |
               ((uint32_t)b1 << 16) |
               ((uint32_t)b2 << 8) |
               (uint32_t)b3;
  addr.s_addr = htonl(host_value);
  return addr;
}

static void init_test_node(artnet_node_t *n) {
  int i = 0;

  memset(n, 0, sizeof(*n));
  n->state.mode = ARTNET_STANDBY;
  n->state.ip_addr = ip4("10.0.0.2");
  n->state.subnet_mask = ip4("255.255.255.0");
  n->state.bcast_addr = ip4("10.0.0.255");
  n->state.gateway = ip4("10.0.0.1");
  n->state.esta_hi = 0x12;
  n->state.esta_lo = 0x34;
  n->state.oem_hi = 0x56;
  n->state.oem_lo = 0x78;
  n->state.node_type = ARTNET_NODE;
  n->state.diag_priority = 0xFF;
  n->state.bind_index = 1;
  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    n->ports.in[i].seq = 1;
  }
}

static void start_sendable_node(artnet_node_t *n) {
  ASSERT_TRUE(artnet_net_start(n) == ARTNET_EOK,
              "artnet_net_start should succeed for send-capable tests");
  n->state.mode = ARTNET_ON;
}

static void stop_sendable_node(artnet_node_t *n) {
  node_entry_private_t *entry = n->node_list.first;
  while (entry) {
    node_entry_private_t *next = entry->next;
    if (entry->firmware.data) {
      free(entry->firmware.data);
    }
    free(entry);
    entry = next;
  }
  n->node_list.first = NULL;
  n->node_list.last = NULL;
  n->node_list.current = NULL;
  n->node_list.length = 0;

  artnet_net_close(n->sd);
  n->sd = INVALID_SOCKET;
  n->state.mode = ARTNET_STANDBY;
}

static void init_packet(artnet_packet_t *p, artnet_packet_type_t type, struct in_addr from) {
  static const uint8_t k_artnet_id[8] = {'A', 'r', 't', '-', 'N', 'e', 't', '\0'};
  memset(p, 0, sizeof(*p));
  memcpy(p->data.ap.id, k_artnet_id, sizeof(k_artnet_id));
  p->data.ap.opCode = htols(type);
  p->data.ap.verH = 0;
  p->data.ap.ver = ARTNET_VERSION;
  p->length = (int)sizeof(artnet_packet_t);
  p->from = from;
  p->type = type;
}

static int send_capture_handler(artnet_node vn, void *pp, void *data) {
  artnet_packet p = (artnet_packet)pp;
  send_capture_t *capture = (send_capture_t *)data;

  (void)vn;
  capture->called++;
  capture->type = p->type;
  capture->to = p->to;
  memcpy(&capture->data, &p->data, sizeof(capture->data));
  return 0;
}

static int send_list_capture_handler(artnet_node vn, void *pp, void *data) {
  artnet_packet p = (artnet_packet)pp;
  send_list_capture_t *capture = (send_list_capture_t *)data;

  (void)vn;
  if (capture->called < (int)(sizeof(capture->types) / sizeof(capture->types[0]))) {
    capture->types[capture->called] = p->type;
    capture->to[capture->called] = p->to;
    memcpy(&capture->data[capture->called], &p->data, sizeof(capture->data[0]));
  }
  capture->called++;
  return 0;
}

static void set_vlc_magic(uint8_t *data, int payload_length) {
  memset(data, 0, ARTNET_VLC_MIN_LENGTH + payload_length);
  data[0] = 0x41;
  data[1] = 0x4c;
  data[2] = 0x45;
  data[8] = short_get_high_byte(payload_length);
  data[9] = short_get_low_byte(payload_length);
}

static int simple_packet_handler(artnet_node vn, void *pp, void *data) {
  simple_capture_t *capture = (simple_capture_t *)data;
  (void)vn;
  (void)pp;
  capture->called++;
  return 0;
}

static int rdm_init_handler(artnet_node vn, int port, void *data) {
  rdm_init_capture_t *capture = (rdm_init_capture_t *)data;
  (void)vn;
  capture->called++;
  capture->port = port;
  return 0;
}

static int rdm_data_handler(artnet_node vn, int address, uint8_t *rdm, int length, void *data) {
  rdm_capture_t *capture = (rdm_capture_t *)data;
  (void)vn;
  capture->called++;
  capture->address = address;
  capture->length = length;
  if (length > 0) {
    memcpy(capture->data, rdm, (size_t)length);
  }
  return 0;
}

static int firmware_data_handler(artnet_node vn, int ubea, uint16_t *fw, int length, void *data) {
  firmware_capture_t *capture = (firmware_capture_t *)data;
  (void)vn;
  capture->called++;
  capture->ubea = ubea;
  capture->length = length;
  if (length > 0) {
    memcpy(capture->data, fw, (size_t)length * sizeof(uint16_t));
  }
  return 0;
}

static int firmware_status_handler(artnet_node vn, artnet_firmware_status_code code, void *data) {
  firmware_status_capture_t *capture = (firmware_status_capture_t *)data;
  (void)vn;
  capture->called++;
  capture->code = code;
  return 0;
}

static void test_poll_schedules_unicast_reply_and_reply_on_change_flag(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.110");

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  init_packet(&p, ARTNET_POLL, requester);
  p.data.ap.flags = ARTNET_POLL_FLAG_REPLY_ON_CHANGE;

  handle(&n, &p);

  ASSERT_TRUE(n.state.reply_addr.s_addr == requester.s_addr,
              "handle_poll should set reply target to the poller");
  ASSERT_TRUE(n.state.send_apr_on_change == TRUE,
              "handle_poll should enable reply-on-change when requested");
  ASSERT_TRUE(n.state.apr_pending == TRUE,
              "handle_poll should schedule an ArtPollReply instead of sending immediately");
  ASSERT_TRUE(send_capture.called == 0,
              "handle_poll should not send an immediate ArtPollReply");

  n.state.apr_pending_time = 0;
  check_timeouts(&n);

  ASSERT_TRUE(send_capture.called == 1,
              "check_timeouts should send the pending ArtPollReply");
  ASSERT_TRUE(send_capture.type == ARTNET_REPLY,
              "pending poll reply should be sent as ArtPollReply");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "ArtPollReply should be unicast to the poller");

  stop_sendable_node(&n);
}

static void test_legacy_14_byte_poll_is_accepted(void) {
  artnet_node_t n;
  artnet_packet_t p;
  struct in_addr requester = ip4("127.0.0.112");

  init_test_node(&n);
  init_packet(&p, ARTNET_POLL, requester);
  p.length = 14;

  handle(&n, &p);

  ASSERT_TRUE(n.state.reply_addr.s_addr == requester.s_addr,
              "handle should accept legacy 14-byte ArtPoll packets");
  ASSERT_TRUE(n.state.apr_pending == TRUE,
              "legacy 14-byte ArtPoll should schedule an ArtPollReply");
  ASSERT_TRUE(n.state.report_code != ARTNET_RC_PARSE_FAIL,
              "legacy 14-byte ArtPoll should not be reported as parse failure");
}

static void test_poll_vlc_disable_flag_controls_vlc_sends(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  uint8_t vlc[ARTNET_VLC_MIN_LENGTH + 1];

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.ports.in[0].port_enabled = TRUE;
  n.ports.in[0].port_addr = make_addr(1, 2, 6);
  add_stub_node_entry(&n, "192.168.1.14", 1, 2, 6);
  set_vlc_magic(vlc, 1);
  vlc[ARTNET_VLC_MIN_LENGTH] = 0x44;

  init_packet(&p, ARTNET_POLL, ip4("127.0.0.113"));
  p.data.ap.flags = ARTNET_POLL_FLAG_VLC_DISABLE;
  handle_poll(&n, &p);

  ASSERT_TRUE(n.state.vlc_disabled == TRUE,
              "ArtPoll Flags bit 4 should disable VLC transmission");
  ASSERT_TRUE(artnet_send_vlc((artnet_node)&n, make_addr(1, 2, 6),
                              (int16_t)sizeof(vlc), vlc) == ARTNET_EACTION,
              "artnet_send_vlc should refuse sends while VLC transmission is disabled");

  p.data.ap.flags = 0;
  handle_poll(&n, &p);
  ASSERT_TRUE(n.state.vlc_disabled == FALSE,
              "ArtPoll with VLC enabled should clear the VLC disable flag");
  ASSERT_TRUE(artnet_send_vlc((artnet_node)&n, make_addr(1, 2, 6),
                              (int16_t)sizeof(vlc), vlc) == ARTNET_EOK,
              "artnet_send_vlc should send after VLC transmission is re-enabled");
  ASSERT_TRUE(send_capture.called == 1 && send_capture.type == ARTNET_NZS,
              "ArtVlc should be sent as an ArtNzs packet");

  stop_sendable_node(&n);
}

static void test_poll_target_mode_filters_non_matching_node(void) {
  artnet_node_t n;
  artnet_packet_t p;
  struct in_addr requester = ip4("127.0.0.111");

  init_test_node(&n);
  init_packet(&p, ARTNET_POLL, requester);
  p.data.ap.flags = ARTNET_POLL_FLAG_TARGET_MODE;
  p.data.ap.targetPortAddressBottomHi = 0x12;
  p.data.ap.targetPortAddressBottomLo = 0x34;
  p.data.ap.targetPortAddressTopHi = 0x12;
  p.data.ap.targetPortAddressTopLo = 0x34;

  ASSERT_TRUE(handle_poll(&n, &p) == ARTNET_EOK,
              "handle_poll should accept targeted mode packets");
  ASSERT_TRUE(n.state.apr_pending == FALSE,
              "handle_poll should ignore targeted polls that do not match any configured port");
}

static void test_tod_request_unicasts_tod_data_to_requester(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.120");
  uint8_t uid[ARTNET_RDM_UID_WIDTH] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(0x01, 0x02, 0x03);
  add_tod_uid(&n.ports.out[0].port_tod, uid);

  init_packet(&p, ARTNET_TODREQUEST, requester);
  p.data.todreq.command = ARTNET_TOD_FULL;
  p.data.todreq.net = 0x01;
  p.data.todreq.adCount = 1;
  p.data.todreq.address[0] = 0x23;

  ASSERT_TRUE(handle_tod_request(&n, &p) == ARTNET_EOK,
              "handle_tod_request should succeed for a matching output port");
  ASSERT_TRUE(n.state.tod_reply_addr.s_addr == requester.s_addr,
              "handle_tod_request should store the TOD requester address");
  ASSERT_TRUE(send_capture.called == 1,
              "handle_tod_request should emit one ArtTodData packet for one UID");
  ASSERT_TRUE(send_capture.type == ARTNET_TODDATA,
              "handle_tod_request should respond with ArtTodData");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "ArtTodData should be unicast to the TOD requester");
  ASSERT_TRUE(send_capture.data.toddata.net == 0x01 &&
              send_capture.data.toddata.address == 0x23,
              "ArtTodData should report the requested port address");
  ASSERT_TRUE(send_capture.data.toddata.uidCount == 1,
              "ArtTodData should include the single discovered UID");
  ASSERT_TRUE(memcmp(send_capture.data.toddata.tod[0], uid, ARTNET_RDM_UID_WIDTH) == 0,
              "ArtTodData should include the configured UID");

  stop_sendable_node(&n);
}

static void test_tod_updates_are_sent_to_all_previous_requesters(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester1 = ip4("127.0.0.122");
  struct in_addr requester2 = ip4("127.0.0.123");
  uint8_t uid1[ARTNET_RDM_UID_WIDTH] = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26};
  uint8_t uid2[ARTNET_RDM_UID_WIDTH] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(0x01, 0x02, 0x03);
  add_tod_uid(&n.ports.out[0].port_tod, uid1);

  init_packet(&p, ARTNET_TODREQUEST, requester1);
  p.data.todreq.command = ARTNET_TOD_FULL;
  p.data.todreq.net = 0x01;
  p.data.todreq.adCount = 1;
  p.data.todreq.address[0] = 0x23;
  ASSERT_TRUE(handle_tod_request(&n, &p) == ARTNET_EOK,
              "first ArtTodRequest should succeed");

  p.from = requester2;
  ASSERT_TRUE(handle_tod_request(&n, &p) == ARTNET_EOK,
              "second ArtTodRequest should succeed");
  ASSERT_TRUE(n.state.tod_requester_count == 2,
              "TOD requester list should retain distinct previous requesters");

  send_capture.called = 0;
  ASSERT_TRUE(artnet_add_rdm_device((artnet_node)&n, 0, uid2) == ARTNET_EOK,
              "adding an RDM device should send TOD updates");
  ASSERT_TRUE(send_capture.called == 2,
              "TOD updates should be unicast to all previous TOD requesters");
  ASSERT_TRUE(send_capture.to.s_addr == requester2.s_addr,
              "last TOD update should target the last remembered requester");

  stop_sendable_node(&n);
}

static void test_tod_control_flush_triggers_discovery_and_empty_tod_reply(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  rdm_init_capture_t init_capture = {0};
  struct in_addr requester = ip4("127.0.0.121");
  uint8_t uid[ARTNET_RDM_UID_WIDTH] = {1, 2, 3, 4, 5, 6};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.callbacks.rdm_init_c.fh = rdm_init_handler;
  n.callbacks.rdm_init_c.data = &init_capture;
  n.ports.out[1].port_enabled = TRUE;
  n.ports.out[1].port_addr = make_addr(0x02, 0x03, 0x04);
  add_tod_uid(&n.ports.out[1].port_tod, uid);

  init_packet(&p, ARTNET_TODCONTROL, requester);
  p.data.todcontrol.cmd = ARTNET_TOD_FLUSH;
  p.data.todcontrol.net = 0x02;
  p.data.todcontrol.address = 0x34;

  ASSERT_TRUE(handle_tod_control(&n, &p) == ARTNET_EOK,
              "handle_tod_control should succeed for FLUSH on a matching port");
  ASSERT_TRUE(init_capture.called == 1 && init_capture.port == 1,
              "ArtTodControl FLUSH should trigger RDM discovery callback for the matching port");
  ASSERT_TRUE(send_capture.called == 1,
              "ArtTodControl FLUSH should emit one ArtTodData reply");
  ASSERT_TRUE(send_capture.type == ARTNET_TODDATA,
              "ArtTodControl FLUSH should reply with ArtTodData");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "ArtTodData reply to ArtTodControl should be unicast to the requester");
  ASSERT_TRUE(send_capture.data.toddata.uidCount == 0,
              "ArtTodControl FLUSH should reply with an empty TOD after flushing");
  ASSERT_TRUE(send_capture.data.toddata.uidTotalHi == 0 &&
              send_capture.data.toddata.uidTotal == 0,
              "ArtTodControl FLUSH should report zero total UIDs after flushing");

  stop_sendable_node(&n);
}

static void test_address_programming_updates_node_state_and_replies(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.130");
  uint8_t in_addrs[ARTNET_MAX_PORTS] = {PROGRAM_NO_CHANGE, PROGRAM_NO_CHANGE, PROGRAM_NO_CHANGE, PROGRAM_NO_CHANGE};
  uint8_t out_addrs[ARTNET_MAX_PORTS] = {PROGRAM_NO_CHANGE, PROGRAM_NO_CHANGE, PROGRAM_NO_CHANGE, PROGRAM_NO_CHANGE};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.state.reply_addr = ip4("127.0.0.199");
  n.state.default_netSwitch = 0;
  n.state.default_subSwitch = 0;
  n.state.netSwitch = 0;
  n.state.subSwitch = 0;
  n.ports.in[0].port_addr = make_addr(0, 0, 1);
  n.ports.in[0].port_default_addr = 1;
  n.ports.out[0].port_addr = make_addr(0, 0, 2);
  n.ports.out[0].port_default_addr = 2;

  init_packet(&p, ARTNET_ADDRESS, requester);
  memset(p.data.addr.shortName, 0, ARTNET_SHORT_NAME_LENGTH);
  memcpy(p.data.addr.shortName, "Node A", 6);
  memset(p.data.addr.longName, 0, ARTNET_LONG_NAME_LENGTH);
  memcpy(p.data.addr.longName, "Node A Long", 11);
  memcpy(p.data.addr.swIn, in_addrs, ARTNET_MAX_PORTS);
  memcpy(p.data.addr.swOut, out_addrs, ARTNET_MAX_PORTS);
  p.data.addr.netSwitch = 0x80 | 0x05;
  p.data.addr.bindIndex = 1;
  p.data.addr.subSwitch = 0x80 | 0x03;
  p.data.addr.swOut[0] = 0x80 | 0x07;
  p.data.addr.acnPriority = 100;
  p.data.addr.command = ARTNET_PC_FAIL_ZERO;

  ASSERT_TRUE(handle_address(&n, &p) == ARTNET_EOK,
              "handle_address should succeed for programmable node");
  ASSERT_TRUE(strcmp(n.state.shortName, "Node A") == 0,
              "handle_address should update short name");
  ASSERT_TRUE(strcmp(n.state.longName, "Node A Long") == 0,
              "handle_address should update long name");
  ASSERT_TRUE(n.state.netSwitch == 0x05 && n.state.subSwitch == 0x03,
              "handle_address should update net and subnet");
  ASSERT_TRUE(n.ports.out[0].port_addr == make_addr(0x05, 0x03, 0x07),
              "handle_address should update output port address");
  ASSERT_TRUE(n.state.acn_priority == 100,
              "handle_address should update sACN priority");
  ASSERT_TRUE(n.state.failsafe_mode == ARTNET_FAILSAFE_ZERO,
              "handle_address should update failsafe mode");
  ASSERT_TRUE(send_capture.called == 1,
              "handle_address should emit one ArtPollReply");
  ASSERT_TRUE(send_capture.type == ARTNET_REPLY,
              "handle_address should respond with ArtPollReply");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "handle_address should unicast ArtPollReply to the current requester");

  stop_sendable_node(&n);
}

static void test_address_bind_index_filters_other_bound_pages(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.134");

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.state.bind_index = 2;
  n.state.acn_priority = 0xFF;

  init_packet(&p, ARTNET_ADDRESS, requester);
  memset(p.data.addr.shortName, PROGRAM_NO_CHANGE, ARTNET_SHORT_NAME_LENGTH);
  memset(p.data.addr.longName, PROGRAM_NO_CHANGE, ARTNET_LONG_NAME_LENGTH);
  memset(p.data.addr.swIn, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  memset(p.data.addr.swOut, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  p.data.addr.bindIndex = 1;
  p.data.addr.netSwitch = PROGRAM_NO_CHANGE;
  p.data.addr.subSwitch = PROGRAM_NO_CHANGE;
  p.data.addr.acnPriority = 100;

  ASSERT_TRUE(handle_address(&n, &p) == ARTNET_EOK,
              "ArtAddress with non-matching BindIndex should be ignored cleanly");
  ASSERT_TRUE(n.state.acn_priority == 0xFF,
              "ArtAddress should not reprogram state for a different BindIndex");
  ASSERT_TRUE(send_capture.called == 0,
              "ArtAddress should not reply for a different BindIndex");

  p.data.addr.bindIndex = 2;
  ASSERT_TRUE(handle_address(&n, &p) == ARTNET_EOK,
              "ArtAddress with matching BindIndex should be processed");
  ASSERT_TRUE(n.state.acn_priority == 100,
              "ArtAddress should reprogram state for the matching BindIndex");
  ASSERT_TRUE(send_capture.called == 1,
              "ArtAddress should reply for the matching BindIndex");

  stop_sendable_node(&n);
}

static void test_address_programming_recomputes_ports_when_only_net_changes(void) {
  artnet_node_t n;
  artnet_packet_t p;
  struct in_addr requester = ip4("127.0.0.133");

  init_test_node(&n);
  start_sendable_node(&n);
  n.state.reply_addr = requester;
  n.state.default_netSwitch = 0;
  n.state.default_subSwitch = 2;
  n.state.netSwitch = 0;
  n.state.subSwitch = 2;
  n.ports.in[0].port_addr = make_addr(0, 2, 1);
  n.ports.out[0].port_addr = make_addr(0, 2, 2);

  init_packet(&p, ARTNET_ADDRESS, requester);
  memset(p.data.addr.shortName, PROGRAM_NO_CHANGE, ARTNET_SHORT_NAME_LENGTH);
  memset(p.data.addr.longName, PROGRAM_NO_CHANGE, ARTNET_LONG_NAME_LENGTH);
  memset(p.data.addr.swIn, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  memset(p.data.addr.swOut, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  p.data.addr.netSwitch = 0x80 | 0x05;
  p.data.addr.bindIndex = 1;
  p.data.addr.subSwitch = PROGRAM_NO_CHANGE;
  p.data.addr.acnPriority = 0xFF;
  p.data.addr.command = ARTNET_PC_NONE;

  ASSERT_TRUE(handle_address(&n, &p) == ARTNET_EOK,
              "handle_address should accept net-only reprogramming");
  ASSERT_TRUE(n.ports.in[0].port_addr == make_addr(0x05, 0x02, 0x01),
              "net-only reprogramming should rebuild input port addresses");
  ASSERT_TRUE(n.ports.out[0].port_addr == make_addr(0x05, 0x02, 0x02),
              "net-only reprogramming should rebuild output port addresses");

  stop_sendable_node(&n);
}

static void test_address_rdm_and_bqp_commands_update_state(void) {
  artnet_node_t n;
  artnet_packet_t p;
  struct in_addr requester = ip4("127.0.0.131");

  init_test_node(&n);
  start_sendable_node(&n);
  n.state.reply_addr = requester;
  n.ports.out[0].rdm_enabled = 1;
  n.state.bqp_policy = ARTNET_BQP_NONE;

  init_packet(&p, ARTNET_ADDRESS, requester);
  memset(p.data.addr.shortName, PROGRAM_NO_CHANGE, ARTNET_SHORT_NAME_LENGTH);
  memset(p.data.addr.longName, PROGRAM_NO_CHANGE, ARTNET_LONG_NAME_LENGTH);
  memset(p.data.addr.swIn, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  memset(p.data.addr.swOut, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  p.data.addr.netSwitch = PROGRAM_NO_CHANGE;
  p.data.addr.bindIndex = 1;
  p.data.addr.subSwitch = PROGRAM_NO_CHANGE;
  p.data.addr.acnPriority = 0xFF;

  p.data.addr.command = ARTNET_PC_RDM_DISABLED_0;
  ASSERT_TRUE(handle_address(&n, &p) == ARTNET_EOK,
              "handle_address should accept RDM disable command");
  ASSERT_TRUE(n.ports.out[0].rdm_enabled == 0,
              "handle_address should disable RDM on port 0");

  p.data.addr.command = ARTNET_PC_BQP_WARNING;
  ASSERT_TRUE(handle_address(&n, &p) == ARTNET_EOK,
              "handle_address should accept BQP command");
  ASSERT_TRUE(n.state.bqp_policy == ARTNET_PC_BQP_WARNING,
              "handle_address should update background queue policy");

  stop_sendable_node(&n);
}

static void test_address_ignores_deprecated_port_index_commands(void) {
  artnet_node_t n;
  artnet_packet_t p;

  init_test_node(&n);
  n.state.mode = ARTNET_ON;
  n.state.node_type = ARTNET_NODE;

  n.ports.out[1].rdm_enabled = 1;
  n.ports.out[1].merge_mode = ARTNET_MERGE_HTP;
  n.ports.out[1].port_status = 0;
  n.ports.out[1].proto_sel = 0;

  init_packet(&p, ARTNET_ADDRESS, ip4("127.0.0.143"));
  memset(p.data.addr.shortName, PROGRAM_NO_CHANGE, ARTNET_SHORT_NAME_LENGTH);
  memset(p.data.addr.longName, PROGRAM_NO_CHANGE, ARTNET_LONG_NAME_LENGTH);
  memset(p.data.addr.swIn, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  memset(p.data.addr.swOut, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  p.data.addr.bindIndex = 1;
  p.data.addr.netSwitch = PROGRAM_NO_CHANGE;
  p.data.addr.subSwitch = PROGRAM_NO_CHANGE;
  p.data.addr.acnPriority = 0xFF;

  p.data.addr.command = ARTNET_PC_RDM_DISABLED_1;
  ASSERT_TRUE(handle_address(&n, &p) == ARTNET_EOK,
              "deprecated ArtAddress RDM disable command should be ignored cleanly");
  ASSERT_TRUE(n.ports.out[1].rdm_enabled == 1,
              "deprecated ArtAddress RDM disable command must not update port 1");

  p.data.addr.command = ARTNET_PC_MERGE_LTP_1;
  ASSERT_TRUE(handle_address(&n, &p) == ARTNET_EOK,
              "deprecated ArtAddress merge command should be ignored cleanly");
  ASSERT_TRUE(n.ports.out[1].merge_mode == ARTNET_MERGE_HTP &&
              (n.ports.out[1].port_status & PORT_STATUS_LPT_MODE) == 0,
              "deprecated ArtAddress merge command must not update port 1");

  p.data.addr.command = ARTNET_PC_ACN_SEL_1;
  ASSERT_TRUE(handle_address(&n, &p) == ARTNET_EOK,
              "deprecated ArtAddress protocol command should be ignored cleanly");
  ASSERT_TRUE(n.ports.out[1].proto_sel == 0,
              "deprecated ArtAddress protocol command must not update port 1");
}

static void test_input_disable_and_enable_updates_port_status_and_reply(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.132");

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.state.reply_addr = ip4("127.0.0.198");

  init_packet(&p, ARTNET_INPUT, requester);
  p.data.ainput.bindIndex = 1;
  p.data.ainput.numbports = 1;
  p.data.ainput.input[0] = PORT_DISABLE_MASK;

  ASSERT_TRUE(_artnet_handle_input(&n, &p) == ARTNET_EOK,
              "_artnet_handle_input should succeed when disabling a port");
  ASSERT_TRUE((n.ports.in[0].port_status & PORT_STATUS_INPUT_DISABLED) != 0,
              "_artnet_handle_input should set input disabled bit");
  ASSERT_TRUE(send_capture.called == 1 && send_capture.type == ARTNET_REPLY,
              "_artnet_handle_input should respond with ArtPollReply after disable");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "_artnet_handle_input should reply to the current requester after disable");

  send_capture.called = 0;
  p.data.ainput.input[0] = 0x00;
  ASSERT_TRUE(_artnet_handle_input(&n, &p) == ARTNET_EOK,
              "_artnet_handle_input should succeed when enabling a port");
  ASSERT_TRUE((n.ports.in[0].port_status & PORT_STATUS_INPUT_DISABLED) == 0,
              "_artnet_handle_input should clear input disabled bit");
  ASSERT_TRUE(send_capture.called == 1 && send_capture.type == ARTNET_REPLY,
              "_artnet_handle_input should respond with ArtPollReply after enable");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "_artnet_handle_input should reply to the current requester after enable");

  stop_sendable_node(&n);
}

static void test_input_bind_index_filters_other_bound_pages(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.135");

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.state.bind_index = 2;

  init_packet(&p, ARTNET_INPUT, requester);
  p.data.ainput.bindIndex = 1;
  p.data.ainput.numbports = 1;
  p.data.ainput.input[0] = PORT_DISABLE_MASK;

  ASSERT_TRUE(_artnet_handle_input(&n, &p) == ARTNET_EOK,
              "ArtInput with non-matching BindIndex should be ignored cleanly");
  ASSERT_TRUE((n.ports.in[0].port_status & PORT_STATUS_INPUT_DISABLED) == 0,
              "ArtInput should not change input state for a different BindIndex");
  ASSERT_TRUE(send_capture.called == 0,
              "ArtInput should not reply for a different BindIndex");

  p.data.ainput.bindIndex = 2;
  ASSERT_TRUE(_artnet_handle_input(&n, &p) == ARTNET_EOK,
              "ArtInput with matching BindIndex should be processed");
  ASSERT_TRUE((n.ports.in[0].port_status & PORT_STATUS_INPUT_DISABLED) != 0,
              "ArtInput should change input state for the matching BindIndex");
  ASSERT_TRUE(send_capture.called == 1,
              "ArtInput should reply for the matching BindIndex");

  stop_sendable_node(&n);
}

static void test_poll_reply_build_populates_artnet4_fields(void) {
  artnet_node_t n;
  artnet_reply_t *reply = NULL;
  uint8_t uid[ARTNET_RDM_UID_WIDTH] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};

  init_test_node(&n);
  n.state.mode = ARTNET_ON;
  n.state.style_code = ARTNET_ST_NODE;
  n.state.acn_priority = 123;
  n.state.status2 = ARTNET_STATUS2_15BIT_ADDR | ARTNET_STATUS2_RDM_CONTROL | ARTNET_STATUS2_SACN_SWITCHABLE;
  n.state.status3 = ARTNET_STATUS3_PORT_DIRECTION | ARTNET_STATUS3_RDMNET;
  n.state.failsafe_mode = ARTNET_FAILSAFE_SCENE;
  n.state.bqp_policy = ARTNET_BQP_WARNING;
  n.state.refresh_rate = 44;
  n.state.bind_index = 3;
  memcpy(n.state.default_resp_uid, uid, sizeof(uid));
  n.ports.types[0] = (uint8_t)((uint8_t)ARTNET_ENABLE_OUTPUT | (uint8_t)ARTNET_PORT_DMX);
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(1, 2, 3);
  n.ports.out[0].output_style = 1;
  n.ports.out[0].rdm_enabled = 0;

  ASSERT_TRUE(artnet_tx_build_art_poll_reply(&n) == ARTNET_EOK,
              "artnet_tx_build_art_poll_reply should succeed");

  reply = &n.ar_temp;
  ASSERT_TRUE(reply->acnPriority == 123,
              "PollReply should carry configured sACN priority");
  ASSERT_TRUE(reply->bindIndex == 3,
              "PollReply should carry configured BindIndex");
  ASSERT_TRUE(memcmp(reply->bindIp, &n.state.ip_addr.s_addr, ARTNET_IP_SIZE) == 0,
              "PollReply should publish bind IP equal to node IP");
  ASSERT_TRUE(reply->estaMan[0] == 0x34 && reply->estaMan[1] == 0x12,
              "PollReply should encode ESTA manufacturer bytes in wire-order low/high");
  ASSERT_TRUE(reply->status2 == n.state.status2,
              "PollReply should carry Status2 flags");
  ASSERT_TRUE((reply->status & STATUS_PROG_AUTH_MASK) == 0x10,
              "PollReply should default programming authority to front-panel/local control");
  ASSERT_TRUE(reply->status3 == (uint8_t)(ARTNET_FAILSAFE_SCENE | n.state.status3),
              "PollReply should combine failsafe mode with Status3 flags");
  ASSERT_TRUE(reply->goodOutputB[0] == (ARTNET_GOODB_RDM_DISABLED | ARTNET_GOODB_STYLE_CONSTANT),
              "PollReply should publish GoodOutputB state for RDM disabled and constant style");
  ASSERT_TRUE(reply->refreshRateHi == 0 && reply->refreshRateLo == 44,
              "PollReply should publish configured refresh rate");
  ASSERT_TRUE(reply->bgQueuePolicy == ARTNET_BQP_WARNING,
              "PollReply should publish background queue policy");
  ASSERT_TRUE(memcmp(reply->defaultRespUid, uid, sizeof(uid)) == 0,
              "PollReply should publish default responder UID");
}

static void test_poll_reply_build_marks_network_programming_and_bg_discovery_state(void) {
  artnet_node_t n;
  artnet_reply_t *reply = NULL;

  init_test_node(&n);
  n.state.mode = ARTNET_ON;
  n.state.netSwitch_net_ctl = TRUE;
  n.state.bqp_policy = ARTNET_BQP_DISABLED;
  n.ports.types[0] = (uint8_t)((uint8_t)ARTNET_ENABLE_OUTPUT | (uint8_t)ARTNET_PORT_DMX);
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].rdm_enabled = 1;
  n.ports.out[0].output_style = 0;

  ASSERT_TRUE(artnet_tx_build_art_poll_reply(&n) == ARTNET_EOK,
              "artnet_tx_build_art_poll_reply should succeed for network-programmed state");

  reply = &n.ar_temp;
  ASSERT_TRUE((reply->status & STATUS_PROG_AUTH_MASK) == 0x20,
              "PollReply should mark programming authority as network-controlled when remote programming is active");
  ASSERT_TRUE(reply->goodOutputB[0] == (ARTNET_GOODB_DISCOVERY_IDLE | ARTNET_GOODB_BG_DISCOVERY_DISABLED),
              "PollReply should publish discovery idle and background discovery disabled bits when applicable");
}

static void test_send_dmx_unicasts_to_matching_subscribers_only(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  uint8_t data[4] = {1, 2, 3, 4};

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.ports.in[0].port_enabled = TRUE;
  n.ports.in[0].port_addr = make_addr(1, 2, 3);

  add_stub_node_entry(&n, "192.168.1.10", 1, 2, 3);
  add_stub_node_entry(&n, "192.168.1.11", 1, 2, 4);

  ASSERT_TRUE(artnet_send_dmx((artnet_node)&n, 0, 4, data) == ARTNET_EOK,
              "artnet_send_dmx should succeed on enabled input port");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_send_dmx should unicast to exactly one matching subscriber");
  ASSERT_TRUE(send_capture.type == ARTNET_DMX,
              "artnet_send_dmx should send ArtDmx packets");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.10").s_addr,
              "artnet_send_dmx should target the matching subscriber IP");
  ASSERT_TRUE(send_capture.data.admx.universe == htols(make_addr(1, 2, 3)),
              "artnet_send_dmx should encode the correct universe");
  ASSERT_TRUE(send_capture.data.admx.length == 4,
              "artnet_send_dmx should encode the correct payload length");
  ASSERT_TRUE(memcmp(send_capture.data.admx.data, data, 4) == 0,
              "artnet_send_dmx should copy DMX payload");
  ASSERT_TRUE(send_capture.data.admx.sequence == 1,
              "artnet_send_dmx should send a non-zero sequence number");
  ASSERT_TRUE(n.ports.in[0].seq == 2,
              "artnet_send_dmx should advance sequence number");
  ASSERT_TRUE(n.ports.in[0].last_dmx_length == 4 &&
              memcmp(n.ports.in[0].last_dmx_data, data, 4) == 0,
              "artnet_send_dmx should store keepalive payload");

  stop_sendable_node(&n);
}

static void test_send_dmx_matches_swin_subscribers_and_wraps_sequence(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  node_entry_private_t *entry = NULL;
  uint8_t data[2] = {9, 10};

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.ports.in[0].port_enabled = TRUE;
  n.ports.in[0].port_addr = make_addr(1, 2, 3);
  n.ports.in[0].seq = 255;

  entry = add_stub_node_entry(&n, "192.168.1.15", 1, 2, 4);
  entry->pub.portTypes[0] = (uint8_t)((uint8_t)ARTNET_ENABLE_INPUT | (uint8_t)ARTNET_PORT_DMX);
  entry->pub.swIn[0] = 3;

  ASSERT_TRUE(artnet_send_dmx((artnet_node)&n, 0, 2, data) == ARTNET_EOK,
              "artnet_send_dmx should succeed for SwIn subscribers");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_send_dmx should unicast to SwIn subscribers");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.15").s_addr,
              "artnet_send_dmx should target a matching SwIn subscriber");
  ASSERT_TRUE(send_capture.data.admx.sequence == 255,
              "artnet_send_dmx should send the current 255 sequence value");
  ASSERT_TRUE(n.ports.in[0].seq == 1,
              "artnet_send_dmx should wrap sequence from 255 to 1");

  stop_sendable_node(&n);
}

static void test_send_dmx_rejects_invalid_lengths(void) {
  artnet_node_t n;
  uint8_t data[4] = {1, 2, 3, 4};

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.ports.in[0].port_enabled = TRUE;
  n.ports.in[0].port_addr = make_addr(1, 2, 3);

  ASSERT_TRUE(artnet_send_dmx((artnet_node)&n, 0, 1, data) == ARTNET_EARG,
              "artnet_send_dmx should reject length 1");
  ASSERT_TRUE(artnet_send_dmx((artnet_node)&n, 0, 3, data) == ARTNET_EARG,
              "artnet_send_dmx should reject odd ArtDmx lengths");

  stop_sendable_node(&n);
}

static void test_raw_send_dmx_unicasts_to_subscribers_only(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  uint8_t data[2] = {1, 2};

  init_test_node(&n);
  n.state.node_type = ARTNET_RAW;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  add_stub_node_entry(&n, "192.168.1.16", 1, 2, 3);
  add_stub_node_entry(&n, "192.168.1.17", 1, 2, 4);

  ASSERT_TRUE(artnet_raw_send_dmx((artnet_node)&n, make_addr(1, 2, 3), 2, data) == ARTNET_EOK,
              "artnet_raw_send_dmx should succeed with a matching subscriber");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_raw_send_dmx should unicast to exactly one matching subscriber");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.16").s_addr,
              "artnet_raw_send_dmx should target the matching subscriber");

  send_capture.called = 0;
  ASSERT_TRUE(artnet_raw_send_dmx((artnet_node)&n, make_addr(1, 2, 8), 2, data) == ARTNET_EOK,
              "artnet_raw_send_dmx should succeed even when no subscriber exists");
  ASSERT_TRUE(send_capture.called == 0,
              "artnet_raw_send_dmx should not broadcast when no subscriber exists");
  ASSERT_TRUE(artnet_raw_send_dmx((artnet_node)&n, make_addr(1, 2, 3), 1, data) == ARTNET_EARG,
              "artnet_raw_send_dmx should reject length 1");
  ASSERT_TRUE(artnet_raw_send_dmx((artnet_node)&n, make_addr(1, 2, 3), 3, data) == ARTNET_EARG,
              "artnet_raw_send_dmx should reject odd ArtDmx lengths");

  stop_sendable_node(&n);
}

static void test_send_nzs_unicasts_and_preserves_start_code(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  uint8_t data[3] = {9, 8, 7};

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.ports.in[0].port_enabled = TRUE;
  n.ports.in[0].port_addr = make_addr(1, 2, 5);

  add_stub_node_entry(&n, "192.168.1.12", 1, 2, 5);

  ASSERT_TRUE(artnet_send_nzs((artnet_node)&n, make_addr(1, 2, 5), 0xCF, 3, data) == ARTNET_EOK,
              "artnet_send_nzs should succeed for a configured input universe");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_send_nzs should unicast to exactly one matching subscriber");
  ASSERT_TRUE(send_capture.type == ARTNET_NZS,
              "artnet_send_nzs should emit ArtNzs packets");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.12").s_addr,
              "artnet_send_nzs should target the matching subscriber IP");
  ASSERT_TRUE(send_capture.data.nzs.startCode == 0xCF,
              "artnet_send_nzs should preserve the non-zero start code");
  ASSERT_TRUE(send_capture.data.nzs.length == 3,
              "artnet_send_nzs should encode the correct payload length");
  ASSERT_TRUE(memcmp(send_capture.data.nzs.data, data, 3) == 0,
              "artnet_send_nzs should copy NZS payload");

  stop_sendable_node(&n);
}

static void test_send_nzs_raw_unicasts_to_subscribers_only(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  uint8_t data[3] = {1, 2, 3};

  init_test_node(&n);
  n.state.node_type = ARTNET_RAW;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  add_stub_node_entry(&n, "192.168.1.18", 1, 2, 7);

  ASSERT_TRUE(artnet_send_nzs((artnet_node)&n, make_addr(1, 2, 7), 0xCF, 3, data) == ARTNET_EOK,
              "artnet_send_nzs should succeed in raw mode with a matching subscriber");
  ASSERT_TRUE(send_capture.called == 1,
              "raw ArtNzs should unicast to matching subscribers");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.18").s_addr,
              "raw ArtNzs should target the matching subscriber");

  send_capture.called = 0;
  ASSERT_TRUE(artnet_send_nzs((artnet_node)&n, make_addr(1, 2, 8), 0xCF, 3, data) == ARTNET_EOK,
              "raw ArtNzs should succeed when no subscriber exists");
  ASSERT_TRUE(send_capture.called == 0,
              "raw ArtNzs should not broadcast when no subscriber exists");

  stop_sendable_node(&n);
}

static void test_send_vlc_validates_magic_and_payload_count(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  uint8_t vlc[ARTNET_VLC_MIN_LENGTH + 2];

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.ports.in[0].port_enabled = TRUE;
  n.ports.in[0].port_addr = make_addr(1, 2, 6);
  add_stub_node_entry(&n, "192.168.1.19", 1, 2, 6);

  set_vlc_magic(vlc, 2);
  vlc[ARTNET_VLC_MIN_LENGTH] = 0xab;
  vlc[ARTNET_VLC_MIN_LENGTH + 1] = 0xcd;

  ASSERT_TRUE(artnet_send_vlc((artnet_node)&n, make_addr(1, 2, 6),
                              (int16_t)sizeof(vlc), vlc) == ARTNET_EOK,
              "artnet_send_vlc should accept valid VLC payloads");
  ASSERT_TRUE(send_capture.called == 1 && send_capture.type == ARTNET_NZS,
              "artnet_send_vlc should emit ArtNzs");
  ASSERT_TRUE(send_capture.data.nzs.startCode == ARTNET_VLC_START_CODE,
              "artnet_send_vlc should encode VLC start code");
  ASSERT_TRUE(send_capture.data.nzs.length == sizeof(vlc),
              "artnet_send_vlc should encode VLC payload length");

  vlc[0] = 0x00;
  ASSERT_TRUE(artnet_send_vlc((artnet_node)&n, make_addr(1, 2, 6),
                              (int16_t)sizeof(vlc), vlc) == ARTNET_EARG,
              "artnet_send_vlc should reject missing VLC magic");

  set_vlc_magic(vlc, 1);
  ASSERT_TRUE(artnet_send_vlc((artnet_node)&n, make_addr(1, 2, 6),
                              (int16_t)sizeof(vlc), vlc) == ARTNET_EARG,
              "artnet_send_vlc should reject mismatched payload count");

  stop_sendable_node(&n);
}

static void test_send_data_request_encodes_target_and_request_code(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  ASSERT_TRUE(artnet_send_data_request((artnet_node)&n, "192.168.1.55", ARTNET_DR_URL_PRODUCT) == ARTNET_EOK,
              "artnet_send_data_request should succeed for a valid target");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_send_data_request should emit one outbound packet");
  ASSERT_TRUE(send_capture.type == ARTNET_DATAREQUEST,
              "artnet_send_data_request should send an ArtDataRequest packet");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.55").s_addr,
              "artnet_send_data_request should target the requested IP address");
  ASSERT_TRUE(send_capture.data.datareq.requestHi == 0x00 &&
              send_capture.data.datareq.requestLo == 0x01,
              "artnet_send_data_request should encode the selected request code");
  ASSERT_TRUE(send_capture.data.datareq.estaManHi == 0x12 &&
              send_capture.data.datareq.estaManLo == 0x34,
              "artnet_send_data_request should include the node ESTA code");
  ASSERT_TRUE(send_capture.data.datareq.oemHi == 0x56 &&
              send_capture.data.datareq.oemLo == 0x78,
              "artnet_send_data_request should include the node OEM code");

  stop_sendable_node(&n);
}

static void test_send_data_reply_encodes_target_and_payload(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  const char payload[] = "https://example.invalid/product";
  int payload_len = (int)strlen(payload) + 1;

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  ASSERT_TRUE(artnet_send_data_reply((artnet_node)&n,
                                     "192.168.1.56",
                                     ARTNET_DR_URL_PRODUCT,
                                     payload,
                                     (int16_t)payload_len) == ARTNET_EOK,
              "artnet_send_data_reply should succeed for a valid target");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_send_data_reply should emit one outbound packet");
  ASSERT_TRUE(send_capture.type == ARTNET_DATAREPLY,
              "artnet_send_data_reply should send an ArtDataReply packet");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.56").s_addr,
              "artnet_send_data_reply should target the requested IP address");
  ASSERT_TRUE(send_capture.data.datarep.requestHi == 0x00 &&
              send_capture.data.datarep.requestLo == 0x01,
              "artnet_send_data_reply should encode the selected request code");
  ASSERT_TRUE(send_capture.data.datarep.payLenHi == 0x00 &&
              send_capture.data.datarep.payLenLo == (uint8_t)payload_len,
              "artnet_send_data_reply should encode the payload length");
  ASSERT_TRUE(memcmp(send_capture.data.datarep.payLoad, payload, (size_t)payload_len) == 0,
              "artnet_send_data_reply should copy the payload bytes");

  stop_sendable_node(&n);
}

static void test_send_data_reply_rejects_invalid_payload_arguments(void) {
  artnet_node_t n;

  init_test_node(&n);
  start_sendable_node(&n);

  ASSERT_TRUE(artnet_send_data_reply((artnet_node)&n,
                                     "192.168.1.56",
                                     ARTNET_DR_URL_PRODUCT,
                                     NULL,
                                     1) == ARTNET_EARG,
              "artnet_send_data_reply should reject a NULL non-empty payload");
  ASSERT_TRUE(artnet_send_data_reply((artnet_node)&n,
                                     "192.168.1.56",
                                     ARTNET_DR_URL_PRODUCT,
                                     "",
                                     -1) == ARTNET_EARG,
              "artnet_send_data_reply should reject a negative payload length");

  stop_sendable_node(&n);
}

static void test_send_address_accepts_null_fields_as_no_change(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  uint8_t nochange[ARTNET_MAX_PORTS];
  node_entry_private_t *entry = NULL;

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  memset(nochange, PROGRAM_NO_CHANGE, sizeof(nochange));
  entry = add_stub_node_entry(&n, "192.168.1.60", 1, 2, 3);

  ASSERT_TRUE(artnet_send_address((artnet_node)&n,
                                  &entry->pub,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  (uint8_t)ARTNET_ADDRESS_NO_CHANGE,
                                  (uint8_t)ARTNET_ADDRESS_NO_CHANGE,
                                  ARTNET_PC_NONE,
                                  0xFF) == ARTNET_EOK,
              "artnet_send_address should treat NULL fields as no-change sentinels");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_send_address should emit one outbound packet");
  ASSERT_TRUE(send_capture.type == ARTNET_ADDRESS,
              "artnet_send_address should send an ArtAddress packet");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.60").s_addr,
              "artnet_send_address should target the selected node");
  ASSERT_TRUE(send_capture.data.addr.shortName[0] == PROGRAM_NO_CHANGE,
              "artnet_send_address should encode NULL short name as no-change");
  ASSERT_TRUE(send_capture.data.addr.longName[0] == PROGRAM_NO_CHANGE,
              "artnet_send_address should encode NULL long name as no-change");
  ASSERT_TRUE(memcmp(send_capture.data.addr.swIn, nochange, sizeof(nochange)) == 0,
              "artnet_send_address should encode NULL input addresses as no-change");
  ASSERT_TRUE(memcmp(send_capture.data.addr.swOut, nochange, sizeof(nochange)) == 0,
              "artnet_send_address should encode NULL output addresses as no-change");
  ASSERT_TRUE(send_capture.data.addr.netSwitch == PROGRAM_NO_CHANGE &&
              send_capture.data.addr.subSwitch == PROGRAM_NO_CHANGE,
              "artnet_send_address should preserve no-change net and subnet sentinels");
  ASSERT_TRUE(send_capture.data.addr.bindIndex == 1,
              "artnet_send_address should encode the target node BindIndex");

  stop_sendable_node(&n);
}

static void test_send_ipprog_encodes_programming_fields(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  node_entry_private_t *entry = NULL;

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  entry = add_stub_node_entry(&n, "192.168.1.61", 1, 2, 3);

  ASSERT_TRUE(artnet_send_ipprog((artnet_node)&n,
                                 &entry->pub,
                                 0x96,
                                 "10.77.66.55",
                                 "255.255.254.0",
                                 "10.77.66.1") == ARTNET_EOK,
              "artnet_send_ipprog should succeed for valid programming parameters");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_send_ipprog should emit one outbound packet");
  ASSERT_TRUE(send_capture.type == ARTNET_IPPROG,
              "artnet_send_ipprog should send an ArtIpProg packet");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.61").s_addr,
              "artnet_send_ipprog should target the selected node");
  ASSERT_TRUE(send_capture.data.aip.Command == 0x96,
              "artnet_send_ipprog should preserve the command bitfield");
  ASSERT_TRUE(send_capture.data.aip.ProgIpHi == 10 &&
              send_capture.data.aip.ProgIp2 == 77 &&
              send_capture.data.aip.ProgIp1 == 66 &&
              send_capture.data.aip.ProgIpLo == 55,
              "artnet_send_ipprog should encode the requested IP address");
  ASSERT_TRUE(send_capture.data.aip.ProgSmHi == 255 &&
              send_capture.data.aip.ProgSm2 == 255 &&
              send_capture.data.aip.ProgSm1 == 254 &&
              send_capture.data.aip.ProgSmLo == 0,
              "artnet_send_ipprog should encode the requested subnet mask");
  ASSERT_TRUE(send_capture.data.aip.ProgDgHi == 10 &&
              send_capture.data.aip.ProgDg2 == 77 &&
              send_capture.data.aip.ProgDg1 == 66 &&
              send_capture.data.aip.ProgDgLo == 1,
              "artnet_send_ipprog should encode the requested gateway");

  stop_sendable_node(&n);
}

static void test_send_command_encodes_text_and_target(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  const char text[] = "SwoutText=Node";

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  ASSERT_TRUE(artnet_send_command((artnet_node)&n,
                                  0x1234,
                                  text,
                                  (int16_t)sizeof(text),
                                  "192.168.1.62") == ARTNET_EOK,
              "artnet_send_command should succeed for a valid target and payload");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_send_command should emit one outbound packet");
  ASSERT_TRUE(send_capture.type == ARTNET_COMMAND,
              "artnet_send_command should send an ArtCommand packet");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.62").s_addr,
              "artnet_send_command should target the requested IP address");
  ASSERT_TRUE(send_capture.data.cmd.estaManHi == 0x12 &&
              send_capture.data.cmd.estaManLo == 0x34,
              "artnet_send_command should encode the ESTA code");
  ASSERT_TRUE(send_capture.data.cmd.lengthHi == 0x00 &&
              send_capture.data.cmd.lengthLo == (uint8_t)sizeof(text),
              "artnet_send_command should encode the payload length");
  ASSERT_TRUE(memcmp(send_capture.data.cmd.data, text, sizeof(text)) == 0,
              "artnet_send_command should copy the text payload");

  stop_sendable_node(&n);
}

static void test_send_media_packets_encode_payloads_and_targets(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  node_entry_private_t *entry = NULL;
  uint8_t patch_data[3] = {1, 2, 3};
  uint8_t control_data[4] = {4, 5, 6, 7};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  entry = add_stub_node_entry(&n, "192.168.1.63", 1, 2, 3);

  ASSERT_TRUE(artnet_send_media_patch((artnet_node)&n,
                                      &entry->pub,
                                      2,
                                      make_addr(1, 2, 3),
                                      patch_data,
                                      3) == ARTNET_EOK,
              "artnet_send_media_patch should succeed for a valid target and payload");
  ASSERT_TRUE(send_capture.called == 1 && send_capture.type == ARTNET_MEDIAPATCH,
              "artnet_send_media_patch should emit an ArtMediaPatch packet");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.63").s_addr,
              "artnet_send_media_patch should target the selected node");
  ASSERT_TRUE(send_capture.data.mpatch.physical == 2,
              "artnet_send_media_patch should encode the physical port");
  ASSERT_TRUE(send_capture.data.mpatch.universe == htols(make_addr(1, 2, 3)),
              "artnet_send_media_patch should encode the universe");
  ASSERT_TRUE(memcmp(send_capture.data.mpatch.data, patch_data, sizeof(patch_data)) == 0,
              "artnet_send_media_patch should copy the payload bytes");

  send_capture.called = 0;
  ASSERT_TRUE(artnet_send_media_control((artnet_node)&n,
                                        &entry->pub,
                                        control_data,
                                        4) == ARTNET_EOK,
              "artnet_send_media_control should succeed for a valid target and payload");
  ASSERT_TRUE(send_capture.called == 1 && send_capture.type == ARTNET_MEDIACONTROL,
              "artnet_send_media_control should emit an ArtMediaControl packet");
  ASSERT_TRUE(memcmp(send_capture.data.mctrl.data, control_data, sizeof(control_data)) == 0,
              "artnet_send_media_control should copy the payload bytes");

  send_capture.called = 0;
  ASSERT_TRUE(artnet_send_media_control_reply((artnet_node)&n,
                                              &entry->pub,
                                              control_data,
                                              4) == ARTNET_EOK,
              "artnet_send_media_control_reply should succeed for a valid target and payload");
  ASSERT_TRUE(send_capture.called == 1 && send_capture.type == ARTNET_MEDIACONTROLREPLY,
              "artnet_send_media_control_reply should emit an ArtMediaControlReply packet");
  ASSERT_TRUE(memcmp(send_capture.data.mctrl.data, control_data, sizeof(control_data)) == 0,
              "artnet_send_media_control_reply should copy the payload bytes");

  stop_sendable_node(&n);
}

static void test_send_poll_flags_encodes_explicit_artnet4_fields(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  ASSERT_TRUE(artnet_send_poll_flags((artnet_node)&n,
                                     "192.168.1.64",
                                     (uint8_t)(ARTNET_POLL_FLAG_REPLY_ON_CHANGE |
                                               ARTNET_POLL_FLAG_DIAG_ENABLE |
                                               ARTNET_POLL_FLAG_TARGET_MODE),
                                     ARTNET_DIAG_HIGH,
                                     0x1234,
                                     0x1000,
                                     0x1122,
                                     0x3344) == ARTNET_EOK,
              "artnet_send_poll_flags should succeed for a valid target");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_send_poll_flags should emit one outbound packet");
  ASSERT_TRUE(send_capture.type == ARTNET_POLL,
              "artnet_send_poll_flags should send an ArtPoll packet");
  ASSERT_TRUE(send_capture.to.s_addr == ip4("192.168.1.64").s_addr,
              "artnet_send_poll_flags should target the requested IP address");
  ASSERT_TRUE(send_capture.data.ap.flags ==
              (ARTNET_POLL_FLAG_REPLY_ON_CHANGE |
               ARTNET_POLL_FLAG_DIAG_ENABLE |
               ARTNET_POLL_FLAG_TARGET_MODE),
              "artnet_send_poll_flags should preserve the explicit flags");
  ASSERT_TRUE(send_capture.data.ap.diagPriority == ARTNET_DIAG_HIGH,
              "artnet_send_poll_flags should encode the diagnostic priority");
  ASSERT_TRUE(send_capture.data.ap.targetPortAddressTopHi == 0x12 &&
              send_capture.data.ap.targetPortAddressTopLo == 0x34 &&
              send_capture.data.ap.targetPortAddressBottomHi == 0x10 &&
              send_capture.data.ap.targetPortAddressBottomLo == 0x00,
              "artnet_send_poll_flags should encode the target Port-Address range");
  ASSERT_TRUE(send_capture.data.ap.estaMan[0] == 0x11 &&
              send_capture.data.ap.estaMan[1] == 0x22 &&
              send_capture.data.ap.oem[0] == 0x33 &&
              send_capture.data.ap.oem[1] == 0x44,
              "artnet_send_poll_flags should encode explicit ESTA and OEM values");

  stop_sendable_node(&n);
}

static void test_sync_only_accepts_matching_last_dmx_source(void) {
  artnet_node_t n;
  artnet_packet_t sync_packet;

  init_test_node(&n);
  init_packet(&sync_packet, ARTNET_SYNC, ip4("10.9.9.9"));

  n.state.sync_mode = 0;
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(1, 2, 3);
  n.ports.out[0].last_dmx_source = ip4("10.8.8.8");
  handle_sync(&n, &sync_packet);
  ASSERT_TRUE(n.state.sync_mode == 0,
              "handle_sync should ignore ArtSync from a different source than the last ArtDmx");

  sync_packet.from = ip4("10.8.8.8");
  handle_sync(&n, &sync_packet);
  ASSERT_TRUE(n.state.sync_mode == 1,
              "handle_sync should accept ArtSync from the same source as the last ArtDmx");
}

static void test_sync_buffers_dmx_until_sync_flush(void) {
  artnet_node_t n;
  artnet_packet_t dmx_packet;
  artnet_packet_t sync_packet;
  rdm_init_capture_t dmx_capture = {0};

  init_test_node(&n);
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(1, 2, 3);
  n.state.sync_mode = 1;
  n.callbacks.dmx_c.fh = rdm_init_handler;
  n.callbacks.dmx_c.data = &dmx_capture;

  init_packet(&dmx_packet, ARTNET_DMX, ip4("10.8.8.8"));
  dmx_packet.length = (int)(sizeof(artnet_dmx_t) - ARTNET_DMX_LENGTH + 3);
  dmx_packet.data.admx.universe = htols(make_addr(1, 2, 3));
  dmx_packet.data.admx.lengthHi = 0;
  dmx_packet.data.admx.length = 3;
  dmx_packet.data.admx.data[0] = 11;
  dmx_packet.data.admx.data[1] = 22;
  dmx_packet.data.admx.data[2] = 33;

  handle_dmx(&n, &dmx_packet);

  ASSERT_TRUE(n.ports.out[0].sync_pending == TRUE,
              "handle_dmx should buffer output data while sync mode is active");
  ASSERT_TRUE(n.ports.out[0].length == 0,
              "buffered ArtDmx should not update live output length before ArtSync");
  ASSERT_TRUE(dmx_capture.called == 0,
              "buffered ArtDmx should not trigger the DMX callback before ArtSync");

  init_packet(&sync_packet, ARTNET_SYNC, ip4("10.8.8.8"));
  handle_sync(&n, &sync_packet);

  ASSERT_TRUE(n.ports.out[0].sync_pending == FALSE,
              "handle_sync should flush buffered ArtDmx data");
  ASSERT_TRUE(n.ports.out[0].length == 3,
              "handle_sync should commit buffered ArtDmx length");
  ASSERT_TRUE(n.ports.out[0].data[0] == 11 &&
              n.ports.out[0].data[1] == 22 &&
              n.ports.out[0].data[2] == 33,
              "handle_sync should commit buffered ArtDmx payload");
  ASSERT_TRUE(dmx_capture.called == 1 && dmx_capture.port == 0,
              "handle_sync should trigger the DMX callback when flushing buffered data");
}

static void test_sync_flushes_same_ip_different_physical_merge(void) {
  artnet_node_t n;
  artnet_packet_t p1;
  artnet_packet_t p2;
  artnet_packet_t sync_packet;
  rdm_init_capture_t dmx_capture = {0};

  init_test_node(&n);
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(1, 2, 3);
  n.ports.out[0].merge_mode = ARTNET_MERGE_LTP;
  n.state.sync_mode = 1;
  n.state.last_dmx_source = ip4("10.40.40.1");
  n.callbacks.dmx_c.fh = rdm_init_handler;
  n.callbacks.dmx_c.data = &dmx_capture;

  init_packet(&p1, ARTNET_DMX, ip4("10.40.40.1"));
  p1.data.admx.universe = htols(make_addr(1, 2, 3));
  p1.data.admx.lengthHi = 0;
  p1.data.admx.length = 2;
  p1.data.admx.physical = 1;
  p1.data.admx.data[0] = 10;
  p1.data.admx.data[1] = 20;

  init_packet(&p2, ARTNET_DMX, ip4("10.40.40.1"));
  p2.data.admx.universe = htols(make_addr(1, 2, 3));
  p2.data.admx.lengthHi = 0;
  p2.data.admx.length = 2;
  p2.data.admx.physical = 2;
  p2.data.admx.data[0] = 30;
  p2.data.admx.data[1] = 40;

  handle_dmx(&n, &p1);
  handle_dmx(&n, &p2);
  ASSERT_TRUE((n.ports.out[0].port_status & PORT_STATUS_MERGE) != 0,
              "setup should enter merge mode for same-IP different-Physical sources");
  ASSERT_TRUE(n.ports.out[0].sync_pending == TRUE,
              "merged output should remain buffered while sync mode is active");

  init_packet(&sync_packet, ARTNET_SYNC, ip4("10.40.40.1"));
  handle_sync(&n, &sync_packet);

  ASSERT_TRUE(n.ports.out[0].sync_pending == FALSE,
              "ArtSync should flush buffered data for same-IP different-Physical merge");
  ASSERT_TRUE(n.ports.out[0].data[0] == 30 && n.ports.out[0].data[1] == 40,
              "same-IP different-Physical merge should still flush the merged/latest frame");
  ASSERT_TRUE(dmx_capture.called == 1 && dmx_capture.port == 0,
              "ArtSync should still trigger the DMX callback for same-IP different-Physical merge");
}

static void test_rdm_request_updates_reply_target_and_callback_payload(void) {
  artnet_node_t n;
  artnet_packet_t p;
  rdm_capture_t capture = {0};
  uint8_t payload[3] = {0xCC, 0x01, 0x02};
  struct in_addr requester = ip4("127.0.0.140");

  init_test_node(&n);
  n.callbacks.rdm_c.fh = rdm_data_handler;
  n.callbacks.rdm_c.data = &capture;

  init_packet(&p, ARTNET_RDM, requester);
  p.length = (int)(sizeof(artnet_rdm_t) - ARTNET_MAX_RDM_DATA + sizeof(payload));
  p.data.rdm.net = 0x01;
  p.data.rdm.address = 0x23;
  memcpy(p.data.rdm.data, payload, sizeof(payload));

  handle_rdm(&n, &p);

  ASSERT_TRUE(n.state.rdm_reply_addr.s_addr == requester.s_addr,
              "handle_rdm should store requester IP for unicast replies");
  ASSERT_TRUE(capture.called == 1,
              "handle_rdm should invoke the RDM callback once");
  ASSERT_TRUE(capture.address == make_addr(0x01, 0x02, 0x03),
              "handle_rdm should decode the full 15-bit universe address");
  ASSERT_TRUE(capture.length == (int)sizeof(payload),
              "handle_rdm should pass through the exact RDM payload length");
  ASSERT_TRUE(memcmp(capture.data, payload, sizeof(payload)) == 0,
              "handle_rdm should pass through the exact RDM payload");
}

static void test_send_rdm_and_rdmsub_unicast_to_last_requester(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  uint8_t payload[2] = {0x11, 0x22};
  uint8_t uid[ARTNET_RDM_UID_WIDTH] = {1, 2, 3, 4, 5, 6};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.state.rdm_reply_addr = ip4("127.0.0.141");

  ASSERT_TRUE(artnet_send_rdm((artnet_node)&n, make_addr(0x01, 0x02, 0x03), payload, 2) == ARTNET_EOK,
              "artnet_send_rdm should succeed when reply target is known");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_RDM &&
              send_capture.to.s_addr == ip4("127.0.0.141").s_addr,
              "artnet_send_rdm should unicast to the stored requester");

  send_capture.called = 0;
  ASSERT_TRUE(artnet_send_rdmsub((artnet_node)&n, uid, 0x20, 0x1234, 0x0001, 0x0002, payload, 2) == ARTNET_EOK,
              "artnet_send_rdmsub should succeed when reply target is known");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_RDMSUB &&
              send_capture.to.s_addr == ip4("127.0.0.141").s_addr,
              "artnet_send_rdmsub should unicast to the stored requester");
  ASSERT_TRUE(memcmp(send_capture.data.rdmsub.uid, uid, ARTNET_RDM_UID_WIDTH) == 0,
              "artnet_send_rdmsub should preserve the target UID");

  stop_sendable_node(&n);
}

static void test_send_rdm_and_rdmsub_reject_missing_target_and_bad_payloads(void) {
  artnet_node_t n;
  uint8_t payload[2] = {0x11, 0x22};
  uint8_t uid[ARTNET_RDM_UID_WIDTH] = {1, 2, 3, 4, 5, 6};

  init_test_node(&n);
  start_sendable_node(&n);

  ASSERT_TRUE(artnet_send_rdm((artnet_node)&n, make_addr(0x01, 0x02, 0x03), payload, 2) == ARTNET_EACTION,
              "artnet_send_rdm should reject sends before a requester target is known");
  ASSERT_TRUE(artnet_send_rdmsub((artnet_node)&n, uid, 0x20, 0x1234, 0x0001, 0x0002, payload, 2) == ARTNET_EACTION,
              "artnet_send_rdmsub should reject sends before a requester target is known");

  n.state.rdm_reply_addr = ip4("127.0.0.141");

  ASSERT_TRUE(artnet_send_rdm((artnet_node)&n, make_addr(0x01, 0x02, 0x03), NULL, 1) == ARTNET_EARG,
              "artnet_send_rdm should reject a NULL non-empty payload");
  ASSERT_TRUE(artnet_send_rdm((artnet_node)&n, make_addr(0x01, 0x02, 0x03), payload, -1) == ARTNET_EARG,
              "artnet_send_rdm should reject negative payload lengths");
  ASSERT_TRUE(artnet_send_rdmsub((artnet_node)&n, NULL, 0x20, 0x1234, 0x0001, 0x0002, payload, 2) == ARTNET_EARG,
              "artnet_send_rdmsub should reject a NULL UID");
  ASSERT_TRUE(artnet_send_rdmsub((artnet_node)&n, uid, 0x20, 0x1234, 0x0001, 0, payload, 2) == ARTNET_EARG,
              "artnet_send_rdmsub should reject a zero SubCount");
  ASSERT_TRUE(artnet_send_rdmsub((artnet_node)&n, uid, 0x20, 0x1234, 0x0001, 0x0002, NULL, 1) == ARTNET_EARG,
              "artnet_send_rdmsub should reject a NULL non-empty payload");
  ASSERT_TRUE(artnet_send_rdmsub((artnet_node)&n, uid, 0x20, 0x1234, 0x0001, 0x0002, payload, -1) == ARTNET_EARG,
              "artnet_send_rdmsub should reject negative payload lengths");

  stop_sendable_node(&n);
}

static void test_rdm_sub_updates_reply_target(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t rdm_capture = {0};
  simple_capture_t rdmsub_capture = {0};
  struct in_addr requester = ip4("127.0.0.142");

  init_test_node(&n);
  n.callbacks.rdm.fh = simple_packet_handler;
  n.callbacks.rdm.data = &rdm_capture;
  n.callbacks.rdmsub.fh = simple_packet_handler;
  n.callbacks.rdmsub.data = &rdmsub_capture;

  init_packet(&p, ARTNET_RDMSUB, requester);
  handle_rdm_sub(&n, &p);

  ASSERT_TRUE(n.state.rdm_reply_addr.s_addr == requester.s_addr,
              "handle_rdm_sub should store requester IP for compressed RDM replies");
  ASSERT_TRUE(rdmsub_capture.called == 1,
              "handle_rdm_sub should invoke the ArtRdmSub packet callback");
  ASSERT_TRUE(rdm_capture.called == 0,
              "handle_rdm_sub should not reuse the ArtRdm packet callback");
}

static void test_failsafe_zero_full_and_scene_modes(void) {
  artnet_node_t n;

  init_test_node(&n);
  n.state.mode = ARTNET_ON;
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].last_dmx_time = 1;
  n.ports.out[0].length = 4;
  n.ports.out[0].data[0] = 10;
  n.ports.out[0].data[1] = 20;
  n.ports.out[0].data[2] = 30;
  n.ports.out[0].data[3] = 40;

  n.state.failsafe_mode = ARTNET_FAILSAFE_ZERO;
  check_timeouts(&n);
  ASSERT_TRUE(n.ports.out[0].data[0] == 0 && n.ports.out[0].data[1] == 0,
              "failsafe ZERO should clear output data");

  n.ports.out[0].failsafe_triggered = FALSE;
  n.ports.out[0].last_dmx_time = 1;
  n.state.failsafe_mode = ARTNET_FAILSAFE_FULL;
  check_timeouts(&n);
  ASSERT_TRUE(n.ports.out[0].data[0] == 0xFF && n.ports.out[0].data[1] == 0xFF,
              "failsafe FULL should drive output data to 0xFF");

  n.ports.out[0].failsafe_triggered = FALSE;
  n.ports.out[0].last_dmx_time = 1;
  n.ports.out[0].failsafe_length = 4;
  n.ports.out[0].failsafe_data[0] = 1;
  n.ports.out[0].failsafe_data[1] = 2;
  n.ports.out[0].failsafe_data[2] = 3;
  n.ports.out[0].failsafe_data[3] = 4;
  n.state.failsafe_mode = ARTNET_FAILSAFE_SCENE;
  check_timeouts(&n);
  ASSERT_TRUE(n.ports.out[0].data[0] == 1 &&
              n.ports.out[0].data[1] == 2 &&
              n.ports.out[0].data[2] == 3 &&
              n.ports.out[0].data[3] == 4,
              "failsafe SCENE should restore the recorded scene data");
}

static void test_dmx_merge_htp_ltp_and_timeout_cleanup(void) {
  artnet_node_t n;
  artnet_packet_t p1;
  artnet_packet_t p2;

  init_test_node(&n);
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(0x01, 0x02, 0x03);

  init_packet(&p1, ARTNET_DMX, ip4("10.10.10.1"));
  p1.data.admx.universe = htols(make_addr(0x01, 0x02, 0x03));
  p1.data.admx.lengthHi = 0;
  p1.data.admx.length = 2;
  p1.data.admx.data[0] = 10;
  p1.data.admx.data[1] = 50;

  init_packet(&p2, ARTNET_DMX, ip4("10.10.10.2"));
  p2.data.admx.universe = htols(make_addr(0x01, 0x02, 0x03));
  p2.data.admx.lengthHi = 0;
  p2.data.admx.length = 2;
  p2.data.admx.data[0] = 20;
  p2.data.admx.data[1] = 40;

  n.ports.out[0].merge_mode = ARTNET_MERGE_HTP;
  handle_dmx(&n, &p1);
  handle_dmx(&n, &p2);
  ASSERT_TRUE((n.ports.out[0].port_status & PORT_STATUS_MERGE) != 0,
              "receiving DMX from two sources should enter merge mode");
  ASSERT_TRUE(n.ports.out[0].data[0] == 20 && n.ports.out[0].data[1] == 50,
              "HTP merge should keep the highest value per channel");

  n.ports.out[0].merge_mode = ARTNET_MERGE_LTP;
  handle_dmx(&n, &p1);
  ASSERT_TRUE(n.ports.out[0].data[0] == 10 && n.ports.out[0].data[1] == 50,
              "LTP merge should replace output with the latest source data");

  n.ports.out[0].timeA = 0;
  n.ports.out[0].timeB = 0;
  check_merge_timeouts(&n, 0);
  ASSERT_TRUE((n.ports.out[0].port_status & PORT_STATUS_MERGE) == 0,
              "merge timeout should clear merge mode when sources expire");
}

static void test_dmx_merge_detects_same_ip_different_physical(void) {
  artnet_node_t n;
  artnet_packet_t p1;
  artnet_packet_t p2;

  init_test_node(&n);
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(0x01, 0x02, 0x03);
  n.ports.out[0].merge_mode = ARTNET_MERGE_HTP;

  init_packet(&p1, ARTNET_DMX, ip4("10.20.30.1"));
  p1.data.admx.universe = htols(make_addr(0x01, 0x02, 0x03));
  p1.data.admx.lengthHi = 0;
  p1.data.admx.length = 2;
  p1.data.admx.physical = 1;
  p1.data.admx.data[0] = 10;
  p1.data.admx.data[1] = 40;

  init_packet(&p2, ARTNET_DMX, ip4("10.20.30.1"));
  p2.data.admx.universe = htols(make_addr(0x01, 0x02, 0x03));
  p2.data.admx.lengthHi = 0;
  p2.data.admx.length = 2;
  p2.data.admx.physical = 2;
  p2.data.admx.data[0] = 30;
  p2.data.admx.data[1] = 20;

  handle_dmx(&n, &p1);
  handle_dmx(&n, &p2);

  ASSERT_TRUE((n.ports.out[0].port_status & PORT_STATUS_MERGE) != 0,
              "ArtDmx from the same IP but different Physical should enter merge mode");
  ASSERT_TRUE(n.ports.out[0].data[0] == 30 && n.ports.out[0].data[1] == 40,
              "same-IP different-Physical merge should still apply the selected merge mode");
}

static void test_address_cancel_merge_ends_merge_on_next_dmx(void) {
  artnet_node_t n;
  artnet_packet_t p1;
  artnet_packet_t p2;
  artnet_packet_t addr;

  init_test_node(&n);
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(0x01, 0x02, 0x03);
  n.ports.out[0].merge_mode = ARTNET_MERGE_HTP;

  init_packet(&p1, ARTNET_DMX, ip4("10.30.30.1"));
  p1.data.admx.universe = htols(make_addr(0x01, 0x02, 0x03));
  p1.data.admx.lengthHi = 0;
  p1.data.admx.length = 2;
  p1.data.admx.data[0] = 10;
  p1.data.admx.data[1] = 50;

  init_packet(&p2, ARTNET_DMX, ip4("10.30.30.2"));
  p2.data.admx.universe = htols(make_addr(0x01, 0x02, 0x03));
  p2.data.admx.lengthHi = 0;
  p2.data.admx.length = 2;
  p2.data.admx.data[0] = 20;
  p2.data.admx.data[1] = 40;

  handle_dmx(&n, &p1);
  handle_dmx(&n, &p2);
  ASSERT_TRUE((n.ports.out[0].port_status & PORT_STATUS_MERGE) != 0,
              "setup should enter merge mode before cancel");

  init_packet(&addr, ARTNET_ADDRESS, ip4("127.0.0.240"));
  memset(addr.data.addr.shortName, PROGRAM_NO_CHANGE, ARTNET_SHORT_NAME_LENGTH);
  memset(addr.data.addr.longName, PROGRAM_NO_CHANGE, ARTNET_LONG_NAME_LENGTH);
  memset(addr.data.addr.swIn, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  memset(addr.data.addr.swOut, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  addr.data.addr.bindIndex = 1;
  addr.data.addr.netSwitch = PROGRAM_NO_CHANGE;
  addr.data.addr.subSwitch = PROGRAM_NO_CHANGE;
  addr.data.addr.acnPriority = 0xFF;
  addr.data.addr.command = ARTNET_PC_CANCEL;

  handle_address(&n, &addr);
  ASSERT_TRUE(n.ports.out[0].cancel_merge_pending == TRUE,
              "AcCancelMerge should arm merge cancellation until the next ArtDmx");

  p1.data.admx.data[0] = 60;
  p1.data.admx.data[1] = 70;
  handle_dmx(&n, &p1);

  ASSERT_TRUE((n.ports.out[0].port_status & PORT_STATUS_MERGE) == 0,
              "the next ArtDmx after AcCancelMerge should end merge mode");
  ASSERT_TRUE(n.ports.out[0].cancel_merge_pending == FALSE,
              "merge cancel flag should clear after the terminating ArtDmx");
  ASSERT_TRUE(n.ports.out[0].ipA.s_addr == p1.from.s_addr &&
              n.ports.out[0].ipB.s_addr == 0,
              "after AcCancelMerge, the terminating ArtDmx source should become the sole active source");
}

static void test_firmware_single_block_upload_completes_and_replies_allgood(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  firmware_capture_t fw_capture = {0};
  uint16_t words[4] = {0x1111, 0x2222, 0x3333, 0x4444};
  int total_words = (int)(sizeof(words) / sizeof(words[0]));

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.callbacks.firmware_c.fh = firmware_data_handler;
  n.callbacks.firmware_c.data = &fw_capture;

  init_packet(&p, ARTNET_FIRMWAREMASTER, ip4("127.0.0.150"));
  p.data.firmware.type = ARTNET_FIRMWARE_FIRMFIRST;
  artnet_misc_int_to_bytes(total_words, p.data.firmware.length);
  memcpy(p.data.firmware.data, words, sizeof(words));

  ASSERT_TRUE(handle_firmware(&n, &p) == ARTNET_EOK,
              "single-block firmware upload should succeed");
  ASSERT_TRUE(fw_capture.called == 1,
              "single-block firmware upload should invoke the firmware callback");
  ASSERT_TRUE(fw_capture.length == total_words,
              "single-block firmware callback should receive the full word count");
  ASSERT_TRUE(memcmp(fw_capture.data, words, sizeof(words)) == 0,
              "single-block firmware callback should receive the uploaded words");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_FIRMWAREREPLY &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_ALLGOOD,
              "single-block firmware upload should reply with ALLGOOD");

  stop_sendable_node(&n);
}

static void test_firmware_first_block_with_zero_length_fails(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  init_packet(&p, ARTNET_FIRMWAREMASTER, ip4("127.0.0.149"));
  p.data.firmware.type = ARTNET_FIRMWARE_FIRMFIRST;
  artnet_misc_int_to_bytes(0, p.data.firmware.length);

  ASSERT_TRUE(handle_firmware(&n, &p) == ARTNET_EOK,
              "zero-length firmware first block should be handled");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_FIRMWAREREPLY &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_FAIL,
              "zero-length firmware first block should reply with FAIL");

  stop_sendable_node(&n);
}

static void test_firmware_multi_block_upload_completes_after_last_block(void) {
  artnet_node_t n;
  artnet_packet_t first;
  artnet_packet_t last;
  send_capture_t send_capture = {0};
  firmware_capture_t fw_capture = {0};
  uint16_t words[514];
  int i = 0;

  for (i = 0; i < 514; i++) {
    words[i] = (uint16_t)(0x1000 + i);
  }

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.callbacks.firmware_c.fh = firmware_data_handler;
  n.callbacks.firmware_c.data = &fw_capture;

  init_packet(&first, ARTNET_FIRMWAREMASTER, ip4("127.0.0.151"));
  first.data.firmware.type = ARTNET_FIRMWARE_FIRMFIRST;
  artnet_misc_int_to_bytes(514, first.data.firmware.length);
  memcpy(first.data.firmware.data, words, ARTNET_FIRMWARE_SIZE * sizeof(uint16_t));

  ASSERT_TRUE(handle_firmware(&n, &first) == ARTNET_EOK,
              "first block of multi-block firmware upload should succeed");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_BLOCKGOOD,
              "first block of multi-block firmware upload should reply BLOCKGOOD");
  ASSERT_TRUE(n.firmware.bytes_current == ARTNET_FIRMWARE_SIZE * (int)sizeof(uint16_t),
              "first block should advance byte counter by one block");

  send_capture.called = 0;
  init_packet(&last, ARTNET_FIRMWAREMASTER, ip4("127.0.0.151"));
  last.data.firmware.type = ARTNET_FIRMWARE_FIRMLAST;
  last.data.firmware.blockId = 1;
  artnet_misc_int_to_bytes(514, last.data.firmware.length);
  memcpy(last.data.firmware.data, &words[ARTNET_FIRMWARE_SIZE], 2 * sizeof(uint16_t));

  ASSERT_TRUE(handle_firmware(&n, &last) == ARTNET_EOK,
              "last block of multi-block firmware upload should succeed");
  ASSERT_TRUE(fw_capture.called == 1,
              "multi-block firmware upload should invoke the firmware callback once at completion");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_ALLGOOD,
              "last block of multi-block firmware upload should reply ALLGOOD");
  ASSERT_TRUE(n.firmware.data == NULL && n.firmware.peer.s_addr == 0,
              "multi-block firmware upload should reset transfer state after completion");

  stop_sendable_node(&n);
}

static void test_firmware_reply_blockgood_sends_next_packet(void) {
  artnet_node_t n;
  artnet_packet_t reply;
  send_capture_t send_capture = {0};
  firmware_status_capture_t status_capture = {0};
  node_entry_private_t *entry = NULL;
  uint16_t words[600];
  int i = 0;

  for (i = 0; i < 600; i++) {
    words[i] = (uint16_t)(i + 1);
  }

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  entry = add_stub_node_entry(&n, "127.0.0.152", 1, 2, 3);
  entry->firmware.data = (uint16_t *)malloc(sizeof(words));
  memcpy(entry->firmware.data, words, sizeof(words));
  entry->firmware.bytes_total = (int)sizeof(words);
  entry->firmware.bytes_current = ARTNET_FIRMWARE_SIZE * (int)sizeof(uint16_t);
  entry->firmware.peer = ip4("127.0.0.152");
  entry->firmware.expected_block = 1;
  entry->firmware.callback = firmware_status_handler;
  entry->firmware.user_data = &status_capture;

  init_packet(&reply, ARTNET_FIRMWAREREPLY, ip4("127.0.0.152"));
  reply.data.firmwarer.type = ARTNET_FIRMWARE_BLOCKGOOD;

  ASSERT_TRUE(handle_firmware_reply(&n, &reply) == ARTNET_EOK,
              "firmware BLOCKGOOD reply should trigger sending the next packet");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_FIRMWAREMASTER,
              "firmware BLOCKGOOD reply should send the next ArtFirmwareMaster packet");
  ASSERT_TRUE(entry->firmware.bytes_current > ARTNET_FIRMWARE_SIZE * (int)sizeof(uint16_t),
              "sending next firmware packet should advance the transfer byte counter");

  stop_sendable_node(&n);
}

static void test_node_list_update_and_timeout_cleanup(void) {
  artnet_node_t n;
  artnet_packet_t reply;
  artnet_node_list nl = NULL;
  artnet_node_entry first = NULL;
  node_entry_private_t *first_private = NULL;

  init_test_node(&n);
  n.state.mode = ARTNET_ON;
  nl = &n.node_list;

  init_packet(&reply, ARTNET_REPLY, ip4("127.0.0.160"));
  reply.data.ar.netSwitch = 1;
  reply.data.ar.subSwitch = 2;
  reply.data.ar.numbports = 1;
  reply.data.ar.swOut[0] = 3;
  memcpy(reply.data.ar.shortName, "Node One", 9);
  memcpy(reply.data.ar.ip, &reply.from.s_addr, 4);

  ASSERT_TRUE(artnet_nl_update(&n, &n.node_list, &reply) == ARTNET_EOK,
              "artnet_nl_update should add a new node entry");
  ASSERT_TRUE(artnet_nl_get_length(nl) == 1,
              "node list should contain one entry after first reply");

  first = artnet_nl_first(nl);
  first_private = n.node_list.first;
  ASSERT_TRUE(first != NULL &&
              first->netSwitch == 1 &&
              first->subSwitch == 2 &&
              first->swOut[0] == 3,
              "node list entry should reflect ArtPollReply addressing fields");

  memcpy(reply.data.ar.shortName, "Node One Updated", 17);
  ASSERT_TRUE(artnet_nl_update(&n, &n.node_list, &reply) == ARTNET_EOK,
              "artnet_nl_update should update an existing node entry");
  ASSERT_TRUE(n.node_list.first == first_private,
              "updating an existing node should keep the same linked-list entry");
  ASSERT_TRUE(artnet_nl_get_length(nl) == 1,
              "updating an existing node should not grow the node list");
  first = artnet_nl_first(nl);
  ASSERT_TRUE(first != NULL &&
              memcmp(first->shortName, "Node One Updated", 17) == 0,
              "updating an existing node should refresh node metadata");

  if (n.node_list.first) {
    n.node_list.first->last_seen = 0;
  }
  check_timeouts(&n);
  ASSERT_TRUE(artnet_nl_get_length(nl) == 0,
              "check_timeouts should remove stale node list entries");
}

static void test_node_list_same_ip_and_same_first_port_updates_same_entry(void) {
  artnet_node_t n;
  artnet_packet_t reply;
  node_entry_private_t *first_private = NULL;

  init_test_node(&n);
  n.state.mode = ARTNET_ON;

  init_packet(&reply, ARTNET_REPLY, ip4("127.0.0.161"));
  reply.data.ar.netSwitch = 1;
  reply.data.ar.subSwitch = 2;
  reply.data.ar.numbports = 1;
  reply.data.ar.swOut[0] = 3;
  memcpy(reply.data.ar.shortName, "Alpha", 6);
  memcpy(reply.data.ar.ip, &reply.from.s_addr, 4);

  ASSERT_TRUE(artnet_nl_update(&n, &n.node_list, &reply) == ARTNET_EOK,
              "first reply should create a node list entry");
  first_private = n.node_list.first;

  memcpy(reply.data.ar.shortName, "Alpha-2", 8);
  ASSERT_TRUE(artnet_nl_update(&n, &n.node_list, &reply) == ARTNET_EOK,
              "second reply with same IP and same first port should update the existing entry");
  ASSERT_TRUE(n.node_list.first == first_private &&
              n.node_list.length == 1,
              "same IP and same first port should resolve to the same node list entry");
}

static void test_diag_unicast_then_broadcast_with_multiple_controllers(void) {
  artnet_node_t n;
  artnet_packet_t poll1;
  artnet_packet_t poll2;
  send_capture_t send_capture = {0};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  init_packet(&poll1, ARTNET_POLL, ip4("127.0.0.170"));
  poll1.data.ap.flags = ARTNET_POLL_FLAG_DIAG_ENABLE | ARTNET_POLL_FLAG_DIAG_UNICAST;
  poll1.data.ap.diagPriority = ARTNET_DIAG_MEDIUM;
  handle_poll(&n, &poll1);

  ASSERT_TRUE(n.state.diag_enabled == TRUE,
              "first diagnostic poll should enable diagnostics");
  ASSERT_TRUE(n.state.diag_unicast == TRUE,
              "single controller requesting unicast diagnostics should enable unicast mode");
  ASSERT_TRUE(n.state.diag_priority == ARTNET_DIAG_MEDIUM,
              "diagnostic priority threshold should track the poll request");

  ASSERT_TRUE(artnet_send_diagnostic((artnet_node)&n, ARTNET_DIAG_HIGH, 0, "diag1") == ARTNET_EOK,
              "diagnostic send should succeed for a priority above threshold");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_DIAGDATA &&
              send_capture.to.s_addr == ip4("127.0.0.170").s_addr,
              "single-controller diagnostics should be unicasted back to the poller");

  send_capture.called = 0;
  init_packet(&poll2, ARTNET_POLL, ip4("127.0.0.171"));
  poll2.data.ap.flags = ARTNET_POLL_FLAG_DIAG_ENABLE | ARTNET_POLL_FLAG_DIAG_UNICAST;
  poll2.data.ap.diagPriority = ARTNET_DIAG_LOW;
  handle_poll(&n, &poll2);

  ASSERT_TRUE(n.state.diag_controller_count == 2,
              "second controller should be tracked for diagnostics");
  ASSERT_TRUE(n.state.diag_unicast == FALSE,
              "multiple diagnostic controllers should force broadcast diagnostics");
  ASSERT_TRUE(n.state.diag_priority == ARTNET_DIAG_LOW,
              "diagnostic priority threshold should keep the numerically lowest requested priority");

  ASSERT_TRUE(artnet_send_diagnostic((artnet_node)&n, ARTNET_DIAG_LOW, 0, "diag2") == ARTNET_EOK,
              "diagnostic send at threshold should succeed");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.to.s_addr == n.state.bcast_addr.s_addr,
              "multiple diagnostic controllers should force broadcast diagnostics");

  send_capture.called = 0;
  ASSERT_TRUE(artnet_send_diagnostic((artnet_node)&n, ARTNET_DIAG_HIGH, 0, "diag3") == ARTNET_EOK,
              "high-priority diagnostic should still send");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.to.s_addr == n.state.bcast_addr.s_addr,
              "multiple diagnostic controllers should force broadcast diagnostics");

  stop_sendable_node(&n);
}

static void test_sync_timeout_and_dmx_keepalive_retransmission(void) {
  artnet_node_t n;
  send_capture_t send_capture = {0};
  rdm_init_capture_t dmx_capture = {0};

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.callbacks.dmx_c.fh = rdm_init_handler;
  n.callbacks.dmx_c.data = &dmx_capture;

  n.state.sync_mode = 1;
  n.state.last_sync_time = 1;
  n.ports.out[0].sync_pending = TRUE;
  n.ports.out[0].sync_length = 2;
  n.ports.out[0].sync_data[0] = 55;
  n.ports.out[0].sync_data[1] = 66;

  n.ports.in[0].port_enabled = TRUE;
  n.ports.in[0].port_addr = make_addr(1, 2, 6);
  n.ports.in[0].last_dmx_length = 4;
  n.ports.in[0].last_dmx_data[0] = 7;
  n.ports.in[0].last_dmx_data[1] = 8;
  n.ports.in[0].last_dmx_data[2] = 9;
  n.ports.in[0].last_dmx_data[3] = 10;
  n.ports.in[0].last_dmx_send_time = 1;
  add_stub_node_entry(&n, "192.168.1.20", 1, 2, 6);

  check_timeouts(&n);

  ASSERT_TRUE(n.state.sync_mode == 0,
              "check_timeouts should clear sync mode after ArtSync timeout");
  ASSERT_TRUE(n.ports.out[0].sync_pending == FALSE &&
              n.ports.out[0].length == 2 &&
              n.ports.out[0].data[0] == 55 &&
              n.ports.out[0].data[1] == 66,
              "check_timeouts should flush pending sync output when sync mode times out");
  ASSERT_TRUE(dmx_capture.called == 1 && dmx_capture.port == 0,
              "check_timeouts should trigger the DMX callback when flushing timed-out sync data");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_DMX &&
              send_capture.to.s_addr == ip4("192.168.1.20").s_addr,
              "check_timeouts should retransmit DMX keepalive to matching subscribers");

  stop_sendable_node(&n);
}

static void test_firmware_reply_callbacks_for_allgood_and_fail(void) {
  artnet_node_t n;
  artnet_packet_t reply;
  firmware_status_capture_t status_capture = {0};
  node_entry_private_t *entry = NULL;

  init_test_node(&n);
  start_sendable_node(&n);
  entry = add_stub_node_entry(&n, "127.0.0.172", 1, 2, 3);
  entry->firmware.bytes_total = 100;
  entry->firmware.bytes_current = 100;
  entry->firmware.callback = firmware_status_handler;
  entry->firmware.user_data = &status_capture;

  init_packet(&reply, ARTNET_FIRMWAREREPLY, ip4("127.0.0.172"));
  reply.data.firmwarer.type = ARTNET_FIRMWARE_ALLGOOD;
  ASSERT_TRUE(handle_firmware_reply(&n, &reply) == ARTNET_EOK,
              "ALLGOOD firmware reply should be processed");
  ASSERT_TRUE(status_capture.called == 1 && status_capture.code == ARTNET_FIRMWARE_ALLGOOD,
              "ALLGOOD firmware reply should invoke completion callback");

  status_capture.called = 0;
  entry->firmware.bytes_total = 200;
  entry->firmware.bytes_current = 50;
  entry->firmware.callback = firmware_status_handler;
  entry->firmware.user_data = &status_capture;
  reply.data.firmwarer.type = ARTNET_FIRMWARE_FAIL;
  ASSERT_TRUE(handle_firmware_reply(&n, &reply) == ARTNET_EOK,
              "FAIL firmware reply should be processed");
  ASSERT_TRUE(status_capture.called == 1 && status_capture.code == ARTNET_FIRMWARE_FAIL,
              "FAIL firmware reply should invoke failure callback");

  stop_sendable_node(&n);
}

static void test_firmware_continuation_from_wrong_sender_fails(void) {
  artnet_node_t n;
  artnet_packet_t first;
  artnet_packet_t cont;
  send_capture_t send_capture = {0};
  uint16_t words[600];
  int i = 0;

  for (i = 0; i < 600; i++) {
    words[i] = (uint16_t)(0x2000 + i);
  }

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  init_packet(&first, ARTNET_FIRMWAREMASTER, ip4("127.0.0.173"));
  first.data.firmware.type = ARTNET_FIRMWARE_FIRMFIRST;
  artnet_misc_int_to_bytes(600, first.data.firmware.length);
  memcpy(first.data.firmware.data, words, ARTNET_FIRMWARE_SIZE * sizeof(uint16_t));
  ASSERT_TRUE(handle_firmware(&n, &first) == ARTNET_EOK,
              "first firmware block should start a transfer");

  send_capture.called = 0;
  init_packet(&cont, ARTNET_FIRMWAREMASTER, ip4("127.0.0.174"));
  cont.data.firmware.type = ARTNET_FIRMWARE_FIRMCONT;
  cont.data.firmware.blockId = 1;
  artnet_misc_int_to_bytes(600, cont.data.firmware.length);
  memcpy(cont.data.firmware.data, &words[ARTNET_FIRMWARE_SIZE], 88 * sizeof(uint16_t));

  ASSERT_TRUE(handle_firmware(&n, &cont) == ARTNET_EOK,
              "continuation block from the wrong sender should still be handled");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_FIRMWAREREPLY &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_FAIL,
              "continuation block from a different sender should be rejected with FAIL");

  stop_sendable_node(&n);
}

static void test_firmware_last_block_out_of_range_fails(void) {
  artnet_node_t n;
  artnet_packet_t first;
  artnet_packet_t last;
  send_capture_t send_capture = {0};
  uint16_t words[514];
  int i = 0;

  for (i = 0; i < 514; i++) {
    words[i] = (uint16_t)(0x3000 + i);
  }

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  init_packet(&first, ARTNET_FIRMWAREMASTER, ip4("127.0.0.175"));
  first.data.firmware.type = ARTNET_FIRMWARE_FIRMFIRST;
  artnet_misc_int_to_bytes(514, first.data.firmware.length);
  memcpy(first.data.firmware.data, words, ARTNET_FIRMWARE_SIZE * sizeof(uint16_t));
  ASSERT_TRUE(handle_firmware(&n, &first) == ARTNET_EOK,
              "first block should initialize firmware transfer state");

  send_capture.called = 0;
  init_packet(&last, ARTNET_FIRMWAREMASTER, ip4("127.0.0.175"));
  last.data.firmware.type = ARTNET_FIRMWARE_FIRMLAST;
  last.data.firmware.blockId = 9;
  artnet_misc_int_to_bytes(514, last.data.firmware.length);
  memcpy(last.data.firmware.data, &words[ARTNET_FIRMWARE_SIZE], 2 * sizeof(uint16_t));

  ASSERT_TRUE(handle_firmware(&n, &last) == ARTNET_EOK,
              "out-of-range last block should still be handled");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_FIRMWAREREPLY &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_FAIL,
              "out-of-range last block should reply with FAIL");

  stop_sendable_node(&n);
}

static void test_node_list_allows_same_ip_with_different_first_port(void) {
  artnet_node_t n;
  artnet_packet_t reply1;
  artnet_packet_t reply2;
  artnet_node_list nl = NULL;

  init_test_node(&n);
  n.state.mode = ARTNET_ON;
  nl = &n.node_list;

  init_packet(&reply1, ARTNET_REPLY, ip4("127.0.0.180"));
  reply1.data.ar.netSwitch = 1;
  reply1.data.ar.subSwitch = 2;
  reply1.data.ar.numbports = 1;
  reply1.data.ar.swOut[0] = 3;
  memcpy(reply1.data.ar.ip, &reply1.from.s_addr, 4);

  init_packet(&reply2, ARTNET_REPLY, ip4("127.0.0.180"));
  reply2.data.ar.netSwitch = 1;
  reply2.data.ar.subSwitch = 2;
  reply2.data.ar.numbports = 1;
  reply2.data.ar.swOut[0] = 4;
  memcpy(reply2.data.ar.ip, &reply2.from.s_addr, 4);

  ASSERT_TRUE(artnet_nl_update(&n, &n.node_list, &reply1) == ARTNET_EOK,
              "first joined-node reply should be added");
  ASSERT_TRUE(artnet_nl_update(&n, &n.node_list, &reply2) == ARTNET_EOK,
              "second joined-node reply with same IP but different first port should be added");
  ASSERT_TRUE(artnet_nl_get_length(nl) == 2,
              "node list should allow multiple joined-node entries sharing the same IP");
}

static void test_directory_updates_reply_target_and_unicasts_reply(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.100");

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  n.state.reply_addr = ip4("1.1.1.1");
  init_packet(&p, ARTNET_DIRECTORY, requester);

  handle(&n, &p);

  ASSERT_TRUE(n.state.reply_addr.s_addr == requester.s_addr,
              "handle_directory should set reply_addr to packet source");
  ASSERT_TRUE(send_capture.called == 1,
              "handle_directory should trigger exactly one outbound reply");
  ASSERT_TRUE(send_capture.type == ARTNET_DIRECTORYREPLY,
              "handle_directory should send ArtDirectoryReply");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "ArtDirectoryReply should be unicast to the requester");

  stop_sendable_node(&n);
}

static void test_send_directory_unicasts_to_discovered_nodes_only(void) {
  artnet_node_t n;
  send_list_capture_t send_capture = {0};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_list_capture_handler;
  n.callbacks.send.data = &send_capture;

  ASSERT_TRUE(artnet_send_directory((artnet_node)&n) == ARTNET_EACTION,
              "artnet_send_directory should not broadcast when no discovered target exists");
  ASSERT_TRUE(send_capture.called == 0,
              "artnet_send_directory should not emit a broadcast packet");

  add_stub_node_entry(&n, "192.168.1.70", 1, 2, 3);
  add_stub_node_entry(&n, "192.168.1.71", 1, 2, 4);

  ASSERT_TRUE(artnet_send_directory((artnet_node)&n) == ARTNET_EOK,
              "artnet_send_directory should query discovered nodes");
  ASSERT_TRUE(send_capture.called == 2,
              "artnet_send_directory should send one unicast request per discovered node");
  ASSERT_TRUE(send_capture.types[0] == ARTNET_DIRECTORY &&
              send_capture.to[0].s_addr == ip4("192.168.1.70").s_addr,
              "first ArtDirectory request should target the first discovered node");
  ASSERT_TRUE(send_capture.types[1] == ARTNET_DIRECTORY &&
              send_capture.to[1].s_addr == ip4("192.168.1.71").s_addr,
              "second ArtDirectory request should target the second discovered node");

  stop_sendable_node(&n);
}

static void test_file_fn_master_updates_reply_target_and_reply_target_is_used(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  uint8_t data[4] = {1, 2, 3, 4};
  struct in_addr requester = ip4("10.2.2.100");

  init_test_node(&n);
  requester = ip4("127.0.0.101");
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  n.state.reply_addr = ip4("2.2.2.2");
  init_packet(&p, ARTNET_FILEFNMASTER, requester);
  p.data.filefn.lengthHi = 0;
  p.data.filefn.lengthLo = 8;
  memcpy(p.data.filefn.filename, "test.bin", 8);
  p.data.filefn.filename[8] = '\0';
  p.length = (int)(sizeof(artnet_file_fn_master_t) - sizeof(p.data.filefn.filename) + 9);

  handle(&n, &p);

  ASSERT_TRUE(n.state.reply_addr.s_addr == requester.s_addr,
              "handle_file_fn_master should set reply_addr to packet source");
  ASSERT_TRUE(artnet_tx_file_fn_reply(&n, 7, 4, data, 4) == ARTNET_EOK,
              "artnet_tx_file_fn_reply should succeed after FileFnMaster sets reply target");
  ASSERT_TRUE(send_capture.called == 1,
              "artnet_tx_file_fn_reply should emit one outbound packet");
  ASSERT_TRUE(send_capture.type == ARTNET_FILEFNREPLY,
              "artnet_tx_file_fn_reply should send ArtFileFnReply");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "ArtFileFnReply should be unicast to the FileFnMaster requester");

  stop_sendable_node(&n);
}

static void test_ipprog_query_updates_reply_target_and_sends_ipreply(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.102");

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  n.state.reply_addr = ip4("3.3.3.3");
  init_packet(&p, ARTNET_IPPROG, requester);
  p.data.aip.Command = 0x00;  // query only

  handle(&n, &p);

  ASSERT_TRUE(n.state.reply_addr.s_addr == requester.s_addr,
              "handle_ipprog query should keep requester as reply target");
  ASSERT_TRUE(send_capture.called == 1,
              "handle_ipprog query should emit one outbound packet");
  ASSERT_TRUE(send_capture.type == ARTNET_IPREPLY,
              "handle_ipprog query should send ArtIpProgReply");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "ArtIpProgReply should be unicast to the requester");

  stop_sendable_node(&n);
}

static void test_ipprog_program_ip_keeps_requester_target_and_reports_network_order(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.103");
  struct in_addr new_ip = ip4("10.77.66.55");
  struct in_addr new_mask = ip4("255.255.254.0");
  struct in_addr new_gw = ip4("10.77.66.1");

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.state.reply_addr = ip4("4.4.4.4");

  init_packet(&p, ARTNET_IPPROG, requester);
  p.data.aip.Command = 0x96;  // bit7 enable + bit4 gw + bit2 ip + bit1 subnet
  p.data.aip.ProgIpHi = 10;
  p.data.aip.ProgIp2 = 77;
  p.data.aip.ProgIp1 = 66;
  p.data.aip.ProgIpLo = 55;
  p.data.aip.ProgSmHi = 255;
  p.data.aip.ProgSm2 = 255;
  p.data.aip.ProgSm1 = 254;
  p.data.aip.ProgSmLo = 0;
  p.data.aip.ProgDgHi = 10;
  p.data.aip.ProgDg2 = 77;
  p.data.aip.ProgDg1 = 66;
  p.data.aip.ProgDgLo = 1;

  handle(&n, &p);

  ASSERT_TRUE(n.state.ip_addr.s_addr == new_ip.s_addr,
              "handle_ipprog should update node IP using correct network byte order");
  ASSERT_TRUE(n.state.subnet_mask.s_addr == new_mask.s_addr,
              "handle_ipprog should update subnet mask using correct network byte order");
  ASSERT_TRUE(n.state.gateway.s_addr == new_gw.s_addr,
              "handle_ipprog should update gateway using correct network byte order");
  ASSERT_TRUE(n.state.reply_addr.s_addr == requester.s_addr,
              "handle_ipprog should not overwrite reply_addr with programmed node IP");
  ASSERT_TRUE(send_capture.called == 2,
              "handle_ipprog programming request should emit PollReply and IpProgReply");
  ASSERT_TRUE(send_capture.type == ARTNET_IPREPLY,
              "last outbound packet for handle_ipprog should be ArtIpProgReply");
  ASSERT_TRUE(send_capture.to.s_addr == requester.s_addr,
              "ArtIpProgReply should target the programming requester");
  ASSERT_TRUE(send_capture.data.aipr.ProgIpHi == 10 &&
              send_capture.data.aipr.ProgIp2 == 77 &&
              send_capture.data.aipr.ProgIp1 == 66 &&
              send_capture.data.aipr.ProgIpLo == 55,
              "ArtIpProgReply should encode the programmed IP address correctly");
  ASSERT_TRUE(send_capture.data.aipr.ProgSmHi == 255 &&
              send_capture.data.aipr.ProgSm2 == 255 &&
              send_capture.data.aipr.ProgSm1 == 254 &&
              send_capture.data.aipr.ProgSmLo == 0,
              "ArtIpProgReply should encode the programmed subnet mask correctly");
  ASSERT_TRUE(send_capture.data.aipr.ProgDgHi == 10 &&
              send_capture.data.aipr.ProgDg2 == 77 &&
              send_capture.data.aipr.ProgDg1 == 66 &&
              send_capture.data.aipr.ProgDgLo == 1,
              "ArtIpProgReply should encode the programmed gateway correctly");

  stop_sendable_node(&n);
}

static void test_command_esta_filter_blocks_non_matching_callbacks(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.command.fh = simple_packet_handler;
  n.callbacks.command.data = &capture;

  init_packet(&p, ARTNET_COMMAND, ip4("10.5.5.100"));
  p.data.cmd.estaManHi = 0xAA;
  p.data.cmd.estaManLo = 0xBB;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle_command should ignore non-matching ESTA packets before invoking callbacks");
}

static void test_command_esta_filter_allows_matching_callbacks(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.command.fh = simple_packet_handler;
  n.callbacks.command.data = &capture;

  init_packet(&p, ARTNET_COMMAND, ip4("10.5.5.101"));
  p.data.cmd.estaManHi = 0x12;
  p.data.cmd.estaManLo = 0x34;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 1,
              "handle_command should invoke callbacks for matching ESTA packets");
}

static void test_trigger_oem_filter_blocks_non_matching_callbacks(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.trigger.fh = simple_packet_handler;
  n.callbacks.trigger.data = &capture;

  init_packet(&p, ARTNET_TRIGGER, ip4("10.6.6.100"));
  p.data.trigger.oemCodeHi = 0xAA;
  p.data.trigger.oemCodeLo = 0xBB;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle_trigger should ignore non-matching OEM packets before invoking callbacks");
}

static void test_trigger_oem_filter_allows_matching_callbacks(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.trigger.fh = simple_packet_handler;
  n.callbacks.trigger.data = &capture;

  init_packet(&p, ARTNET_TRIGGER, ip4("10.6.6.101"));
  p.data.trigger.oemCodeHi = 0x56;
  p.data.trigger.oemCodeLo = 0x78;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 1,
              "handle_trigger should invoke callbacks for matching OEM packets");
}

static void test_reply_tx_requires_reply_target(void) {
  artnet_node_t n;
  uint8_t data[4] = {1, 2, 3, 4};

  init_test_node(&n);
  n.state.mode = ARTNET_ON;
  n.state.reply_addr.s_addr = 0;

  ASSERT_TRUE(artnet_tx_directory_reply(&n) == ARTNET_EACTION,
              "artnet_tx_directory_reply should fail with EACTION when reply_addr is unset");
  ASSERT_TRUE(artnet_tx_ipprog_reply(&n) == ARTNET_EACTION,
              "artnet_tx_ipprog_reply should fail with EACTION when reply_addr is unset");
  ASSERT_TRUE(artnet_tx_file_fn_reply(&n, 0, 4, data, 4) == ARTNET_EACTION,
              "artnet_tx_file_fn_reply should fail with EACTION when reply_addr is unset");
}

static void test_tx_helpers_reject_invalid_arguments(void) {
  artnet_node_t n;
  uint8_t bytes[4] = {1, 2, 3, 4};
  uint16_t words[2] = {0x0102, 0x0304};

  init_test_node(&n);
  n.state.mode = ARTNET_ON;
  n.state.diag_enabled = 1;
  n.state.diag_priority = ARTNET_DIAG_LOW;
  n.state.diag_unicast = 1;
  n.state.reply_addr.s_addr = 0;

  ASSERT_TRUE(artnet_tx_diagdata(&n, ARTNET_DIAG_LOW, 0, NULL) == ARTNET_EARG,
              "artnet_tx_diagdata should reject NULL text");
  ASSERT_TRUE(artnet_tx_diagdata(&n, ARTNET_DIAG_LOW, 0, "diag") == ARTNET_EACTION,
              "unicast ArtDiagData should require a reply target");

  ASSERT_TRUE(artnet_tx_timecode(&n, 30, 0, 0, 0, ARTNET_TIMECODE_FILM, 0) == ARTNET_EARG,
              "artnet_tx_timecode should reject invalid frames");
  ASSERT_TRUE(artnet_tx_timecode(&n, 0, 60, 0, 0, ARTNET_TIMECODE_FILM, 0) == ARTNET_EARG,
              "artnet_tx_timecode should reject invalid seconds");
  ASSERT_TRUE(artnet_tx_timesync(&n, 0, 0, 24, 1, 0, 26) == ARTNET_EARG,
              "artnet_tx_timesync should reject invalid hours");
  ASSERT_TRUE(artnet_tx_timesync(&n, 0, 0, 0, 0, 0, 26) == ARTNET_EARG,
              "artnet_tx_timesync should reject invalid day of month");

  ASSERT_TRUE(artnet_tx_trigger(&n, 0x56, 0x78, ARTNET_TRIGGER_KEY_SHOW + 1, 0, NULL, 0) == ARTNET_EARG,
              "artnet_tx_trigger should reject invalid trigger keys");
  ASSERT_TRUE(artnet_tx_trigger(&n, 0x56, 0x78, ARTNET_TRIGGER_KEY_MACRO, 0, NULL, 1) == ARTNET_EARG,
              "artnet_tx_trigger should reject NULL non-empty payloads");

  ASSERT_TRUE(artnet_tx_file_tn_master(&n, ip4("192.168.1.80").s_addr, 0, 0, 4, NULL, 1) == ARTNET_EARG,
              "artnet_tx_file_tn_master should reject NULL non-empty data");
  ASSERT_TRUE(artnet_tx_file_tn_master(&n, ip4("192.168.1.80").s_addr, 0, 0, 4, words, -1) == ARTNET_EARG,
              "artnet_tx_file_tn_master should reject negative data length");

  n.state.reply_addr = ip4("127.0.0.81");
  ASSERT_TRUE(artnet_tx_file_fn_reply(&n, 0, 4, NULL, 1) == ARTNET_EARG,
              "artnet_tx_file_fn_reply should reject NULL non-empty data");
  ASSERT_TRUE(artnet_tx_file_fn_reply(&n, 0, 4, bytes, -1) == ARTNET_EARG,
              "artnet_tx_file_fn_reply should reject negative data length");
}

static void test_handle_ignores_short_address_packet(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.210");

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.state.reply_addr = ip4("127.0.0.211");

  init_packet(&p, ARTNET_ADDRESS, requester);
  p.length = (int)sizeof(artnet_address_t) - 1;
  memset(p.data.addr.shortName, 0, ARTNET_SHORT_NAME_LENGTH);
  memcpy(p.data.addr.shortName, "ShortPkt", 8);
  p.data.addr.netSwitch = 0x80 | 0x04;

  handle(&n, &p);

  ASSERT_TRUE(n.state.shortName[0] == '\0',
              "handle should ignore short ArtAddress packets before changing node state");
  ASSERT_TRUE(n.state.netSwitch == 0,
              "handle should ignore short ArtAddress packets before changing network addressing");
  ASSERT_TRUE(send_capture.called == 0,
              "handle should ignore short ArtAddress packets before sending a reply");

  stop_sendable_node(&n);
}

static void test_handle_ignores_legacy_protocol_version_packet(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.command.fh = simple_packet_handler;
  n.callbacks.command.data = &capture;

  init_packet(&p, ARTNET_COMMAND, ip4("127.0.0.220"));
  p.data.cmd.verH = 0;
  p.data.cmd.ver = ARTNET_VERSION - 1;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore packets from protocol versions older than Art-Net 4");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "legacy protocol packets should update the node report to parse failure");
}

static void test_handle_ignores_invalid_dmx_length_packet(void) {
  artnet_node_t n;
  artnet_packet_t p;
  rdm_init_capture_t dmx_capture = {0};

  init_test_node(&n);
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(1, 2, 3);
  n.callbacks.dmx_c.fh = rdm_init_handler;
  n.callbacks.dmx_c.data = &dmx_capture;

  init_packet(&p, ARTNET_DMX, ip4("127.0.0.221"));
  p.length = (int)(sizeof(artnet_dmx_t) - ARTNET_DMX_LENGTH + 3);
  p.data.admx.universe = htols(make_addr(1, 2, 3));
  p.data.admx.lengthHi = 0;
  p.data.admx.length = 3;
  p.data.admx.data[0] = 1;
  p.data.admx.data[1] = 2;
  p.data.admx.data[2] = 3;

  handle(&n, &p);

  ASSERT_TRUE(dmx_capture.called == 0,
              "handle should ignore ArtDmx packets with odd payload lengths");
  ASSERT_TRUE(n.ports.out[0].length == 0,
              "invalid ArtDmx packets should not update output state");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtDmx packets should update the node report to parse failure");
}

static void test_handle_ignores_invalid_vlc_packets(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.nzs.fh = simple_packet_handler;
  n.callbacks.nzs.data = &capture;

  init_packet(&p, ARTNET_NZS, ip4("127.0.0.234"));
  p.length = (int)(sizeof(artnet_nzs_t) - ARTNET_DMX_LENGTH + ARTNET_VLC_MIN_LENGTH);
  p.data.nzs.startCode = ARTNET_VLC_START_CODE;
  p.data.nzs.universe = htols(make_addr(1, 2, 3));
  p.data.nzs.lengthHi = short_get_high_byte(ARTNET_VLC_MIN_LENGTH);
  p.data.nzs.length = short_get_low_byte(ARTNET_VLC_MIN_LENGTH);
  set_vlc_magic(p.data.nzs.data, 1);

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtVlc packets with mismatched payload count");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtVlc packets should update the node report to parse failure");

  init_test_node(&n);
  n.callbacks.nzs.fh = simple_packet_handler;
  n.callbacks.nzs.data = &capture;
  capture.called = 0;
  init_packet(&p, ARTNET_NZS, ip4("127.0.0.235"));
  p.length = (int)(sizeof(artnet_nzs_t) - ARTNET_DMX_LENGTH + ARTNET_VLC_MIN_LENGTH);
  p.data.nzs.startCode = ARTNET_VLC_START_CODE;
  p.data.nzs.universe = htols(make_addr(1, 2, 3));
  p.data.nzs.lengthHi = short_get_high_byte(ARTNET_VLC_MIN_LENGTH);
  p.data.nzs.length = short_get_low_byte(ARTNET_VLC_MIN_LENGTH);
  set_vlc_magic(p.data.nzs.data, 0);
  p.data.nzs.data[0] = 0x00;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtVlc packets with invalid magic bytes");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtVlc magic should update the node report to parse failure");
}

static void test_handle_ignores_invalid_address_acn_priority(void) {
  artnet_node_t n;
  artnet_packet_t p;

  init_test_node(&n);
  init_packet(&p, ARTNET_ADDRESS, ip4("127.0.0.222"));
  memset(p.data.addr.shortName, PROGRAM_NO_CHANGE, ARTNET_SHORT_NAME_LENGTH);
  memset(p.data.addr.longName, PROGRAM_NO_CHANGE, ARTNET_LONG_NAME_LENGTH);
  memset(p.data.addr.swIn, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  memset(p.data.addr.swOut, PROGRAM_NO_CHANGE, ARTNET_MAX_PORTS);
  p.data.addr.bindIndex = 1;
  p.data.addr.acnPriority = 201;

  handle(&n, &p);

  ASSERT_TRUE(n.state.acn_priority == 0,
              "handle should ignore ArtAddress packets with invalid sACN priorities");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtAddress packets should update the node report to parse failure");
}

static void test_media_control_reply_uses_dedicated_handler(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t control_capture = {0};
  simple_capture_t reply_capture = {0};

  init_test_node(&n);
  n.callbacks.mediacontrol.fh = simple_packet_handler;
  n.callbacks.mediacontrol.data = &control_capture;
  n.callbacks.mediacontrol_reply.fh = simple_packet_handler;
  n.callbacks.mediacontrol_reply.data = &reply_capture;

  init_packet(&p, ARTNET_MEDIACONTROLREPLY, ip4("127.0.0.223"));
  handle(&n, &p);

  ASSERT_TRUE(control_capture.called == 0,
              "ArtMediaControlReply should not be dispatched to the request handler");
  ASSERT_TRUE(reply_capture.called == 1,
              "ArtMediaControlReply should be dispatched to the dedicated reply handler");
}

static void test_handle_ignores_command_with_truncated_text_payload(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.command.fh = simple_packet_handler;
  n.callbacks.command.data = &capture;

  init_packet(&p, ARTNET_COMMAND, ip4("127.0.0.224"));
  p.data.cmd.estaManHi = 0x12;
  p.data.cmd.estaManLo = 0x34;
  p.data.cmd.lengthHi = 0;
  p.data.cmd.lengthLo = 5;
  p.length = (int)(sizeof(artnet_command_t) - ARTNET_DMX_LENGTH + 4);
  memcpy(p.data.cmd.data, "ABCD", 4);

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtCommand packets whose declared text length exceeds the packet body");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "truncated ArtCommand packets should update the node report to parse failure");
}

static void test_handle_ignores_diagdata_without_null_terminator(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.diagdata.fh = simple_packet_handler;
  n.callbacks.diagdata.data = &capture;

  init_packet(&p, ARTNET_DIAGDATA, ip4("127.0.0.225"));
  p.data.diagdata.lengthHi = 0;
  p.data.diagdata.length = 4;
  p.length = (int)(sizeof(artnet_diagdata_t) - ARTNET_DMX_LENGTH + 4);
  memcpy(p.data.diagdata.data, "ABCD", 4);

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtDiagData packets whose text payload is not null terminated");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtDiagData packets should update the node report to parse failure");
}

static void test_handle_ignores_file_fn_master_without_filename_terminator(void) {
  artnet_node_t n;
  artnet_packet_t p;

  init_test_node(&n);
  init_packet(&p, ARTNET_FILEFNMASTER, ip4("127.0.0.226"));
  p.data.filefn.lengthHi = 0;
  p.data.filefn.lengthLo = 4;
  p.length = (int)(sizeof(artnet_file_fn_master_t) - sizeof(p.data.filefn.filename) + 4);
  memcpy(p.data.filefn.filename, "test", 4);

  handle(&n, &p);

  ASSERT_TRUE(n.state.reply_addr.s_addr == 0,
              "handle should ignore ArtFileFnMaster packets without a null-terminated filename");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtFileFnMaster packets should update the node report to parse failure");
}

static void test_file_tn_master_uses_actual_payload_length_for_callback(void) {
  artnet_node_t n;
  artnet_packet_t p;
  firmware_capture_t fw_capture = {0};
  send_capture_t send_capture = {0};
  uint16_t words[4] = {0x0102, 0x0304, 0x0506, 0x0708};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.callbacks.firmware_c.fh = firmware_data_handler;
  n.callbacks.firmware_c.data = &fw_capture;

  init_packet(&p, ARTNET_FILETNMASTER, ip4("127.0.0.234"));
  artnet_misc_int_to_bytes(4, p.data.filetn.length);
  memcpy(p.data.filetn.data, words, sizeof(words));
  p.length = (int)(sizeof(artnet_file_tn_master_t) -
                   ARTNET_FIRMWARE_SIZE * sizeof(uint16_t) +
                   sizeof(words));

  ASSERT_TRUE(handle_file_tn_master(&n, &p) == ARTNET_EOK,
              "ArtFileTnMaster with a short payload should be accepted");
  ASSERT_TRUE(fw_capture.called == 1,
              "ArtFileTnMaster should bridge received payload into the firmware callback");
  ASSERT_TRUE(fw_capture.length == 4,
              "ArtFileTnMaster callback length should report the number of 16-bit words present");
  ASSERT_TRUE(memcmp(fw_capture.data, words, sizeof(words)) == 0,
              "ArtFileTnMaster callback should receive only the actual payload words");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_FIRMWAREREPLY &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_ALLGOOD,
              "ArtFileTnMaster should acknowledge a valid short payload with ALLGOOD");

  stop_sendable_node(&n);
}

static void test_file_tn_master_rejects_truncated_word_payload(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  uint8_t odd_bytes[3] = {0xAA, 0xBB, 0xCC};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  init_packet(&p, ARTNET_FILETNMASTER, ip4("127.0.0.235"));
  artnet_misc_int_to_bytes(2, p.data.filetn.length);
  memcpy(p.data.filetn.data, odd_bytes, sizeof(odd_bytes));
  p.length = (int)(sizeof(artnet_file_tn_master_t) -
                   ARTNET_FIRMWARE_SIZE * sizeof(uint16_t) +
                   sizeof(odd_bytes));

  ASSERT_TRUE(handle_file_tn_master(&n, &p) == ARTNET_EOK,
              "truncated ArtFileTnMaster payload should still be handled");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_FIRMWAREREPLY &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_FAIL,
              "truncated ArtFileTnMaster payload should be rejected with FAIL");

  stop_sendable_node(&n);
}

static void test_handle_ignores_file_fn_reply_with_payload_longer_than_total_length(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};
  uint16_t words[4] = {1, 2, 3, 4};

  init_test_node(&n);
  n.callbacks.file_fn_reply.fh = simple_packet_handler;
  n.callbacks.file_fn_reply.data = &capture;

  init_packet(&p, ARTNET_FILEFNREPLY, ip4("127.0.0.237"));
  p.data.filefnr.fileLengthHi = 0;
  p.data.filefnr.fileLengthLo = 2;
  memcpy(p.data.filefnr.data, words, sizeof(words));
  p.length = (int)(sizeof(artnet_file_fn_reply_t) -
                   ARTNET_FIRMWARE_SIZE * sizeof(uint16_t) +
                   sizeof(words));

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtFileFnReply packets whose payload exceeds declared total length");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtFileFnReply packets should update the node report to parse failure");
}

static void test_firmware_first_block_rejects_truncated_payload(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  uint16_t words[4] = {0x1111, 0x2222, 0x3333, 0x4444};

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  init_packet(&p, ARTNET_FIRMWAREMASTER, ip4("127.0.0.236"));
  p.data.firmware.type = ARTNET_FIRMWARE_FIRMFIRST;
  artnet_misc_int_to_bytes(4, p.data.firmware.length);
  memcpy(p.data.firmware.data, words, sizeof(words));
  p.length = (int)(sizeof(artnet_firmware_t) -
                   ARTNET_FIRMWARE_SIZE * sizeof(uint16_t) +
                   sizeof(words) - 1);

  ASSERT_TRUE(handle_firmware(&n, &p) == ARTNET_EOK,
              "truncated first firmware block should still be handled");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_FIRMWAREREPLY &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_FAIL,
              "truncated first firmware block should be rejected with FAIL");

  stop_sendable_node(&n);
}

static void test_handle_ignores_invalid_timesync_fields(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.timesync.fh = simple_packet_handler;
  n.callbacks.timesync.data = &capture;

  init_packet(&p, ARTNET_TIMESYNC, ip4("127.0.0.227"));
  p.data.tsync.tm_sec = 60;
  p.data.tsync.tm_min = 10;
  p.data.tsync.tm_hour = 12;
  p.data.tsync.tm_mday = 1;
  p.data.tsync.tm_mon = 0;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtTimeSync packets with out-of-range time fields");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtTimeSync packets should update the node report to parse failure");
}

static void test_handle_ignores_invalid_trigger_key(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.trigger.fh = simple_packet_handler;
  n.callbacks.trigger.data = &capture;

  init_packet(&p, ARTNET_TRIGGER, ip4("127.0.0.228"));
  p.data.trigger.oemCodeHi = 0x56;
  p.data.trigger.oemCodeLo = 0x78;
  p.data.trigger.key = 0x09;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtTrigger packets with undefined key values");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtTrigger packets should update the node report to parse failure");
}

static void test_handle_ignores_invalid_data_request_code(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.datareq.fh = simple_packet_handler;
  n.callbacks.datareq.data = &capture;

  init_packet(&p, ARTNET_DATAREQUEST, ip4("127.0.0.229"));
  p.data.datareq.requestHi = 0x00;
  p.data.datareq.requestLo = 0x06;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtDataRequest packets with undefined request codes");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtDataRequest packets should update the node report to parse failure");
}

static void test_handle_ignores_data_reply_without_null_terminated_payload(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.datarep.fh = simple_packet_handler;
  n.callbacks.datarep.data = &capture;

  init_packet(&p, ARTNET_DATAREPLY, ip4("127.0.0.230"));
  p.data.datarep.requestHi = 0x00;
  p.data.datarep.requestLo = 0x01;
  p.data.datarep.payLenHi = 0;
  p.data.datarep.payLenLo = 4;
  p.length = (int)(sizeof(artnet_data_reply_t) - sizeof(p.data.datarep.payLoad) + 4);
  memcpy(p.data.datarep.payLoad, "ABCD", 4);

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtDataReply packets whose payload is not null terminated");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtDataReply packets should update the node report to parse failure");
}

static void test_handle_ignores_invalid_diag_priority(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.diagdata.fh = simple_packet_handler;
  n.callbacks.diagdata.data = &capture;

  init_packet(&p, ARTNET_DIAGDATA, ip4("127.0.0.231"));
  p.data.diagdata.diagPriority = 0x20;
  p.data.diagdata.lengthHi = 0;
  p.data.diagdata.length = 5;
  p.length = (int)(sizeof(artnet_diagdata_t) - ARTNET_DMX_LENGTH + 5);
  memcpy(p.data.diagdata.data, "OK!\0", 5);

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtDiagData packets with undefined priority values");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtDiagData packets should update the node report to parse failure");
}

static void test_handle_ignores_invalid_timecode_frame_value(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.timecode.fh = simple_packet_handler;
  n.callbacks.timecode.data = &capture;

  init_packet(&p, ARTNET_TIMECODE, ip4("127.0.0.232"));
  p.data.tc.frames = 30;
  p.data.tc.seconds = 10;
  p.data.tc.minutes = 20;
  p.data.tc.hours = 1;
  p.data.tc.type = ARTNET_TIMECODE_SMPTE;

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtTimeCode packets with out-of-range frame values");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "invalid ArtTimeCode packets should update the node report to parse failure");
}

static void test_handle_ignores_truncated_toddata_payload(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};

  init_test_node(&n);
  n.callbacks.toddata.fh = simple_packet_handler;
  n.callbacks.toddata.data = &capture;

  init_packet(&p, ARTNET_TODDATA, ip4("127.0.0.233"));
  p.data.toddata.rdmVer = ARTNET_RDM_VERSION;
  p.data.toddata.port = 1;
  p.data.toddata.bindIndex = 1;
  p.data.toddata.cmdRes = ARTNET_TOD_FULL;
  p.data.toddata.uidCount = 2;
  p.length = (int)(sizeof(artnet_toddata_t) - sizeof(p.data.toddata.tod) + ARTNET_RDM_UID_WIDTH);

  handle(&n, &p);

  ASSERT_TRUE(capture.called == 0,
              "handle should ignore ArtTodData packets whose UID payload is truncated");
  ASSERT_TRUE(n.state.report_code == ARTNET_RC_PARSE_FAIL,
              "truncated ArtTodData packets should update the node report to parse failure");
}

int main(void) {
  test_poll_schedules_unicast_reply_and_reply_on_change_flag();
  test_legacy_14_byte_poll_is_accepted();
  test_poll_vlc_disable_flag_controls_vlc_sends();
  test_poll_target_mode_filters_non_matching_node();
  test_address_programming_updates_node_state_and_replies();
  test_address_bind_index_filters_other_bound_pages();
  test_address_programming_recomputes_ports_when_only_net_changes();
  test_address_rdm_and_bqp_commands_update_state();
  test_address_ignores_deprecated_port_index_commands();
  test_input_disable_and_enable_updates_port_status_and_reply();
  test_input_bind_index_filters_other_bound_pages();
  test_poll_reply_build_populates_artnet4_fields();
  test_poll_reply_build_marks_network_programming_and_bg_discovery_state();
  test_send_dmx_unicasts_to_matching_subscribers_only();
  test_send_dmx_matches_swin_subscribers_and_wraps_sequence();
  test_send_dmx_rejects_invalid_lengths();
  test_raw_send_dmx_unicasts_to_subscribers_only();
  test_send_nzs_unicasts_and_preserves_start_code();
  test_send_nzs_raw_unicasts_to_subscribers_only();
  test_send_vlc_validates_magic_and_payload_count();
  test_send_data_request_encodes_target_and_request_code();
  test_send_data_reply_encodes_target_and_payload();
  test_send_data_reply_rejects_invalid_payload_arguments();
  test_send_address_accepts_null_fields_as_no_change();
  test_send_ipprog_encodes_programming_fields();
  test_send_command_encodes_text_and_target();
  test_send_media_packets_encode_payloads_and_targets();
  test_send_poll_flags_encodes_explicit_artnet4_fields();
  test_sync_only_accepts_matching_last_dmx_source();
  test_sync_buffers_dmx_until_sync_flush();
  test_sync_flushes_same_ip_different_physical_merge();
  test_rdm_request_updates_reply_target_and_callback_payload();
  test_send_rdm_and_rdmsub_unicast_to_last_requester();
  test_send_rdm_and_rdmsub_reject_missing_target_and_bad_payloads();
  test_rdm_sub_updates_reply_target();
  test_failsafe_zero_full_and_scene_modes();
  test_dmx_merge_htp_ltp_and_timeout_cleanup();
  test_dmx_merge_detects_same_ip_different_physical();
  test_address_cancel_merge_ends_merge_on_next_dmx();
  test_firmware_single_block_upload_completes_and_replies_allgood();
  test_firmware_first_block_with_zero_length_fails();
  test_firmware_multi_block_upload_completes_after_last_block();
  test_firmware_reply_blockgood_sends_next_packet();
  test_firmware_last_block_out_of_range_fails();
  test_node_list_update_and_timeout_cleanup();
  test_node_list_same_ip_and_same_first_port_updates_same_entry();
  test_diag_unicast_then_broadcast_with_multiple_controllers();
  test_sync_timeout_and_dmx_keepalive_retransmission();
  test_firmware_reply_callbacks_for_allgood_and_fail();
  test_firmware_continuation_from_wrong_sender_fails();
  test_node_list_allows_same_ip_with_different_first_port();
  test_tod_request_unicasts_tod_data_to_requester();
  test_tod_updates_are_sent_to_all_previous_requesters();
  test_tod_control_flush_triggers_discovery_and_empty_tod_reply();
  test_directory_updates_reply_target_and_unicasts_reply();
  test_send_directory_unicasts_to_discovered_nodes_only();
  test_file_fn_master_updates_reply_target_and_reply_target_is_used();
  test_ipprog_query_updates_reply_target_and_sends_ipreply();
  test_ipprog_program_ip_keeps_requester_target_and_reports_network_order();
  test_command_esta_filter_blocks_non_matching_callbacks();
  test_command_esta_filter_allows_matching_callbacks();
  test_trigger_oem_filter_blocks_non_matching_callbacks();
  test_trigger_oem_filter_allows_matching_callbacks();
  test_reply_tx_requires_reply_target();
  test_tx_helpers_reject_invalid_arguments();
  test_handle_ignores_short_address_packet();
  test_handle_ignores_legacy_protocol_version_packet();
  test_handle_ignores_invalid_dmx_length_packet();
  test_handle_ignores_invalid_vlc_packets();
  test_handle_ignores_invalid_address_acn_priority();
  test_media_control_reply_uses_dedicated_handler();
  test_handle_ignores_command_with_truncated_text_payload();
  test_handle_ignores_diagdata_without_null_terminator();
  test_handle_ignores_file_fn_master_without_filename_terminator();
  test_handle_ignores_invalid_timesync_fields();
  test_handle_ignores_invalid_trigger_key();
  test_handle_ignores_invalid_data_request_code();
  test_handle_ignores_data_reply_without_null_terminated_payload();
  test_handle_ignores_invalid_diag_priority();
  test_handle_ignores_invalid_timecode_frame_value();
  test_handle_ignores_truncated_toddata_payload();
  test_file_tn_master_uses_actual_payload_length_for_callback();
  test_file_tn_master_rejects_truncated_word_payload();
  test_handle_ignores_file_fn_reply_with_payload_longer_than_total_length();
  test_firmware_first_block_rejects_truncated_payload();

  if (g_failures != 0) {
    fprintf(stderr, "Protocol regression tests failed: %d\n", g_failures);
    return 1;
  }

  printf("Protocol regression tests passed.\n");
  return 0;
}
