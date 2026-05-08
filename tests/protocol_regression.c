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
  n.state.reply_addr = requester;
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

static void test_input_disable_and_enable_updates_port_status_and_reply(void) {
  artnet_node_t n;
  artnet_packet_t p;
  send_capture_t send_capture = {0};
  struct in_addr requester = ip4("127.0.0.132");

  init_test_node(&n);
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;
  n.state.reply_addr = requester;

  init_packet(&p, ARTNET_INPUT, requester);
  p.data.ainput.numbports = 1;
  p.data.ainput.input[0] = PORT_DISABLE_MASK;

  ASSERT_TRUE(_artnet_handle_input(&n, &p) == ARTNET_EOK,
              "_artnet_handle_input should succeed when disabling a port");
  ASSERT_TRUE((n.ports.in[0].port_status & PORT_STATUS_INPUT_DISABLED) != 0,
              "_artnet_handle_input should set input disabled bit");
  ASSERT_TRUE(send_capture.called == 1 && send_capture.type == ARTNET_REPLY,
              "_artnet_handle_input should respond with ArtPollReply after disable");

  send_capture.called = 0;
  p.data.ainput.input[0] = 0x00;
  ASSERT_TRUE(_artnet_handle_input(&n, &p) == ARTNET_EOK,
              "_artnet_handle_input should succeed when enabling a port");
  ASSERT_TRUE((n.ports.in[0].port_status & PORT_STATUS_INPUT_DISABLED) == 0,
              "_artnet_handle_input should clear input disabled bit");
  ASSERT_TRUE(send_capture.called == 1 && send_capture.type == ARTNET_REPLY,
              "_artnet_handle_input should respond with ArtPollReply after enable");

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
  memcpy(n.state.default_resp_uid, uid, sizeof(uid));
  n.ports.types[0] = ARTNET_ENABLE_OUTPUT | ARTNET_PORT_DMX;
  n.ports.out[0].port_enabled = TRUE;
  n.ports.out[0].port_addr = make_addr(1, 2, 3);
  n.ports.out[0].output_style = 1;
  n.ports.out[0].rdm_enabled = 0;

  ASSERT_TRUE(artnet_tx_build_art_poll_reply(&n) == ARTNET_EOK,
              "artnet_tx_build_art_poll_reply should succeed");

  reply = &n.ar_temp;
  ASSERT_TRUE(reply->acnPriority == 123,
              "PollReply should carry configured sACN priority");
  ASSERT_TRUE(reply->bindIndex == 1,
              "PollReply should mark root device bind index");
  ASSERT_TRUE(memcmp(reply->bindIp, &n.state.ip_addr.s_addr, ARTNET_IP_SIZE) == 0,
              "PollReply should publish bind IP equal to node IP");
  ASSERT_TRUE(reply->status2 == n.state.status2,
              "PollReply should carry Status2 flags");
  ASSERT_TRUE(reply->status3 == (uint8_t)(ARTNET_FAILSAFE_SCENE | n.state.status3),
              "PollReply should combine failsafe mode with Status3 flags");
  ASSERT_TRUE(reply->goodOutputB[0] == (ARTNET_GOODB_RDM_DISABLED | ARTNET_GOODB_STYLE_CONSTANT),
              "PollReply should publish GoodOutputB state for RDM disabled and constant style");
  ASSERT_TRUE(reply->refreshRateHi == 0 && reply->refreshRateLo == 0,
              "PollReply should publish default refresh rate");
  ASSERT_TRUE(reply->bgQueuePolicy == ARTNET_BQP_WARNING,
              "PollReply should publish background queue policy");
  ASSERT_TRUE(memcmp(reply->defaultRespUid, uid, sizeof(uid)) == 0,
              "PollReply should publish default responder UID");
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
  ASSERT_TRUE(n.ports.in[0].seq == 1,
              "artnet_send_dmx should advance sequence number");
  ASSERT_TRUE(n.ports.in[0].last_dmx_length == 4 &&
              memcmp(n.ports.in[0].last_dmx_data, data, 4) == 0,
              "artnet_send_dmx should store keepalive payload");

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

static void test_sync_only_accepts_matching_last_dmx_source(void) {
  artnet_node_t n;
  artnet_packet_t sync_packet;

  init_test_node(&n);
  init_packet(&sync_packet, ARTNET_SYNC, ip4("10.9.9.9"));

  n.state.sync_mode = 0;
  n.state.last_dmx_source = ip4("10.8.8.8");
  handle_sync(&n, &sync_packet);
  ASSERT_TRUE(n.state.sync_mode == 0,
              "handle_sync should ignore ArtSync from a different source than the last ArtDmx");

  sync_packet.from = ip4("10.8.8.8");
  handle_sync(&n, &sync_packet);
  ASSERT_TRUE(n.state.sync_mode == 1,
              "handle_sync should accept ArtSync from the same source as the last ArtDmx");
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

static void test_rdm_sub_updates_reply_target(void) {
  artnet_node_t n;
  artnet_packet_t p;
  simple_capture_t capture = {0};
  struct in_addr requester = ip4("127.0.0.142");

  init_test_node(&n);
  n.callbacks.rdm.fh = simple_packet_handler;
  n.callbacks.rdm.data = &capture;

  init_packet(&p, ARTNET_RDMSUB, requester);
  handle_rdm_sub(&n, &p);

  ASSERT_TRUE(n.state.rdm_reply_addr.s_addr == requester.s_addr,
              "handle_rdm_sub should store requester IP for compressed RDM replies");
  ASSERT_TRUE(capture.called == 1,
              "handle_rdm_sub should still invoke the packet callback");
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
  ASSERT_TRUE(fw_capture.length == total_words * (int)sizeof(uint16_t),
              "single-block firmware callback should receive the full byte length");
  ASSERT_TRUE(memcmp(fw_capture.data, words, sizeof(words)) == 0,
              "single-block firmware callback should receive the uploaded words");
  ASSERT_TRUE(send_capture.called == 1 &&
              send_capture.type == ARTNET_FIRMWAREREPLY &&
              send_capture.data.firmwarer.type == ARTNET_FIRMWARE_ALLGOOD,
              "single-block firmware upload should reply with ALLGOOD");

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

  init_test_node(&n);
  n.state.node_type = ARTNET_SRV;
  start_sendable_node(&n);
  n.callbacks.send.fh = send_capture_handler;
  n.callbacks.send.data = &send_capture;

  n.state.sync_mode = 1;
  n.state.last_sync_time = 1;

  n.ports.in[0].port_enabled = TRUE;
  n.ports.in[0].port_addr = make_addr(1, 2, 6);
  n.ports.in[0].last_dmx_length = 3;
  n.ports.in[0].last_dmx_data[0] = 7;
  n.ports.in[0].last_dmx_data[1] = 8;
  n.ports.in[0].last_dmx_data[2] = 9;
  n.ports.in[0].last_dmx_send_time = 1;
  add_stub_node_entry(&n, "192.168.1.20", 1, 2, 6);

  check_timeouts(&n);

  ASSERT_TRUE(n.state.sync_mode == 0,
              "check_timeouts should clear sync mode after ArtSync timeout");
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

int main(void) {
  test_poll_schedules_unicast_reply_and_reply_on_change_flag();
  test_poll_target_mode_filters_non_matching_node();
  test_address_programming_updates_node_state_and_replies();
  test_address_rdm_and_bqp_commands_update_state();
  test_input_disable_and_enable_updates_port_status_and_reply();
  test_poll_reply_build_populates_artnet4_fields();
  test_send_dmx_unicasts_to_matching_subscribers_only();
  test_send_nzs_unicasts_and_preserves_start_code();
  test_sync_only_accepts_matching_last_dmx_source();
  test_rdm_request_updates_reply_target_and_callback_payload();
  test_send_rdm_and_rdmsub_unicast_to_last_requester();
  test_rdm_sub_updates_reply_target();
  test_failsafe_zero_full_and_scene_modes();
  test_dmx_merge_htp_ltp_and_timeout_cleanup();
  test_firmware_single_block_upload_completes_and_replies_allgood();
  test_firmware_multi_block_upload_completes_after_last_block();
  test_firmware_reply_blockgood_sends_next_packet();
  test_node_list_update_and_timeout_cleanup();
  test_diag_unicast_then_broadcast_with_multiple_controllers();
  test_sync_timeout_and_dmx_keepalive_retransmission();
  test_firmware_reply_callbacks_for_allgood_and_fail();
  test_firmware_continuation_from_wrong_sender_fails();
  test_node_list_allows_same_ip_with_different_first_port();
  test_tod_request_unicasts_tod_data_to_requester();
  test_tod_control_flush_triggers_discovery_and_empty_tod_reply();
  test_directory_updates_reply_target_and_unicasts_reply();
  test_file_fn_master_updates_reply_target_and_reply_target_is_used();
  test_ipprog_query_updates_reply_target_and_sends_ipreply();
  test_ipprog_program_ip_keeps_requester_target_and_reports_network_order();
  test_command_esta_filter_blocks_non_matching_callbacks();
  test_command_esta_filter_allows_matching_callbacks();
  test_trigger_oem_filter_blocks_non_matching_callbacks();
  test_trigger_oem_filter_allows_matching_callbacks();
  test_reply_tx_requires_reply_target();

  if (g_failures != 0) {
    fprintf(stderr, "Protocol regression tests failed: %d\n", g_failures);
    return 1;
  }

  printf("Protocol regression tests passed.\n");
  return 0;
}
