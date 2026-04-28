/*
 * directory_query.c
 * Art-Net 4 directory query example
 *
 * Interactive controller that sends ArtDirectory to discover node file lists
 * and displays ArtDirectoryReply responses.
 *
 * Usage: directory_query [-i <bind_ip>]
 *   -i  IP address to bind to (default: first non-loopback interface)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <artnet/artnet.h>
#include <artnet/packets.h>
#include <artnet/common.h>

static volatile int running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static int dir_reply_handler(artnet_node n, void *pp, void *data) {
  (void)n;
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_directory_reply_t *r = &packet->data.dirr;

  int count = ((int)r->dirCountHi << 8) | r->dirCountLo;
  int total = ((int)r->dirTotalHi << 8) | r->dirTotalLo;

  printf("\n[DirectoryReply] entries=%d, total=%d\n", count, total);
  if (count > 0) {
    /* dirEntry is raw binary data, print as hex/text */
    int show = count > 128 ? 128 : count;
    printf("  ");
    for (int i = 0; i < show; i++) {
      uint8_t c = r->dirEntry[i];
      if (c >= 0x20 && c < 0x7F)
        printf("%c", c);
      else
        printf("\\x%02X", c);
    }
    if (count > 128) printf("...");
    printf("\n");
  }
  printf("> ");
  fflush(stdout);
  return 0;
}

static void print_menu(void) {
  printf("\n--- Directory Query ---\n");
  printf("  p) Poll network\n");
  printf("  d) Send ArtDirectory (query file lists)\n");
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
      printf("Usage: directory_query [-i <bind_ip>]\n");
      return 1;
    }
  }

  printf("Art-Net 4 Directory Query\n");
  printf("  Bind IP: %s\n\n", ip ? ip : "auto");

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_SRV);
  artnet_set_style_code(node, ARTNET_ST_CONTROLLER);
  artnet_set_short_name(node, "DirQuery");
  artnet_set_long_name(node, "libartnet Art-Net 4 Directory Query");

  artnet_set_handler(node, ARTNET_DIRECTORY_REPLY_HANDLER, dir_reply_handler, NULL);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("Directory query controller started.\n");
  artnet_send_poll(node, NULL, ARTNET_TTM_AUTO);

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  char cmd[64];
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
      case 'd': {
        int ret = artnet_send_directory(node);
        printf("[Directory] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
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
