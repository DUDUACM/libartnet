/*
 * dmx_node_tx_rx.c
 * Art-Net 4 DMX node example (8-universe, input + output)
 *
 * Uses 2 joined nodes to support 8 universes.
 * Each node has 4 ports (Art-Net protocol limit), so 2 nodes = 8 ports.
 *
 * Usage: dmx_node_tx_rx [-i <bind_ip>] [-n <net>] [-s <subnet>] [-u <universe>]
 *                       [-c <channels>] [--no-chase]
 *   -i  IP address to bind to (default: first non-loopback interface)
 *   -n  Net address 0-127 (default: 0)
 *   -s  Subnet address 0-15 (default: 0)
 *   -u  Starting port address 0-15 (default: 0)
 *   -c  Number of DMX channels per universe (default: 512)
 *       --no-chase  Only receive, don't send DMX chase
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <artnet/artnet.h>
#include <artnet/common.h>

#define DEFAULT_NET      0
#define DEFAULT_SUBNET   0
#define DEFAULT_UNIVERSE 0
#define DEFAULT_CHANNELS ARTNET_DMX_LENGTH
#define FRAME_INTERVAL_MS 25
#define SINE_PERIOD_SEC  5.0

#define NUM_NODES        2
#define PORTS_PER_NODE   ARTNET_MAX_PORTS
#define TOTAL_PORTS      (NUM_NODES * PORTS_PER_NODE)

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

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static void print_usage(const char *prog) {
  printf("Usage: %s [-i <bind_ip>] [-n <net>] [-s <subnet>] [-u <universe>] "
         "[-c <channels>] [--no-chase]\n", prog);
  printf("  -i          IP address to bind to (default: auto)\n");
  printf("  -n          Net address 0-127 (default: %d)\n", DEFAULT_NET);
  printf("  -s          Subnet address 0-15 (default: %d)\n", DEFAULT_SUBNET);
  printf("  -u          Starting port address 0-15 (default: %d)\n", DEFAULT_UNIVERSE);
  printf("  -c          Number of DMX channels per universe 1-512 (default: %d)\n", DEFAULT_CHANNELS);
  printf("  --no-chase  Don't send DMX, just be discoverable\n");
}

int main(int argc, char *argv[]) {
  const char *ip = NULL;
  int net = DEFAULT_NET;
  int subnet = DEFAULT_SUBNET;
  int universe = DEFAULT_UNIVERSE;
  int channels = DEFAULT_CHANNELS;
  int no_chase = 0;
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
    } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
      channels = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--no-chase") == 0) {
      no_chase = 1;
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
  if (channels < 1) channels = 1;
  if (channels > ARTNET_DMX_LENGTH) channels = ARTNET_DMX_LENGTH;

  printf("Art-Net 4 DMX Node (%d universes, %d nodes)\n", TOTAL_PORTS, NUM_NODES);
  printf("  Bind IP  : %s\n", ip ? ip : "auto");
  printf("  Base Addr: Net %d / Subnet %d / Port %d\n", net, subnet, universe);
  printf("  Channels : %d per universe\n", channels);
  printf("  Chase    : %s\n\n", no_chase ? "off" : "on");

  // --- Create nodes ---
  artnet_node nodes[NUM_NODES];

  for (i = 0; i < NUM_NODES; i++) {
    nodes[i] = artnet_new(ip, 0);
    if (!nodes[i]) {
      printf("Error: failed to create node %d: %s\n", i, artnet_strerror());
      // cleanup already created nodes
      while (--i >= 0) {
        artnet_destroy(nodes[i]);
      }
      return 1;
    }

    artnet_set_node_type(nodes[i], ARTNET_NODE);
    artnet_set_style_code(nodes[i], ARTNET_ST_NODE);
    artnet_set_short_name(nodes[i], "LibArtNet 8Uni");
    artnet_set_long_name(nodes[i], "libartnet Art-Net 4 Example Node");
    artnet_setoem(nodes[i], 0xFF, 0x00);
    artnet_setesta(nodes[i], 0x00, 0xFF);

    // Configure 4 ports on this node (input + output)
    int p;
    for (p = 0; p < PORTS_PER_NODE; p++) {
      // 15-bit address: base + node_offset * 4 + port_index
      uint16_t addr = (uint16_t)((net << 8) | (subnet << 4) | (universe + i * PORTS_PER_NODE + p));
      artnet_set_port_type(nodes[i], p, ARTNET_ENABLE_INPUT | ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
      artnet_set_port_addr(nodes[i], p, ARTNET_INPUT_PORT, (uint8_t)(universe + i * PORTS_PER_NODE + p));
      artnet_set_port_addr(nodes[i], p, ARTNET_OUTPUT_PORT, (uint8_t)(universe + i * PORTS_PER_NODE + p));
      printf("  Node %d Port %d -> 0x%04X (Net %d Sub %d Port %d) [I/O]\n",
             i, p, addr,
             (addr >> 8) & 0x7F,
             (addr >> 4) & 0x0F,
             addr & 0x0F);
    }
    artnet_set_dmx_handler(nodes[i], dmx_handler, NULL);
  }

  // Join nodes: all share node[0]'s UDP socket
  for (i = 1; i < NUM_NODES; i++) {
    artnet_join(nodes[0], nodes[i]);
  }

  // Start all nodes
  for (i = 0; i < NUM_NODES; i++) {
    if (artnet_start(nodes[i]) != ARTNET_EOK) {
      printf("Error: failed to start node %d: %s\n", i, artnet_strerror());
      while (--i >= 0) {
        artnet_stop(nodes[i]);
        artnet_destroy(nodes[i]);
      }
      return 1;
    }
  }

  printf("\nAll nodes started, waiting for ArtPoll... (Ctrl+C to stop)\n");

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  uint8_t dmx_data[ARTNET_DMX_LENGTH];
  int tick = 0;

  while (running) {
    // process incoming packets on all nodes
    for (i = 0; i < NUM_NODES; i++) {
      artnet_read(nodes[i], 0);
    }

    // send DMX sine wave on all universes
    if (!no_chase) {
      tick++;
      double t = tick * FRAME_INTERVAL_MS / 1000.0;

      for (i = 0; i < NUM_NODES; i++) {
        int p;
        for (p = 0; p < PORTS_PER_NODE; p++) {
          int uni = i * PORTS_PER_NODE + p; // global universe index
          memset(dmx_data, 0, sizeof(dmx_data));
          double phase = 2.0 * 3.14159265 * t / SINE_PERIOD_SEC - uni * 0.25;
          dmx_data[uni % channels] = (uint8_t)(127.5 + 127.5 * sin(phase));
          artnet_send_dmx(nodes[i], p, (int16_t)channels, dmx_data);
        }
      }
    }

#ifdef WIN32
    Sleep(FRAME_INTERVAL_MS);
#else
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = FRAME_INTERVAL_MS * 1000000;
    nanosleep(&ts, NULL);
#endif
  }

  printf("\nStopping...\n");
  for (i = NUM_NODES - 1; i >= 0; i--) {
    artnet_stop(nodes[i]);
    artnet_destroy(nodes[i]);
  }
  return 0;
}
