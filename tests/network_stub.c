#include "private.h"

int artnet_net_init(node n, const char *ip) {
  (void)ip;

  if (n->state.ip_addr.s_addr == 0) {
    n->state.ip_addr.s_addr = htonl(0x0A000002u);
  }
  if (n->state.subnet_mask.s_addr == 0) {
    n->state.subnet_mask.s_addr = htonl(0xFFFFFF00u);
  }
  if (n->state.bcast_addr.s_addr == 0) {
    n->state.bcast_addr.s_addr = htonl(0x0A0000FFu);
  }
  return ARTNET_EOK;
}

int artnet_net_start(node n) {
  n->sd = 1;
  return ARTNET_EOK;
}

int artnet_net_close(artnet_socket_t sock) {
  (void)sock;
  return ARTNET_EOK;
}

int artnet_net_recv(node n, artnet_packet p, int block) {
  (void)n;
  (void)p;
  (void)block;
  return RECV_NO_DATA;
}

int artnet_net_set_non_block(node n) {
  (void)n;
  return ARTNET_EOK;
}

int artnet_net_join(node n1, node n2) {
  (void)n1;
  (void)n2;
  return ARTNET_EOK;
}

int artnet_net_set_fdset(node n, fd_set *fdset) {
  (void)n;
  (void)fdset;
  return ARTNET_EOK;
}

int artnet_net_inet_aton(const char *ip_address, struct in_addr *address) {
  unsigned int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
  uint32_t host_value = 0;

  if (!ip_address || !address) {
    return ARTNET_EARG;
  }

  if (sscanf(ip_address, "%u.%u.%u.%u", &b0, &b1, &b2, &b3) != 4 ||
      b0 > 255 || b1 > 255 || b2 > 255 || b3 > 255) {
    return ARTNET_EARG;
  }

  host_value = ((uint32_t)b0 << 24) |
               ((uint32_t)b1 << 16) |
               ((uint32_t)b2 << 8) |
               (uint32_t)b3;
  address->s_addr = htonl(host_value);
  return ARTNET_EOK;
}

const char *artnet_net_last_error(void) {
  return "stub";
}

int artnet_net_send(node n, artnet_packet p) {
  if (n->state.mode != ARTNET_ON) {
    return ARTNET_EACTION;
  }

  p->from = n->state.ip_addr;

  if (n->callbacks.send.fh) {
    get_type(p);
    n->callbacks.send.fh(n, p, n->callbacks.send.data);
  }

  return ARTNET_EOK;
}

