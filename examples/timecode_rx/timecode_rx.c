/*
 * timecode_rx.c
 * Art-Net 4 TimeCode receiver example
 *
 * Listens for ArtTimeCode packets and prints the received timecode.
 *
 * Usage: timecode_rx [-i <bind_ip>]
 *   -i  IP address to bind to (default: first non-loopback interface)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <artnet/artnet.h>
#include <artnet/packets.h>
#include <artnet/common.h>

static volatile int running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static const char *type_name(uint8_t type) {
  switch (type) {
    case ARTNET_TIMECODE_FILM:  return "Film (24fps)";
    case ARTNET_TIMECODE_EBU:   return "EBU (25fps)";
    case ARTNET_TIMECODE_DF:    return "DF (29.97fps)";
    case ARTNET_TIMECODE_SMPTE: return "SMPTE (30fps)";
  }
  return "Unknown";
}

static int timecode_handler(artnet_node n, void *pp, void *data) {
  (void)n;
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_timecode_t *tc = &packet->data.tc;

  printf("[RX] %02d:%02d:%02d:%02d  %s  stream=%d\n",
         tc->hours, tc->minutes, tc->seconds, tc->frames,
         type_name(tc->type), tc->streamId);
  return 0;
}

static void print_usage(const char *prog) {
  printf("Usage: %s [-i <bind_ip>]\n", prog);
  printf("  -i  IP address to bind to (default: auto)\n");
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char *ip = NULL;
  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      ip = argv[++i];
    } else {
      print_usage(argv[0]);
      return 1;
    }
  }

  printf("Art-Net 4 TimeCode Receiver\n");
  printf("  Bind IP: %s\n\n", ip ? ip : "auto");

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: failed to create node: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_NODE);
  artnet_set_style_code(node, ARTNET_ST_NODE);
  artnet_set_short_name(node, "LibArtNet TC-RX");
  artnet_set_long_name(node, "libartnet Art-Net 4 TimeCode Receiver");

  artnet_set_handler(node, ARTNET_TIMECODE_HANDLER, timecode_handler, NULL);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: failed to start node: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("TimeCode receiver started, waiting for packets... (Ctrl+C to stop)\n\n");

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
