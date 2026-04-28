/*
 * full_node.c
 * Art-Net 4 full-featured node example (4-port bidirectional)
 *
 * A complete Art-Net 4 node with:
 *   - 4 input ports (DMX output) + 4 output ports (DMX input)
 *   - RDM device discovery (TOD) and RDM data handling
 *   - Remote programming via ArtAddress/ArtInput
 *   - ArtSync, ArtTimeCode, ArtTimeSync, ArtTrigger, ArtNzs receive
 *   - Firmware upload reception
 *   - Diagnostic message reception
 *   - Fail-safe mode (hold/zero/full/scene)
 *   - ArtPollReply on change
 *
 * Usage: full_node [-i <bind_ip>] [-n <net>] [-s <subnet>] [-u <universe>]
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
#include <artnet/packets.h>
#include <artnet/common.h>

#define DEFAULT_NET      0
#define DEFAULT_SUBNET   0
#define DEFAULT_UNIVERSE 0
#define NUM_PORTS        ARTNET_MAX_PORTS

static volatile int running = 1;

/* ---- Packet handlers ---- */

static int dmx_handler(artnet_node n, int port, void *data) {
  (void)data;
  int length;
  uint8_t *dmx = artnet_read_dmx(n, port, &length);
  if (dmx && length > 0) {
    printf("[DMX] Port %d, %d ch: ch1=%d ch2=%d ch3=%d ... ch%d=%d\n",
           port, length, dmx[0], dmx[1], dmx[2], length, dmx[length - 1]);
  }
  return 0;
}

static int sync_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)pp; (void)data;
  printf("[Sync] ArtSync received - frame complete\n");
  return 0;
}

static int program_handler(artnet_node n, void *data) {
  (void)data;
  printf("\n=== Remote Programming Applied ===\n");
  artnet_dump_config(n);
  printf("===================================\n\n");
  return 0;
}

static int poll_handler(artnet_node n, void *pp, void *data) {
  (void)pp; (void)data;
  printf("[Poll] ArtPoll received from controller\n");
  return 0;
}

static int reply_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_reply_t *r = &packet->data.ar;

  printf("[Reply] %.18s at %d.%d.%d.%d  Net %d Sub %d  style=0x%02X\n",
         (char *)r->shortName,
         r->ip[0], r->ip[1], r->ip[2], r->ip[3],
         r->netSwitch, r->subSwitch & 0x0F, r->style);
  return 0;
}

static int tod_request_handler(artnet_node n, void *pp, void *data) {
  (void)pp; (void)data;
  printf("[TOD] ArtTodRequest received, sending TOD data\n");
  for (int i = 0; i < NUM_PORTS; i++)
    artnet_send_tod_data(n, i);
  return 0;
}

static int tod_data_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_toddata_t *tod = &packet->data.toddata;
  printf("[TOD] TodData net=%d addr=0x%02X uidCount=%d\n",
         tod->net, tod->address, tod->uidCount);
  return 0;
}

static int tod_control_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_todcontrol_t *tc = &packet->data.todcontrol;
  printf("[TOD] TodControl net=%d addr=0x%02X action=0x%02X\n",
         tc->net, tc->address, tc->cmd);
  return 0;
}

static int rdm_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_rdm_t *rdm = &packet->data.rdm;
  printf("[RDM] net=%d addr=0x%02X cmd=0x%02X\n",
         rdm->net, rdm->address, rdm->cmd);
  return 0;
}

static int nzs_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_nzs_t *nzs = &packet->data.nzs;
  printf("[Nzs] universe=0x%04X startCode=0x%02X, %d bytes\n",
         nzs->universe, nzs->startCode, (int)((nzs->lengthHi << 8) | nzs->length));
  return 0;
}

static int timecode_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_timecode_t *tc = &packet->data.tc;
  const char *type_str = tc->type == 0 ? "Film24" :
                         tc->type == 1 ? "EBU25" :
                         tc->type == 2 ? "DF29" : "SMPTE30";
  printf("[TimeCode] %02d:%02d:%02d.%02d type=%s stream=%d\n",
         tc->hours, tc->minutes, tc->seconds, tc->frames,
         type_str, tc->streamId);
  return 0;
}

static int timesync_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_timesync_t *ts = &packet->data.tsync;
  printf("[TimeSync] %04d-%02d-%02d %02d:%02d:%02d\n",
         ts->tm_year + 1900, ts->tm_mon, ts->tm_mday,
         ts->tm_hour, ts->tm_min, ts->tm_sec);
  return 0;
}

static int trigger_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_trigger_t *tr = &packet->data.trigger;
  const char *key_str = tr->key == 0 ? "ASCII" :
                        tr->key == 1 ? "Macro" :
                        tr->key == 2 ? "Soft" : "Show";
  printf("[Trigger] OEM=0x%02X%02X key=%s subKey=%d\n",
         tr->oemCodeHi, tr->oemCodeLo, key_str, tr->subKey);
  return 0;
}

static int diag_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_diagdata_t *d = &packet->data.diagdata;
  printf("[Diag] priority=0x%02X port=%d: %.512s\n",
         d->diagPriority, d->logicalPort, (char *)d->data);
  return 0;
}

static int firmware_handler(artnet_node n, int ubea, uint16_t *fw_data, int length, void *d) {
  (void)n; (void)d;
  printf("[Firmware] Transfer complete: %s, %d words\n",
         ubea ? "UBEA" : "Firmware", length);
  return 0;
}

static int address_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)pp; (void)data;
  printf("[Address] ArtAddress received\n");
  return 0;
}

static int input_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)pp; (void)data;
  printf("[Input] ArtInput received (port enable/disable)\n");
  return 0;
}

/* ---- Helpers ---- */

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

/* ---- Main ---- */

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

  if (net < 0 || net > 127) { printf("Error: net must be 0-127\n"); return 1; }
  if (subnet < 0 || subnet > 15) { printf("Error: subnet must be 0-15\n"); return 1; }
  if (universe < 0 || universe > 15) { printf("Error: port address must be 0-15\n"); return 1; }

  printf("Art-Net 4 Full Node (%d bidirectional ports)\n", NUM_PORTS);
  printf("  Bind IP  : %s\n", ip ? ip : "auto");
  printf("  Base Addr: Net %d / Subnet %d / Port %d\n\n", net, subnet, universe);

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: failed to create node: %s\n", artnet_strerror());
    return 1;
  }

  /* Node identity */
  artnet_set_node_type(node, ARTNET_NODE);
  artnet_set_style_code(node, ARTNET_ST_NODE);
  artnet_set_short_name(node, "FullNode");
  artnet_set_long_name(node, "libartnet Art-Net 4 Full Feature Node");
  artnet_setoem(node, 0xFF, 0x00);
  artnet_setesta(node, 0x00, 0xFF);
  artnet_set_net_addr(node, (uint8_t)net);
  artnet_set_subnet_addr(node, (uint8_t)subnet);

  /* Status registers */
  artnet_set_status2(node, ARTNET_STATUS2_15BIT_ADDR | ARTNET_STATUS2_RDM_CONTROL);

  /* RDM default responder UID */
  uint8_t uid[ARTNET_RDM_UID_WIDTH] = {0x00, 0xFF, 0x00, 0x00, 0x00, 0x01};
  artnet_set_default_resp_uid(node, uid);

  /* Configure 4 bidirectional ports (input = DMX output, output = DMX input) */
  for (i = 0; i < NUM_PORTS; i++) {
    uint8_t port_addr = (uint8_t)(universe + i);
    uint16_t full_addr = (uint16_t)((net << 8) | (subnet << 4) | port_addr);
    artnet_set_port_type(node, i,
                         ARTNET_ENABLE_INPUT | ARTNET_ENABLE_OUTPUT,
                         ARTNET_PORT_DMX);
    artnet_set_port_addr(node, i, ARTNET_INPUT_PORT, port_addr);
    artnet_set_port_addr(node, i, ARTNET_OUTPUT_PORT, port_addr);
    printf("  Port %d -> 0x%04X (Net %d Sub %d Port %d) [IN+OUT]\n",
           i, full_addr, (full_addr >> 8) & 0x7F,
           (full_addr >> 4) & 0x0F, full_addr & 0x0F);
  }

  /* Add a sample RDM device to port 0 */
  uint8_t rdm_uid[ARTNET_RDM_UID_WIDTH] = {0x00, 0xFF, 0x00, 0x00, 0x00, 0x01};
  artnet_add_rdm_device(node, 0, rdm_uid);

  /* Register all handlers */
  artnet_set_dmx_handler(node, dmx_handler, NULL);
  artnet_set_handler(node, ARTNET_SYNC_HANDLER, sync_handler, NULL);
  artnet_set_handler(node, ARTNET_POLL_HANDLER, poll_handler, NULL);
  artnet_set_handler(node, ARTNET_REPLY_HANDLER, reply_handler, NULL);
  artnet_set_handler(node, ARTNET_TOD_REQUEST_HANDLER, tod_request_handler, NULL);
  artnet_set_handler(node, ARTNET_TOD_DATA_HANDLER, tod_data_handler, NULL);
  artnet_set_handler(node, ARTNET_TOD_CONTROL_HANDLER, tod_control_handler, NULL);
  artnet_set_handler(node, ARTNET_RDM_HANDLER, rdm_handler, NULL);
  artnet_set_handler(node, ARTNET_NZS_HANDLER, nzs_handler, NULL);
  artnet_set_handler(node, ARTNET_TIMECODE_HANDLER, timecode_handler, NULL);
  artnet_set_handler(node, ARTNET_TIMESYNC_HANDLER, timesync_handler, NULL);
  artnet_set_handler(node, ARTNET_TRIGGER_HANDLER, trigger_handler, NULL);
  artnet_set_handler(node, ARTNET_DIAGDATA_HANDLER, diag_handler, NULL);
  artnet_set_handler(node, ARTNET_ADDRESS_HANDLER, address_handler, NULL);
  artnet_set_handler(node, ARTNET_INPUT_HANDLER, input_handler, NULL);
  artnet_set_program_handler(node, program_handler, NULL);
  artnet_set_firmware_handler(node, firmware_handler, NULL);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: failed to start node: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("\nFull node started, all handlers active. (Ctrl+C to stop)\n");
  printf("Supported: DMX, RDM, TOD, Sync, TimeCode, TimeSync, Trigger,\n");
  printf("           Nzs, Firmware, Diagnostics, Remote Programming\n\n");

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  uint8_t dmx_data[ARTNET_DMX_LENGTH];
  int tick = 0;

  while (running) {
    artnet_read(node, 100);
    tick++;

    /* Send a test DMX frame on each input port every ~5 seconds (50 ticks * 100ms) */
    if (tick % 50 == 0) {
      memset(dmx_data, 0, sizeof(dmx_data));
      for (i = 0; i < NUM_PORTS; i++) {
        dmx_data[0] = (uint8_t)((tick / 50 + i * 30) & 0xFF);
        dmx_data[1] = (uint8_t)((tick / 50 + i * 60) & 0xFF);
        dmx_data[2] = (uint8_t)((tick / 50 + i * 90) & 0xFF);
        artnet_send_dmx(node, i, 3, dmx_data);
      }
    }
  }

  printf("\nStopping...\n");
  artnet_stop(node);
  artnet_destroy(node);
  return 0;
}
