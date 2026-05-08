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

static struct in_addr ip4(const char *text) {
  unsigned int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
  struct in_addr addr = {0};

  if (sscanf(text, "%u.%u.%u.%u", &b0, &b1, &b2, &b3) != 4 ||
      b0 > 255 || b1 > 255 || b2 > 255 || b3 > 255) {
    fprintf(stderr, "Invalid IPv4 literal: %s\n", text);
    exit(2);
  }

  addr.s_addr = ((uint32_t)b0 << 24) |
                ((uint32_t)b1 << 16) |
                ((uint32_t)b2 << 8) |
                (uint32_t)b3;
  return addr;
}

static void init_test_node(artnet_node_t *n) {
  memset(n, 0, sizeof(*n));
  n->state.mode = ARTNET_STANDBY;
  n->state.ip_addr = ip4("10.0.0.2");
  n->state.subnet_mask = ip4("255.255.255.0");
  n->state.bcast_addr = ip4("10.0.0.255");
  n->state.esta_hi = 0x12;
  n->state.esta_lo = 0x34;
  n->state.oem_hi = 0x56;
  n->state.oem_lo = 0x78;
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

static void test_directory_updates_reply_target(void) {
  artnet_node_t n;
  artnet_packet_t p;

  init_test_node(&n);

  n.state.reply_addr = ip4("1.1.1.1");
  init_packet(&p, ARTNET_DIRECTORY, ip4("10.1.1.100"));

  handle(&n, &p);

  ASSERT_TRUE(n.state.reply_addr.s_addr == p.from.s_addr,
              "handle_directory should set reply_addr to packet source");
}

static void test_file_fn_master_updates_reply_target(void) {
  artnet_node_t n;
  artnet_packet_t p;

  init_test_node(&n);

  n.state.reply_addr = ip4("2.2.2.2");
  init_packet(&p, ARTNET_FILEFNMASTER, ip4("10.2.2.100"));

  handle(&n, &p);

  ASSERT_TRUE(n.state.reply_addr.s_addr == p.from.s_addr,
              "handle_file_fn_master should set reply_addr to packet source");
}

static void test_ipprog_query_updates_reply_target(void) {
  artnet_node_t n;
  artnet_packet_t p;

  init_test_node(&n);

  n.state.reply_addr = ip4("3.3.3.3");
  init_packet(&p, ARTNET_IPPROG, ip4("10.3.3.100"));
  p.data.aip.Command = 0x00;  // query only

  handle(&n, &p);

  ASSERT_TRUE(n.state.reply_addr.s_addr == p.from.s_addr,
              "handle_ipprog query should keep requester as reply target");
}

static void test_ipprog_program_ip_keeps_requester_target(void) {
  artnet_node_t n;
  artnet_packet_t p;
  struct in_addr requester = ip4("10.4.4.100");
  struct in_addr new_ip = ip4("10.77.66.55");

  init_test_node(&n);

  init_packet(&p, ARTNET_IPPROG, requester);
  p.data.aip.Command = 0x84;  // bit7 enable + bit2 program IP
  p.data.aip.ProgIpHi = 10;
  p.data.aip.ProgIp2 = 77;
  p.data.aip.ProgIp1 = 66;
  p.data.aip.ProgIpLo = 55;

  handle(&n, &p);

  ASSERT_TRUE(n.state.ip_addr.s_addr == new_ip.s_addr,
              "handle_ipprog should update node IP when bit2 is set");
  ASSERT_TRUE(n.state.reply_addr.s_addr == requester.s_addr,
              "handle_ipprog should not overwrite reply_addr with programmed node IP");
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
  test_directory_updates_reply_target();
  test_file_fn_master_updates_reply_target();
  test_ipprog_query_updates_reply_target();
  test_ipprog_program_ip_keeps_requester_target();
  test_reply_tx_requires_reply_target();

  if (g_failures != 0) {
    fprintf(stderr, "Protocol regression tests failed: %d\n", g_failures);
    return 1;
  }

  printf("Protocol regression tests passed.\n");
  return 0;
}
