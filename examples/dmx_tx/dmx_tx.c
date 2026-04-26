/*
 * dmx_tx.c
 * Art-Net 4 DMX transmitter example (4-universe)
 *
 * Sends a sine wave chase on 4 universes (single node) with ArtSync.
 * Supports standard DMX (ArtDmx), non-zero start code (ArtNzs),
 * and raw 15-bit universe addressing.
 *
 * Usage: dmx_tx [-i <bind_ip>] [-n <net>] [-s <subnet>] [-u <universe>]
 *               [-c <channels>] [-z <start_code>] [-r]
 *   -i  IP address to bind to (default: first non-loopback interface)
 *   -n  Net address 0-127 (default: 0)
 *   -s  Subnet address 0-15 (default: 0)
 *   -u  Starting port address 0-15 (default: 0)
 *   -c  Number of DMX channels per universe (default: 512)
 *   -z  Non-zero start code for ArtNzs (default: 0 = ArtDmx)
 *   -r  Use raw 15-bit universe addressing via artnet_raw_send_dmx
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
#define NUM_PORTS        ARTNET_MAX_PORTS
#define FRAME_INTERVAL_MS 25
#define SINE_PERIOD_SEC  5.0

static volatile int running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static void print_usage(const char *prog) {
  printf("Usage: %s [-i <bind_ip>] [-n <net>] [-s <subnet>] [-u <universe>] "
         "[-c <channels>] [-z <start_code>] [-r]\n", prog);
  printf("  -i  IP address to bind to (default: auto)\n");
  printf("  -n  Net address 0-127 (default: %d)\n", DEFAULT_NET);
  printf("  -s  Subnet address 0-15 (default: %d)\n", DEFAULT_SUBNET);
  printf("  -u  Starting port address 0-15 (default: %d)\n", DEFAULT_UNIVERSE);
  printf("  -c  Number of DMX channels per universe 1-512 (default: %d)\n", DEFAULT_CHANNELS);
  printf("  -z  Non-zero start code for ArtNzs (0 = ArtDmx, default: 0)\n");
  printf("  -r  Use raw 15-bit universe addressing\n");
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char *ip = NULL;
  int net = DEFAULT_NET;
  int subnet = DEFAULT_SUBNET;
  int universe = DEFAULT_UNIVERSE;
  int channels = DEFAULT_CHANNELS;
  int start_code = 0;
  int raw_mode = 0;
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
    } else if (strcmp(argv[i], "-z") == 0 && i + 1 < argc) {
      start_code = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-r") == 0) {
      raw_mode = 1;
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
  if (start_code < 0 || start_code > 255) start_code = 0;

  const char *mode = start_code ? "ArtNzs" : "ArtDmx";
  printf("Art-Net 4 DMX Transmitter (%d universes, %s%s)\n",
         NUM_PORTS, mode, raw_mode ? ", raw 15-bit" : "");
  printf("  Bind IP  : %s\n", ip ? ip : "auto");
  printf("  Base Addr: Net %d / Subnet %d / Port %d\n", net, subnet, universe);
  printf("  Channels : %d per universe\n", channels);
  if (start_code)
    printf("  StartCode: 0x%02X (ArtNzs)\n", start_code);
  if (raw_mode)
    printf("  Mode     : Raw 15-bit universe addressing\n");
  printf("\n");

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: failed to create node: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_NODE);
  artnet_set_style_code(node, ARTNET_ST_NODE);
  artnet_set_short_name(node, "LibArtNet DMX-TX");
  artnet_set_long_name(node, "libartnet Art-Net 4 DMX Transmitter");
  artnet_setoem(node, 0xFF, 0x00);
  artnet_setesta(node, 0x00, 0xFF);

  for (i = 0; i < NUM_PORTS; i++) {
    uint16_t addr = (uint16_t)((net << 8) | (subnet << 4) | (universe + i));
    artnet_set_port_type(node, i, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
    artnet_set_port_addr(node, i, ARTNET_OUTPUT_PORT, (uint8_t)(universe + i));
    printf("  Port %d -> 0x%04X (Net %d Sub %d Port %d) [OUT]\n",
           i, addr, (addr >> 8) & 0x7F, (addr >> 4) & 0x0F, addr & 0x0F);
  }

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: failed to start node: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("\nDMX transmitter started. (Ctrl+C to stop)\n");

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  uint8_t dmx_data[ARTNET_DMX_LENGTH];
  int tick = 0;

  while (running) {
    artnet_read(node, 0);

    tick++;
    double t = tick * FRAME_INTERVAL_MS / 1000.0;

    for (i = 0; i < NUM_PORTS; i++) {
      memset(dmx_data, 0, sizeof(dmx_data));
      double phase = 2.0 * 3.14159265 * t / SINE_PERIOD_SEC - i * 0.25;
      dmx_data[i % channels] = (uint8_t)(127.5 + 127.5 * sin(phase));

      if (start_code) {
        artnet_send_nzs(node, i, (uint8_t)start_code, (int16_t)channels, dmx_data);
      } else if (raw_mode) {
        uint16_t addr = (uint16_t)((net << 8) | (subnet << 4) | (universe + i));
        artnet_raw_send_dmx(node, addr, (int16_t)channels, dmx_data);
      } else {
        artnet_send_dmx(node, i, (int16_t)channels, dmx_data);
      }
    }

    artnet_send_sync(node);

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
  artnet_stop(node);
  artnet_destroy(node);
  return 0;
}
