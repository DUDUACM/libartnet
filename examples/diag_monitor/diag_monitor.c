/*
 * diag_monitor.c
 * Art-Net 4 diagnostic / event monitor example
 *
 * Listens for ArtDiagData, ArtTimeSync, ArtTrigger, ArtCommand packets
 * and prints their contents. Useful for monitoring network activity.
 *
 * Usage: diag_monitor [-i <bind_ip>]
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

static const char *diag_priority_name(uint8_t pri) {
  switch (pri) {
    case ARTNET_DIAG_LOW:      return "LOW";
    case ARTNET_DIAG_MEDIUM:   return "MED";
    case ARTNET_DIAG_HIGH:     return "HIGH";
    case ARTNET_DIAG_CRITICAL: return "CRIT";
    case ARTNET_DIAG_VOLATILE: return "VOL";
    default:                   return "???";
  }
}

static int diag_handler(artnet_node n, void *pp, void *data) {
  (void)n;
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_diagdata_t *d = &packet->data.diagdata;

  int len = ((int)d->lengthHi << 8) | d->length;
  if (len > ARTNET_DMX_LENGTH) len = ARTNET_DMX_LENGTH;

  printf("[Diag] pri=%-4s port=%d: %.*s\n",
         diag_priority_name(d->diagPriority), d->logicalPort,
         len > 0 ? len - 1 : 0, (char *)d->data);
  return 0;
}

static int timesync_handler(artnet_node n, void *pp, void *data) {
  (void)n;
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_timesync_t *ts = &packet->data.tsync;

  printf("[TimeSync] %04d-%02d-%02d %02d:%02d:%02d\n",
         1900 + ts->tm_year, ts->tm_mon + 1, ts->tm_mday,
         ts->tm_hour, ts->tm_min, ts->tm_sec);
  return 0;
}

static int trigger_handler(artnet_node n, void *pp, void *data) {
  (void)n;
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_trigger_t *tr = &packet->data.trigger;

  const char *key_name;
  switch (tr->key) {
    case ARTNET_TRIGGER_KEY_ASCII: key_name = "ASCII"; break;
    case ARTNET_TRIGGER_KEY_MACRO: key_name = "Macro"; break;
    case ARTNET_TRIGGER_KEY_SOFT:  key_name = "Soft"; break;
    case ARTNET_TRIGGER_KEY_SHOW:  key_name = "Show"; break;
    default:                       key_name = "Unknown"; break;
  }

  printf("[Trigger] OEM=%02X%02X key=%s sub=%d\n",
         tr->oemCodeHi, tr->oemCodeLo, key_name, tr->subKey);
  return 0;
}

static int command_handler(artnet_node n, void *pp, void *data) {
  (void)n;
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_command_t *cmd = &packet->data.cmd;

  int len = ((int)cmd->lengthHi << 8) | cmd->lengthLo;
  if (len > ARTNET_DMX_LENGTH) len = ARTNET_DMX_LENGTH;

  printf("[Command] estaMan=%02X%02X len=%d: %.*s\n",
         cmd->estaManHi, cmd->estaManLo, len, len, (char *)cmd->data);
  return 0;
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char *ip = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      ip = argv[++i];
    } else {
      printf("Usage: diag_monitor [-i <bind_ip>]\n");
      return 1;
    }
  }

  printf("Art-Net 4 Diagnostic Monitor\n");
  printf("  Bind IP: %s\n", ip ? ip : "auto");
  printf("  Monitoring: DiagData, TimeSync, Trigger, Command\n\n");

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_NODE);
  artnet_set_style_code(node, ARTNET_ST_NODE);
  artnet_set_short_name(node, "DiagMonitor");
  artnet_set_long_name(node, "libartnet Art-Net 4 Diagnostic Monitor");

  artnet_set_handler(node, ARTNET_DIAGDATA_HANDLER, diag_handler, NULL);
  artnet_set_handler(node, ARTNET_TIMESYNC_HANDLER, timesync_handler, NULL);
  artnet_set_handler(node, ARTNET_TRIGGER_HANDLER, trigger_handler, NULL);
  artnet_set_handler(node, ARTNET_COMMAND_HANDLER, command_handler, NULL);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("Monitor started, listening... (Ctrl+C to stop)\n");

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
