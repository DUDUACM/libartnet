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
#include <stdint.h>
#if !defined(_WIN32) && !defined(_MSC_VER)
#include <arpa/inet.h>
#endif
#include <artnet/artnet.h>
#include <artnet/packets.h>
#include <artnet/common.h>

#define DEFAULT_NET      0
#define DEFAULT_SUBNET   0
#define DEFAULT_UNIVERSE 0
#define NUM_PORTS        ARTNET_MAX_PORTS

static volatile int running = 1;

typedef struct {
  const char *name;
  const uint8_t *data;
  size_t length;
} mem_file_t;

static const uint8_t k_full_node_log[] =
  "boot=ok\n"
  "dmx=enabled\n"
  "rdm=enabled\n";

static const uint8_t k_scene_a_dmx[] = {
  0x7F, 0x40, 0x20, 0x10, 0x00, 0xFF, 0xAA, 0x55
};

static const uint8_t k_product_note[] =
  "libartnet full_node in-memory file\n";

static const mem_file_t k_mem_files[] = {
  {"full_node.log", k_full_node_log, sizeof(k_full_node_log) - 1},
  {"scene_A.dmx",   k_scene_a_dmx,   sizeof(k_scene_a_dmx)},
  {"readme.txt",    k_product_note,  sizeof(k_product_note) - 1},
};

/**
 * Build a null-separated directory blob from the in-memory file table.
 *
 * @param buf output buffer
 * @param cap output buffer capacity in bytes
 * @return number of bytes written to buf
 */
static int build_directory_blob(uint8_t *buf, size_t cap) {
  size_t used = 0;
  size_t i = 0;

  for (i = 0; i < sizeof(k_mem_files) / sizeof(k_mem_files[0]); i++) {
    size_t name_len = strlen(k_mem_files[i].name) + 1;
    if (used + name_len > cap) {
      break;
    }
    memcpy(buf + used, k_mem_files[i].name, name_len);
    used += name_len;
  }
  return (int)used;
}

static const mem_file_t *find_mem_file(const char *name) {
  size_t i = 0;
  for (i = 0; i < sizeof(k_mem_files) / sizeof(k_mem_files[0]); i++) {
    if (strcmp(k_mem_files[i].name, name) == 0) {
      return &k_mem_files[i];
    }
  }
  return NULL;
}

/* ---- Packet handlers ---- */

static int dmx_handler(artnet_node n, int port, void *data) {
  (void)data;
  int length;
  uint8_t *dmx = artnet_read_dmx(n, port, &length);
  if (dmx && length > 0) {
    int ch2 = (length > 1) ? dmx[1] : -1;
    int ch3 = (length > 2) ? dmx[2] : -1;
    printf("[DMX] Port %d, %d ch: ch1=%d ch2=%d ch3=%d ... ch%d=%d\n",
           port, length, dmx[0], ch2, ch3, length, dmx[length - 1]);
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
  (void)n; (void)pp; (void)data;
  printf("[TOD] ArtTodRequest received\n");
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

static int rdmsub_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_rdm_sub_t *rdmsub = &packet->data.rdmsub;
  uint16_t param_id = (uint16_t)(((uint16_t)rdmsub->paramIdHi << 8) | rdmsub->paramId);
  uint16_t sub_count = (uint16_t)(((uint16_t)rdmsub->subCountHi << 8) | rdmsub->subCount);

  printf("[RDMSub] uid=%02X:%02X:%02X:%02X:%02X:%02X cc=0x%02X pid=0x%04X subCount=%u\n",
         rdmsub->uid[0], rdmsub->uid[1], rdmsub->uid[2],
         rdmsub->uid[3], rdmsub->uid[4], rdmsub->uid[5],
         rdmsub->commandClass, param_id, sub_count);
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
         ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
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

/**
 * Handle ArtIpProg packets by printing the requested settings.
 *
 * @param n    the artnet_node
 * @param pp   pointer to the received packet
 * @param data unused callback data
 * @return always 0
 */
static int ipprog_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_ipprog_t *ipg = &packet->data.aip;
  printf("[IpProg] cmd=0x%02X ip=%d.%d.%d.%d mask=%d.%d.%d.%d gw=%d.%d.%d.%d\n",
         ipg->Command,
         ipg->ProgIpHi, ipg->ProgIp2, ipg->ProgIp1, ipg->ProgIpLo,
         ipg->ProgSmHi, ipg->ProgSm2, ipg->ProgSm1, ipg->ProgSmLo,
         ipg->ProgDgHi, ipg->ProgDg2, ipg->ProgDg1, ipg->ProgDgLo);
  return 0;
}

/**
 * Handle ArtCommand packets by printing the received command text.
 *
 * @param n    the artnet_node
 * @param pp   pointer to the received packet
 * @param data unused callback data
 * @return always 0
 */
static int command_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_command_t *cmd = &packet->data.cmd;
  int len = ((int)cmd->lengthHi << 8) | cmd->lengthLo;
  printf("[Command] esta=%02X%02X text=%.*s\n",
         cmd->estaManHi, cmd->estaManLo,
         len > 0 ? len - 1 : 0, (char *)cmd->data);
  return 0;
}

/**
 * Handle ArtMedia packets.
 *
 * @param n    the artnet_node
 * @param pp   pointer to the received packet
 * @param data unused callback data
 * @return always 0
 */
static int media_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)pp; (void)data;
  printf("[Media] ArtMedia received\n");
  return 0;
}

/**
 * Handle ArtMediaPatch packets by printing basic routing information.
 *
 * @param n    the artnet_node
 * @param pp   pointer to the received packet
 * @param data unused callback data
 * @return always 0
 */
static int media_patch_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_media_patch_t *mp = &packet->data.mpatch;
  int len = ((int)mp->lengthHi << 8) | mp->length;
  printf("[MediaPatch] physical=%d universe=0x%04X len=%d\n",
         mp->physical, mp->universe, len);
  return 0;
}

/**
 * Handle ArtMediaControl packets.
 *
 * @param n    the artnet_node
 * @param pp   pointer to the received packet
 * @param data unused callback data
 * @return always 0
 */
static int media_control_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)pp; (void)data;
  printf("[MediaControl] ArtMediaControl received\n");
  return 0;
}

/**
 * Handle ArtMediaControlReply packets.
 *
 * @param n    the artnet_node
 * @param pp   pointer to the received packet
 * @param data unused callback data
 * @return always 0
 */
static int media_control_reply_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)pp; (void)data;
  printf("[MediaControlReply] ArtMediaControlReply received\n");
  return 0;
}

static int data_request_handler(artnet_node n, void *pp, void *data) {
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  uint16_t request_code = (uint16_t)((packet->data.datareq.requestHi << 8) |
                                     packet->data.datareq.requestLo);
  static const char k_product_url[] = "https://example.invalid/libartnet/full_node";
  static const char k_user_guide_url[] = "https://example.invalid/libartnet/docs";
  static const char k_support_url[] = "https://example.invalid/libartnet/support";
  char ip_buf[16];
  const char *ip_txt = inet_ntoa(packet->from);
  snprintf(ip_buf, sizeof(ip_buf), "%s", ip_txt ? ip_txt : "0.0.0.0");

  printf("[DataRequest] code=0x%04X from %s\n", request_code, ip_buf);

  switch (request_code) {
    case ARTNET_DR_POLL:
      artnet_send_data_reply(n, ip_buf, request_code, "", 1);
      break;
    case ARTNET_DR_URL_PRODUCT:
      artnet_send_data_reply(n, ip_buf, request_code, k_product_url,
                             (int16_t)(strlen(k_product_url) + 1));
      break;
    case ARTNET_DR_URL_USER_GUIDE:
      artnet_send_data_reply(n, ip_buf, request_code, k_user_guide_url,
                             (int16_t)(strlen(k_user_guide_url) + 1));
      break;
    case ARTNET_DR_URL_SUPPORT:
      artnet_send_data_reply(n, ip_buf, request_code, k_support_url,
                             (int16_t)(strlen(k_support_url) + 1));
      break;
    default:
      artnet_send_data_reply(n, ip_buf, request_code, "", 1);
      break;
  }
  return 0;
}

static int directory_handler(artnet_node n, void *pp, void *data) {
  uint8_t dir_blob[256];
  int len = 0;
  (void)pp; (void)data;
  printf("[Directory] ArtDirectory received\n");
  len = build_directory_blob(dir_blob, sizeof(dir_blob));
  artnet_send_directory_reply(n,
                              dir_blob,
                              len,
                              (int)(sizeof(k_mem_files) / sizeof(k_mem_files[0])));
  return 1;
}

static int file_fn_master_handler(artnet_node n, void *pp, void *data) {
  (void)data;
  artnet_packet packet = (artnet_packet)pp;
  const mem_file_t *file = find_mem_file((const char *)packet->data.filefn.filename);
  int block_bytes = ARTNET_FIRMWARE_SIZE * (int)sizeof(uint16_t);
  int total = 0;
  int block_id = 0;
  printf("[FileFnMaster] request received for %.256s\n", (char *)packet->data.filefn.filename);
  if (!file) {
    static const uint8_t not_found[] = "NOT_FOUND";
    artnet_send_file_fn_reply(n, 0, (uint16_t)(sizeof(not_found) - 1),
                              (uint8_t *)not_found, (int)(sizeof(not_found) - 1));
    return 1;
  }
  total = (int)file->length;
  while (block_id * block_bytes < total) {
    int offset = block_id * block_bytes;
    int remaining = total - offset;
    int send_len = remaining > block_bytes ? block_bytes : remaining;
    artnet_send_file_fn_reply(n, (uint8_t)block_id, (uint16_t)total,
                              (uint8_t *)(file->data + offset), send_len);
    block_id++;
  }
  return 1;
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
  artnet_set_handler(node, ARTNET_RDMSUB_HANDLER, rdmsub_handler, NULL);
  artnet_set_handler(node, ARTNET_NZS_HANDLER, nzs_handler, NULL);
  artnet_set_handler(node, ARTNET_TIMECODE_HANDLER, timecode_handler, NULL);
  artnet_set_handler(node, ARTNET_TIMESYNC_HANDLER, timesync_handler, NULL);
  artnet_set_handler(node, ARTNET_TRIGGER_HANDLER, trigger_handler, NULL);
  artnet_set_handler(node, ARTNET_DIAGDATA_HANDLER, diag_handler, NULL);
  artnet_set_handler(node, ARTNET_ADDRESS_HANDLER, address_handler, NULL);
  artnet_set_handler(node, ARTNET_INPUT_HANDLER, input_handler, NULL);
  artnet_set_handler(node, ARTNET_IPPROG_HANDLER, ipprog_handler, NULL);
  artnet_set_handler(node, ARTNET_COMMAND_HANDLER, command_handler, NULL);
  artnet_set_handler(node, ARTNET_MEDIA_HANDLER, media_handler, NULL);
  artnet_set_handler(node, ARTNET_MEDIAPATCH_HANDLER, media_patch_handler, NULL);
  artnet_set_handler(node, ARTNET_MEDIACONTROL_HANDLER, media_control_handler, NULL);
  artnet_set_handler(node, ARTNET_MEDIACONTROL_REPLY_HANDLER, media_control_reply_handler, NULL);
  artnet_set_handler(node, ARTNET_DATAREQUEST_HANDLER, data_request_handler, NULL);
  artnet_set_handler(node, ARTNET_DIRECTORY_HANDLER, directory_handler, NULL);
  artnet_set_handler(node, ARTNET_FILE_FN_MASTER_HANDLER, file_fn_master_handler, NULL);
  artnet_set_program_handler(node, program_handler, NULL);
  artnet_set_firmware_handler(node, firmware_handler, NULL);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: failed to start node: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("\nFull node started, all handlers active. (Ctrl+C to stop)\n");
  printf("Supported: DMX, RDM, TOD, Sync, TimeCode, TimeSync, Trigger,\n");
  printf("           Nzs, Firmware, Diagnostics, Remote Programming,\n");
  printf("           IpProg, Command, MediaPatch, MediaControl,\n");
  printf("           DataRequest, Directory, FileFnMaster\n");
  printf("Files:     ");
  for (i = 0; i < (int)(sizeof(k_mem_files) / sizeof(k_mem_files[0])); i++) {
    printf("%s%s", k_mem_files[i].name,
           (i + 1 < (int)(sizeof(k_mem_files) / sizeof(k_mem_files[0]))) ? ", " : "\n\n");
  }

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
        dmx_data[3] = 0;
        artnet_send_dmx(node, i, 4, dmx_data);
      }
    }
  }

  printf("\nStopping...\n");
  artnet_stop(node);
  artnet_destroy(node);
  return 0;
}
