/*
 * dmx_send.c
 * Art-Net DMX sender example
 *
 * Sends a simple chase pattern to a DMX universe via Art-Net.
 *
 * Usage: dmx_send [-i <bind_ip>] [-u <universe>] [-c <channels>]
 *   -i  IP address to bind to (default: first non-loopback interface)
 *   -u  Universe number (default: 0)
 *   -c  Number of DMX channels to send (default: 24)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <artnet/artnet.h>
#include <artnet/common.h>

#define DEFAULT_UNIVERSE 0
#define DEFAULT_CHANNELS 24
#define FRAME_RATE 38
#define FRAME_INTERVAL_MS 1000 / FRAME_RATE  // ~25 fps

static volatile int running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static void print_usage(const char *prog) {
  printf("Usage: %s [-i <bind_ip>] [-u <universe>] [-c <channels>]\n", prog);
  printf("  -i  IP address to bind to (default: auto)\n");
  printf("  -u  Universe number 0-15 (default: %d)\n", DEFAULT_UNIVERSE);
  printf("  -c  Number of DMX channels 1-512 (default: %d)\n", DEFAULT_CHANNELS);
}

int main(int argc, char *argv[]) {
  const char *ip = NULL;
  int universe = DEFAULT_UNIVERSE;
  int channels = DEFAULT_CHANNELS;
  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      ip = argv[++i];
    } else if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
      universe = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
      channels = atoi(argv[++i]);
    } else {
      print_usage(argv[0]);
      return 1;
    }
  }

  if (channels < 1) channels = 1;
  if (channels > ARTNET_DMX_LENGTH) channels = ARTNET_DMX_LENGTH;
  if (universe < 0 || universe > 15) {
    printf("Error: universe must be 0-15\n");
    return 1;
  }

  printf("Art-Net DMX Sender\n");
  printf("  Bind IP  : %s\n", ip ? ip : "auto");
  printf("  Universe : %d\n", universe);
  printf("  Channels : %d\n", channels);

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: failed to create Art-Net node\n");
    return 1;
  }

  artnet_set_node_type(node, ARTNET_SRV);
  artnet_set_short_name(node, "DMX Send");
  artnet_set_long_name(node, "libartnet DMX Sender Example");

  artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
  artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, (uint8_t)universe);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: failed to start Art-Net node\n");
    artnet_destroy(node);
    return 1;
  }

  printf("Node started, sending DMX data... (Ctrl+C to stop)\n\n");

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  uint8_t dmx_data[ARTNET_DMX_LENGTH];
  int step = 0;

  while (running) {
    // generate chase: single channel moving forward
    memset(dmx_data, 0, sizeof(dmx_data));
    dmx_data[step % channels] = 255;
    step++;

    artnet_send_dmx(node, 0, (int16_t)channels, dmx_data);

    // process incoming packets (non-blocking)
    artnet_read(node, 0);

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
