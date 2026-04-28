/*
 * rdm_controller.c
 * Art-Net 4 RDM controller example
 *
 * Discovers RDM devices via ArtTodRequest, displays TOD (Table of Devices),
 * and provides interactive commands for ArtTodControl and ArtRdm.
 *
 * Usage: rdm_controller [-i <bind_ip>]
 *   -i  IP address to bind to (default: first non-loopback interface)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <artnet/artnet.h>
#include <artnet/packets.h>
#include <artnet/common.h>

static volatile int running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static int tod_data_handler(artnet_node n, void *pp, void *data) {
  (void)n;
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_toddata_t *tod = &packet->data.toddata;

  printf("\n[TodData] net=%d addr=0x%02X uidCount=%d",
         tod->net, tod->address, tod->uidCount);
  if (tod->uidCount == 0) {
    printf(" (no devices)\n");
    return 0;
  }

  printf("\n");
  int count = tod->uidCount > 32 ? 32 : tod->uidCount;
  for (int i = 0; i < count; i++) {
    uint8_t *uid = tod->tod[i];
    printf("  [%d] %02X:%02X:%02X:%02X:%02X:%02X\n",
           i, uid[0], uid[1], uid[2], uid[3], uid[4], uid[5]);
  }
  printf("> ");
  fflush(stdout);
  return 0;
}

static int rdm_handler(artnet_node n, void *pp, void *data) {
  (void)n;
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_rdm_t *rdm = &packet->data.rdm;

  printf("\n[Rdm] net=%d addr=0x%02X, data:",
         rdm->net, rdm->address);
  for (int i = 0; i < 32; i++)
    printf(" %02X", rdm->data[i]);
  printf("\n> ");
  fflush(stdout);
  return 0;
}

static void print_menu(void) {
  printf("\n--- RDM Controller ---\n");
  printf("  p) Poll network (ArtPoll)\n");
  printf("  t) Request TOD (ArtTodRequest)\n");
  printf("  f) Flush TOD on all nodes\n");
  printf("  r) Send RDM to device\n");
  printf("  q) Quit\n");
  printf("  ?> ");
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char *ip = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      ip = argv[++i];
    } else {
      printf("Usage: rdm_controller [-i <bind_ip>]\n");
      return 1;
    }
  }

  printf("Art-Net 4 RDM Controller\n");
  printf("  Bind IP: %s\n\n", ip ? ip : "auto");

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_SRV);
  artnet_set_style_code(node, ARTNET_ST_CONTROLLER);
  artnet_set_short_name(node, "RDM Ctrl");
  artnet_set_long_name(node, "libartnet Art-Net 4 RDM Controller");

  for (int i = 0; i < ARTNET_MAX_PORTS; i++) {
    artnet_set_port_type(node, i, ARTNET_ENABLE_INPUT | ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
    artnet_set_port_addr(node, i, ARTNET_INPUT_PORT, (uint8_t)i);
    artnet_set_port_addr(node, i, ARTNET_OUTPUT_PORT, (uint8_t)i);
  }

  artnet_set_handler(node, ARTNET_TOD_DATA_HANDLER, tod_data_handler, NULL);
  artnet_set_handler(node, ARTNET_RDM_HANDLER, rdm_handler, NULL);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("RDM controller started.\n");
  artnet_send_poll(node, NULL, ARTNET_TTM_AUTO);

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  char cmd[512];
  print_menu();

  while (running) {
    artnet_read(node, 0);

    fd_set fds;
    struct timeval tv = {0, 100000};
    FD_ZERO(&fds);
    FD_SET(0, &fds);

    if (select(1, &fds, NULL, NULL, &tv) > 0 && FD_ISSET(0, &fds)) {
      if (!fgets(cmd, sizeof(cmd), stdin)) break;

      switch (cmd[0]) {
      case 'p': {
        int ret = artnet_send_poll(node, NULL, ARTNET_TTM_AUTO);
        printf("[Poll] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
        break;
      }
      case 't': {
        int ret = artnet_send_tod_request(node);
        printf("[TodRequest] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
        break;
      }
      case 'f': {
        int ret = artnet_send_tod_control(node, 0xFFFF, ARTNET_TOD_FLUSH);
        printf("[TodControl] Flush: %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
        break;
      }
      case 'r': {
        printf("  Universe (hex, e.g. 0000): ");
        fflush(stdout);
        char buf_uni[16];
        if (!fgets(buf_uni, sizeof(buf_uni), stdin)) break;
        unsigned int uni;
        sscanf(buf_uni, "%x", &uni);

        printf("  RDM data (hex bytes, e.g. CC 01 00): ");
        fflush(stdout);
        char buf_data[256];
        if (!fgets(buf_data, sizeof(buf_data), stdin)) break;

        uint8_t rdm_data[256];
        int rdm_len = 0;
        char *p = buf_data;
        while (*p && rdm_len < 256) {
          while (*p && !isxdigit((unsigned char)*p)) p++;
          if (!*p) break;
          unsigned int byte;
          if (sscanf(p, "%2x", &byte) != 1) break;
          rdm_data[rdm_len++] = (uint8_t)byte;
          while (*p && isxdigit((unsigned char)*p)) p++;
        }

        int ret = artnet_send_rdm(node, (uint16_t)uni, rdm_data, rdm_len);
        printf("[Rdm] %s (%d bytes to 0x%04X)\n",
               ret == ARTNET_EOK ? "OK" : artnet_strerror(), rdm_len, uni);
        break;
      }
      case 'q':
        running = 0;
        break;
      case '\n':
        break;
      default:
        break;
      }
      if (running) print_menu();
    }
  }

  printf("\nStopping...\n");
  artnet_stop(node);
  artnet_destroy(node);
  return 0;
}
