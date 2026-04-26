/*
 * file_transfer.c
 * Art-Net 4 file transfer example (upload / download)
 *
 * Interactive controller that uploads a file to a node (ArtFileTnMaster)
 * or requests a file download (ArtFileFnMaster). Displays file transfer
 * progress and receives ArtFileFnReply data blocks.
 *
 * Usage: file_transfer [-i <bind_ip>]
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

static int file_fn_reply_handler(artnet_node n, void *pp, void *data) {
  (void)n;
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_file_fn_reply_t *r = &packet->data.filefnr;

  int totalLen = ((int)r->fileLengthHi << 8) | r->fileLengthLo;
  int blockId = r->blockId;

  printf("\n[FileFnReply] block=%d, totalLen=%d\n", blockId, totalLen);
  printf("  Data (first 32 bytes):");
  for (int i = 0; i < 32; i++)
    printf(" %02X", ((uint8_t *)r->data)[i]);
  printf("\n> ");
  fflush(stdout);
  return 0;
}

static void print_discovered(artnet_node n) {
  artnet_node_list nl = artnet_get_nl(n);
  int count = artnet_nl_get_length(nl);
  if (count == 0) {
    printf("  No nodes discovered. Press 'p' to poll.\n");
    return;
  }
  int idx = 0;
  artnet_node_entry entry = artnet_nl_first(nl);
  while (entry) {
    printf("  [%d] %d.%d.%d.%d  %-18s\n",
           idx, entry->ip[0], entry->ip[1], entry->ip[2], entry->ip[3],
           (char *)entry->shortName);
    entry = artnet_nl_next(nl);
    idx++;
  }
}

static artnet_node_entry select_node(artnet_node n) {
  artnet_node_list nl = artnet_get_nl(n);
  int count = artnet_nl_get_length(nl);
  if (count == 0) {
    printf("No nodes discovered. Press 'p' to poll.\n");
    return NULL;
  }
  print_discovered(n);
  printf("  Select node [0-%d]: ", count - 1);
  fflush(stdout);

  char buf[16];
  if (!fgets(buf, sizeof(buf), stdin)) return NULL;
  int idx = atoi(buf);
  if (idx < 0 || idx >= count) {
    printf("Invalid index.\n");
    return NULL;
  }

  artnet_node_entry entry = artnet_nl_first(nl);
  for (int i = 0; i < idx && entry; i++)
    entry = artnet_nl_next(nl);
  return entry;
}

static void print_menu(void) {
  printf("\n--- File Transfer ---\n");
  printf("  p) Poll network\n");
  printf("  l) List nodes\n");
  printf("  u) Upload file to node\n");
  printf("  d) Download file from node\n");
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
      printf("Usage: file_transfer [-i <bind_ip>]\n");
      return 1;
    }
  }

  printf("Art-Net 4 File Transfer\n");
  printf("  Bind IP: %s\n\n", ip ? ip : "auto");

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_SRV);
  artnet_set_style_code(node, ARTNET_ST_CONTROLLER);
  artnet_set_short_name(node, "FileTransfer");
  artnet_set_long_name(node, "libartnet Art-Net 4 File Transfer");

  artnet_set_handler(node, ARTNET_FILE_FN_REPLY_HANDLER, file_fn_reply_handler, NULL);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("File transfer controller started.\n");
  artnet_send_poll(node, NULL, ARTNET_TTM_AUTO);

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  char cmd[256];
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
      case 'l':
        print_discovered(node);
        break;
      case 'u': {
        artnet_node_entry e = select_node(node);
        if (!e) break;

        printf("  File data (hex, e.g. 48 45 4C 4C 4F): ");
        fflush(stdout);
        char buf[1024];
        if (!fgets(buf, sizeof(buf), stdin)) break;

        uint16_t data[512];
        int dataLen = 0;
        char *p = buf;
        while (*p && dataLen < 512) {
          while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
          if (!*p) break;
          unsigned int byte;
          if (sscanf(p, "%x", &byte) != 1) break;
          data[dataLen++] = (uint16_t)byte;
          while (*p && *p != ' ' && *p != '\t') p++;
        }

        int ret = artnet_send_file_tn_master(node, e, 0x00, 0,
                                              (uint32_t)(dataLen * 2),
                                              data, dataLen);
        printf("[FileTnMaster] %s (%d bytes, 1 block)\n",
               ret == ARTNET_EOK ? "OK" : artnet_strerror(), dataLen * 2);
        break;
      }
      case 'd': {
        artnet_node_entry e = select_node(node);
        if (!e) break;

        printf("  Filename: ");
        fflush(stdout);
        char fname[256];
        if (!fgets(fname, sizeof(fname), stdin)) break;
        fname[strcspn(fname, "\n\r")] = '\0';
        if (fname[0] == '\0') {
          printf("Cancelled.\n");
          break;
        }

        int ret = artnet_send_file_fn_master(node, e, fname);
        printf("[FileFnMaster] Request '%s': %s\n",
               fname, ret == ARTNET_EOK ? "OK" : artnet_strerror());
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
