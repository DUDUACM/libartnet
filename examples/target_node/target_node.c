/*
 * target_node.c
 * Art-Net 4 target node example (4-universe, remotely manageable)
 *
 * A DMX node that can be remotely configured by a controller via
 * ArtAddress (name, net, subnet, universe) and ArtInput (port enable/disable).
 * Prints configuration changes as they are applied.
 *
 * Usage: target_node [-i <bind_ip>] [-n <net>] [-s <subnet>] [-u <universe>]
 *   -i  IP address to bind to (default: first non-loopback interface)
 *   -n  Net address 0-127 (default: 0)
 *   -s  Subnet address 0-15 (default: 0)
 *   -u  Starting port address 0-15 (default: 0)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <artnet/artnet.h>
#include <artnet/common.h>

#define DEFAULT_NET      0
#define DEFAULT_SUBNET   0
#define DEFAULT_UNIVERSE 0
#define NUM_PORTS        ARTNET_MAX_PORTS

static volatile int running = 1;

static int dmx_handler(artnet_node n, int port, void *data) {
  (void)data;
  int length;
  uint8_t *dmx = artnet_read_dmx(n, port, &length);
  if (dmx && length > 0) {
    printf("[RX] Port %d, %d ch: %d %d %d %d ... %d\n",
           port, length, dmx[0], dmx[1], dmx[2], dmx[3], dmx[length - 1]);
  }
  return 0;
}

static int program_handler(artnet_node n, void *data) {
  (void)data;
  printf("\n=== Remote Programming Applied ===\n");
  artnet_dump_config(n);
  printf("===================================\n\n");
  return 0;
}

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static void print_usage(const char *prog) {
  printf("Usage: %s [-i <bind_ip>] [-n <net>] [-s <subnet>] [-u <universe>]\n", prog);
  printf("  -i  IP address to bind to (default: auto)\n");
  printf("  -n  Net address 0-127 (default: %d)\n", DEFAULT_NET);
  printf("  -s  Subnet address 0-15 (default: %d)\n", DEFAULT_SUBNET);
  printf("  -u  Starting port address 0-15 (default: %d)\n", DEFAULT_UNIVERSE);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char *ip = NULL;
  int net = DEFAULT_NET;
  int subnet = DEFAULT_SUBNET;
  int universe = DEFAULT_UNIVERSE;
  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      ip = argv[++i];
    } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
      net = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
      subnet = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
      universe = atoi(argv[++i]);
    } else {
      print_usage(argv[0]);
      return 1;
    }
  }

  if (net < 0 || net > 127) {
    printf("Error: net must be 0-127\n");
    return 1;
  }
  if (subnet < 0 || subnet > 15) {
    printf("Error: subnet must be 0-15\n");
    return 1;
  }
  if (universe < 0 || universe > 15) {
    printf("Error: port address must be 0-15\n");
    return 1;
  }

  printf("Art-Net 4 Target Node (%d universes, remotely manageable)\n", NUM_PORTS);
  printf("  Bind IP  : %s\n", ip ? ip : "auto");
  printf("  Base Addr: Net %d / Subnet %d / Port %d\n\n", net, subnet, universe);

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: failed to create node: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_NODE);
  artnet_set_style_code(node, ARTNET_ST_NODE);
  artnet_set_short_name(node, "TargetNode");
  artnet_set_long_name(node, "libartnet Art-Net 4 Target Node");
  artnet_setoem(node, 0xFF, 0x00);
  artnet_setesta(node, 0x00, 0xFF);

  for (i = 0; i < NUM_PORTS; i++) {
    uint16_t addr = (uint16_t)((net << 8) | (subnet << 4) | (universe + i));
    artnet_set_port_type(node, i, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
    artnet_set_port_addr(node, i, ARTNET_OUTPUT_PORT, (uint8_t)(universe + i));
    printf("  Port %d -> 0x%04X (Net %d Sub %d Port %d) [OUT]\n",
           i, addr, (addr >> 8) & 0x7F, (addr >> 4) & 0x0F, addr & 0x0F);
  }

  artnet_set_dmx_handler(node, dmx_handler, NULL);
  artnet_set_program_handler(node, program_handler, NULL);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: failed to start node: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("\nTarget node started, waiting for management... (Ctrl+C to stop)\n");

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  while (running) {
    artnet_read(node, 1000);
  }

  printf("\nStopping...\n");
  artnet_stop(node);
  artnet_destroy(node);
  return 0;
}
