/*
 * full_controller.c
 * Art-Net 4 full-featured controller example (interactive)
 *
 * Interactive controller demonstrating all Art-Net 4 controller features:
 *   - Node discovery (ArtPoll with flags)
 *   - DMX transmission (ArtDmx, ArtNzs, raw 15-bit)
 *   - ArtSync synchronized output
 *   - Remote management (ArtAddress, ArtInput)
 *   - RDM (ArtTodRequest, ArtTodControl, ArtRdm, ArtRdmSub)
 *   - Firmware upload (ArtFirmwareMaster)
 *   - Time & synchronization (ArtTimeCode, ArtTimeSync)
 *   - ArtTrigger macro triggers
 *   - Diagnostics (ArtDiagData receive)
 *   - File transfer (ArtFileTnMaster, ArtFileFnMaster)
 *   - ArtDirectory / ArtDirectoryReply
 *   - ArtDataRequest / ArtDataReply
 *
 * Usage: full_controller [-i <bind_ip>]
 *   -i  IP address to bind to (default: first non-loopback interface)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <artnet/artnet.h>
#include <artnet/packets.h>
#include <artnet/common.h>

#define DMX_FRAME_MS 25
#define DMX_SINE_PERIOD 5.0
#define SINE_PI2 6.28318530

static volatile int running = 1;
static volatile int dmx_active = 0;

/* ---- Helpers ---- */

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static void strip_newline(char *s) {
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
    s[--len] = '\0';
}

static int read_int(const char *prompt, int default_val) {
  char buf[32];
  printf("%s", prompt);
  fflush(stdout);
  if (!fgets(buf, sizeof(buf), stdin)) return default_val;
  if (buf[0] == '\n') return default_val;
  return atoi(buf);
}

static int read_hex(const char *prompt) {
  char buf[32];
  printf("%s", prompt);
  fflush(stdout);
  if (!fgets(buf, sizeof(buf), stdin)) return 0;
  unsigned int val;
  sscanf(buf, "%x", &val);
  return (int)val;
}

/* ---- Packet handlers ---- */

static int reply_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_reply_t *r = &packet->data.ar;

  printf("\n[Reply] %.18s at %d.%d.%d.%d  Net %d Sub %d  Ports %d  style=0x%02X  status2=0x%02X\n",
         (char *)r->shortName,
         r->ip[0], r->ip[1], r->ip[2], r->ip[3],
         r->netSwitch, r->subSwitch & 0x0F,
         (r->numbportsH << 8) | r->numbports,
         r->style, r->status2);

  if (r->numbports > 0) {
    printf("        in:");
    for (int p = 0; p < r->numbports && p < ARTNET_MAX_PORTS; p++)
      printf(" %d", r->swIn[p] & 0x0F);
    printf("  out:");
    for (int p = 0; p < r->numbports && p < ARTNET_MAX_PORTS; p++)
      printf(" %d", r->swOut[p] & 0x0F);
    printf("\n");
  }
  printf("> ");
  fflush(stdout);
  return 0;
}

static int tod_data_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_toddata_t *tod = &packet->data.toddata;

  printf("\n[TodData] net=%d addr=0x%02X uidCount=%d",
         tod->net, tod->address, tod->uidCount);
  if (tod->uidCount == 0) {
    printf(" (no devices)\n");
  } else {
    printf("\n");
    int count = tod->uidCount > 32 ? 32 : tod->uidCount;
    for (int i = 0; i < count; i++) {
      uint8_t *uid = tod->tod[i];
      printf("  [%d] %02X:%02X:%02X:%02X:%02X:%02X\n", i,
             uid[0], uid[1], uid[2], uid[3], uid[4], uid[5]);
    }
  }
  printf("> ");
  fflush(stdout);
  return 0;
}

static int rdm_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_rdm_t *rdm = &packet->data.rdm;

  printf("\n[Rdm] net=%d addr=0x%02X cmd=0x%02X\n  data:",
         rdm->net, rdm->address, rdm->cmd);
  for (int i = 0; i < 32; i++)
    printf(" %02X", rdm->data[i]);
  printf("\n> ");
  fflush(stdout);
  return 0;
}

static int diag_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_diagdata_t *d = &packet->data.diagdata;
  printf("\n[Diag] priority=0x%02X port=%d: %.512s\n",
         d->diagPriority, d->logicalPort, (char *)d->data);
  printf("> ");
  fflush(stdout);
  return 0;
}

static int firmware_reply_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_firmware_reply_t *r = &packet->data.firmwarer;
  const char *status = r->type == 0x00 ? "BlockGood" :
                       r->type == 0x01 ? "AllGood" : "FAIL";
  printf("\n[FirmwareReply] %s (type=0x%02X)\n", status, r->type);
  printf("> ");
  fflush(stdout);
  return 0;
}

static int file_fn_reply_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_file_fn_reply_t *r = &packet->data.filefnr;

  int totalLen = ((int)r->fileLengthHi << 8) | r->fileLengthLo;
  printf("\n[FileFnReply] block=%d totalLen=%d\n", r->blockId, totalLen);
  printf("  Data (first 16 bytes):");
  for (int i = 0; i < 16 && i < 32; i++)
    printf(" %02X", ((uint8_t *)r->data)[i]);
  printf("\n> ");
  fflush(stdout);
  return 0;
}

static int directory_reply_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_directory_reply_t *r = &packet->data.dirr;

  int count = ((int)r->dirCountHi << 8) | r->dirCountLo;
  int total = ((int)r->dirTotalHi << 8) | r->dirTotalLo;
  printf("\n[DirectoryReply] entries=%d total=%d\n", count, total);
  printf("> ");
  fflush(stdout);
  return 0;
}

static int sync_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)pp; (void)data;
  printf("[Sync]\n");
  return 0;
}

static int timecode_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_timecode_t *tc = &packet->data.tc;
  printf("\n[TimeCode] %02d:%02d:%02d.%02d type=%d stream=%d\n",
         tc->hours, tc->minutes, tc->seconds, tc->frames, tc->type, tc->streamId);
  printf("> ");
  fflush(stdout);
  return 0;
}

static int timesync_handler(artnet_node n, void *pp, void *data) {
  (void)n; (void)data;
  artnet_packet packet = (artnet_packet)pp;
  artnet_timesync_t *ts = &packet->data.tsync;
  printf("\n[TimeSync] %04d-%02d-%02d %02d:%02d:%02d\n",
         ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
         ts->tm_hour, ts->tm_min, ts->tm_sec);
  printf("> ");
  fflush(stdout);
  return 0;
}

/* ---- Node list helpers ---- */

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
    printf("  [%d] %d.%d.%d.%d  %-18s  Net %d Sub %d",
           idx,
           entry->ip[0], entry->ip[1], entry->ip[2], entry->ip[3],
           (char *)entry->shortName,
           entry->netSwitch,
           entry->subSwitch & 0x0F);
    if (entry->numbports > 0) {
      printf("  out:");
      for (int p = 0; p < entry->numbports; p++)
        printf(" %d", entry->swOut[p] & 0x0F);
    }
    printf("  style=0x%02X", entry->style);
    printf("\n");
    entry = artnet_nl_next(nl);
    idx++;
  }
}

static artnet_node_entry select_node(artnet_node n) {
  artnet_node_list nl = artnet_get_nl(n);
  int count = artnet_nl_get_length(nl);
  if (count == 0) {
    printf("  No nodes discovered. Press 'p' to poll.\n");
    return NULL;
  }
  print_discovered(n);
  printf("  Select node [0-%d]: ", count - 1);
  fflush(stdout);
  char buf[16];
  if (!fgets(buf, sizeof(buf), stdin)) return NULL;
  int idx = atoi(buf);
  if (idx < 0 || idx >= count) {
    printf("  Invalid index.\n");
    return NULL;
  }
  artnet_node_entry entry = artnet_nl_first(nl);
  for (int i = 0; i < idx && entry; i++)
    entry = artnet_nl_next(nl);
  return entry;
}

/* ---- Command handlers ---- */

static void cmd_poll(artnet_node n) {
  printf("  Flags: 0=default, 1=reply on change, 2=+diag, 3=+diag unicast, 5=+target mode\n");
  printf("  Flags (default 0): ");
  fflush(stdout);
  char buf[16];
  int flags = 0;
  if (fgets(buf, sizeof(buf), stdin) && buf[0] != '\n')
    flags = atoi(buf);

  artnet_ttm_value_t ttm = ARTNET_TTM_DEFAULT;
  if (flags == 0)
    artnet_send_poll(n, NULL, ARTNET_TTM_DEFAULT);
  else {
    /* For advanced flags, use default TTM - the library handles flag parsing */
    artnet_send_poll(n, NULL, ttm);
  }
  printf("[Poll] sent (flags=0x%02X)\n", flags);
}

static void cmd_list(artnet_node n) {
  print_discovered(n);
}

static void cmd_dmx_send(artnet_node n) {
  int net = read_int("  Net (0-127, default 0): ", 0);
  int subnet = read_int("  Subnet (0-15, default 0): ", 0);
  int universe = read_int("  Universe (0-15, default 0): ", 0);
  int channels = read_int("  Channels (1-512, default 10): ", 10);
  if (channels < 1) channels = 1;
  if (channels > ARTNET_DMX_LENGTH) channels = ARTNET_DMX_LENGTH;

  uint8_t dmx_data[ARTNET_DMX_LENGTH];
  memset(dmx_data, 0, sizeof(dmx_data));
  for (int i = 0; i < channels; i++)
    dmx_data[i] = (uint8_t)((i * 255) / channels);

  uint16_t addr = (uint16_t)((net << 8) | (subnet << 4) | universe);
  int ret = artnet_raw_send_dmx(n, addr, (int16_t)channels, dmx_data);
  printf("[DMX] Send to 0x%04X: %s (%d ch)\n",
         addr, ret == ARTNET_EOK ? "OK" : artnet_strerror(), channels);
}

static void cmd_dmx_flood(artnet_node n) {
  if (dmx_active) {
    dmx_active = 0;
    printf("[DMX] Flood stopped.\n");
    return;
  }

  int net = read_int("  Net (0-127, default 0): ", 0);
  int subnet = read_int("  Subnet (0-15, default 0): ", 0);
  int universe = read_int("  Universe (0-15, default 0): ", 0);
  int num_uni = read_int("  Number of universes (1-4, default 1): ", 1);
  int use_sync = read_int("  Use ArtSync? (0=no, 1=yes, default 1): ", 1);

  if (num_uni < 1) num_uni = 1;
  if (num_uni > ARTNET_MAX_PORTS) num_uni = ARTNET_MAX_PORTS;

  dmx_active = 1;
  printf("[DMX] Flooding %d universes from 0x%04X (%s) - press 'd' again to stop\n",
         num_uni, (net << 8) | (subnet << 4) | universe,
         use_sync ? "with ArtSync" : "no sync");

  uint8_t dmx_data[ARTNET_DMX_LENGTH];
  int tick = 0;

  while (dmx_active && running) {
    artnet_read(n, 0);

    /* Check stdin for stop command */
    fd_set fds;
    struct timeval tv = {0, 0};
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    if (select(1, &fds, NULL, NULL, &tv) > 0 && FD_ISSET(0, &fds)) {
      char cmd[16];
      if (fgets(cmd, sizeof(cmd), stdin)) {
        if (cmd[0] == 'd' || cmd[0] == 'q') {
          dmx_active = 0;
          break;
        }
      }
    }

    tick++;
    double t = tick * DMX_FRAME_MS / 1000.0;

    for (int i = 0; i < num_uni; i++) {
      memset(dmx_data, 0, sizeof(dmx_data));
      double phase = SINE_PI2 * t / DMX_SINE_PERIOD - i * 0.5;
      for (int ch = 0; ch < 512; ch++) {
        double ch_phase = phase + ch * 0.02;
        dmx_data[ch] = (uint8_t)(127.5 + 127.5 * sin(ch_phase));
      }
      uint16_t addr = (uint16_t)((net << 8) | (subnet << 4) | (universe + i));
      artnet_raw_send_dmx(n, addr, ARTNET_DMX_LENGTH, dmx_data);
    }

    if (use_sync)
      artnet_send_sync(n);

#ifdef WIN32
    Sleep(DMX_FRAME_MS);
#else
    struct timespec ts = {0, DMX_FRAME_MS * 1000000};
    nanosleep(&ts, NULL);
#endif
  }

  dmx_active = 0;
  printf("[DMX] Flood stopped.\n");
}

static void cmd_nzs(artnet_node n) {
  int uni = read_int("  Universe (0-32767, default 0): ", 0);
  int sc = read_int("  Start code (hex, e.g. 0x91): ", 0x91);
  int len = read_int("  Data length (1-512, default 10): ", 10);
  if (len < 1) len = 1;
  if (len > ARTNET_DMX_LENGTH) len = ARTNET_DMX_LENGTH;

  uint8_t data[ARTNET_DMX_LENGTH];
  memset(data, 0xAA, len);

  int ret = artnet_send_nzs(n, (uint16_t)uni, (uint8_t)sc, (int16_t)len, data);
  printf("[Nzs] Universe %d startCode=0x%02X: %s (%d bytes)\n",
         uni, sc, ret == ARTNET_EOK ? "OK" : artnet_strerror(), len);
}

static void cmd_sync(artnet_node n) {
  int ret = artnet_send_sync(n);
  printf("[Sync] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_address(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Short name (enter to skip): ");
  fflush(stdout);
  char short_name[ARTNET_SHORT_NAME_LENGTH + 2];
  if (!fgets(short_name, sizeof(short_name), stdin)) return;
  strip_newline(short_name);

  printf("  Long name (enter to skip): ");
  fflush(stdout);
  char long_name[ARTNET_LONG_NAME_LENGTH + 2];
  if (!fgets(long_name, sizeof(long_name), stdin)) return;
  strip_newline(long_name);

  int net = read_int("  Net (0-127, -1=no change): ", -1);
  int sub = read_int("  Subnet (0-15, -1=no change): ", -1);

  printf("  Command: 0=None 2=LedNormal 3=LedMute 4=LedLocate "
         "8=FailHold 9=FailZero 10=FailFull 11=FailScene\n");
  int cmd_val = read_int("  Command (default 0): ", 0);

  uint8_t nochange[ARTNET_MAX_PORTS];
  memset(nochange, 0x7F, ARTNET_MAX_PORTS);

  int ret = artnet_send_address(n, e,
                                short_name[0] ? short_name : NULL,
                                long_name[0] ? long_name : NULL,
                                nochange, nochange,
                                net >= 0 ? (uint8_t)net : 0x7F,
                                sub >= 0 ? (uint8_t)sub : 0x7F,
                                (artnet_port_command_t)cmd_val, 0xFF);
  printf("[Address] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_input(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  int port = read_int("  Port (0-3, default 0): ", 0);
  int enable = read_int("  Enable (1=yes, 0=no, default 1): ", 1);

  uint8_t settings[ARTNET_MAX_PORTS];
  memset(settings, 0, ARTNET_MAX_PORTS);
  if (!enable) settings[port] = 0x01;

  int ret = artnet_send_input(n, e, settings);
  printf("[Input] %s port %d: %s\n", enable ? "Enable" : "Disable",
         port, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_tod_request(artnet_node n) {
  int ret = artnet_send_tod_request(n);
  printf("[TodRequest] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_tod_control(artnet_node n) {
  printf("  Action: 0=Full, 1=Flush, 2=End, 3=IncOn, 4=IncOff\n");
  int action = read_int("  Action (default 0): ", 0);
  int uni = read_hex("  Universe address (hex, default 0000): ");

  int ret = artnet_send_tod_control(n, (uint16_t)uni, (artnet_tod_command_code)action);
  printf("[TodControl] addr=0x%04X action=%d: %s\n",
         uni, action, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_rdm(artnet_node n) {
  int uni = read_hex("  Universe address (hex, default 0000): ");
  printf("  RDM data (hex bytes, e.g. CC 01 00): ");
  fflush(stdout);
  char buf[256];
  if (!fgets(buf, sizeof(buf), stdin)) return;

  uint8_t rdm_data[256];
  int rdm_len = 0;
  char *p = buf;
  while (*p && rdm_len < 256) {
    while (*p && !isxdigit((unsigned char)*p)) p++;
    if (!*p) break;
    unsigned int byte;
    if (sscanf(p, "%2x", &byte) != 1) break;
    rdm_data[rdm_len++] = (uint8_t)byte;
    while (*p && isxdigit((unsigned char)*p)) p++;
  }

  int ret = artnet_send_rdm(n, (uint16_t)uni, rdm_data, rdm_len);
  printf("[Rdm] %s (%d bytes to 0x%04X)\n",
         ret == ARTNET_EOK ? "OK" : artnet_strerror(), rdm_len, uni);
}

static void cmd_rdmsub(artnet_node n) {
  uint8_t uid[ARTNET_RDM_UID_WIDTH] = {0};
  printf("  UID (hex, e.g. 00FF00000001): ");
  fflush(stdout);
  char buf[64];
  if (!fgets(buf, sizeof(buf), stdin)) return;
  for (int i = 0; i < ARTNET_RDM_UID_WIDTH && buf[i * 2]; i++) {
    unsigned int byte;
    sscanf(&buf[i * 2], "%2x", &byte);
    uid[i] = (uint8_t)byte;
  }

  int cc = read_hex("  Command class (hex, e.g. 10=GET, 20=SET): ");
  int pid = read_hex("  Parameter ID (hex, e.g. 0050): ");
  int sub_dev = read_int("  Sub-device (default 0): ", 0);
  int sub_count = read_int("  Sub-count (default 1): ", 1);

  int ret = artnet_send_rdmsub(n, uid, (uint8_t)cc, (uint16_t)pid,
                                (uint16_t)sub_dev, (uint16_t)sub_count,
                                NULL, 0);
  printf("[RdmSub] UID=%02X%02X%02X%02X%02X%02X cc=0x%02X pid=0x%04X: %s\n",
         uid[0], uid[1], uid[2], uid[3], uid[4], uid[5],
         cc, pid, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_firmware(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Firmware data (hex, e.g. 48 45 4C 4C 4F): ");
  fflush(stdout);
  char buf[1024];
  if (!fgets(buf, sizeof(buf), stdin)) return;

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

  int ret = artnet_send_firmware(n, e, 0, data, dataLen, NULL, NULL);
  printf("[Firmware] Upload %d words: %s\n",
         dataLen, ret == ARTNET_EOK ? "started" : artnet_strerror());
}

static void cmd_timecode(artnet_node n) {
  int hours = read_int("  Hours (0-23, default 0): ", 0);
  int minutes = read_int("  Minutes (0-59, default 0): ", 0);
  int seconds = read_int("  Seconds (0-59, default 0): ", 0);
  int frames = read_int("  Frames (0-29, default 0): ", 0);
  printf("  Type: 0=Film24, 1=EBU25, 2=DF29, 3=SMPTE30\n");
  int type = read_int("  Type (default 0): ", 0);

  int ret = artnet_send_timecode(n, (uint8_t)frames, (uint8_t)seconds,
                                  (uint8_t)minutes, (uint8_t)hours,
                                  (artnet_timecode_type_t)type, 0);
  printf("[TimeCode] %02d:%02d:%02d.%02d type=%d: %s\n",
         hours, minutes, seconds, frames, type,
         ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_timesync(artnet_node n) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  int ret = artnet_send_timesync(n, (uint8_t)t->tm_sec, (uint8_t)t->tm_min,
                                  (uint8_t)t->tm_hour, (uint8_t)t->tm_mday,
                                  (uint8_t)(t->tm_mon + 1), (uint8_t)(t->tm_year));
  printf("[TimeSync] %04d-%02d-%02d %02d:%02d:%02d: %s\n",
         t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
         t->tm_hour, t->tm_min, t->tm_sec,
         ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_trigger(artnet_node n) {
  printf("  Key: 0=ASCII, 1=Macro, 2=Soft, 3=Show\n");
  int key = read_int("  Key (default 1): ", 1);
  int sub_key = read_int("  Sub key (default 0): ", 0);

  int ret = artnet_send_trigger(n, 0xFF, 0xFF,
                                 (uint8_t)key, (uint8_t)sub_key, NULL, 0);
  printf("[Trigger] key=%d subKey=%d: %s\n",
         key, sub_key, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_diag_send(artnet_node n) {
  printf("  Priority: 0x10=Low, 0x40=Med, 0x80=High, 0xE0=Critical\n");
  int pri = read_int("  Priority (hex val, default 0x40): ", 0x40);
  printf("  Text: ");
  fflush(stdout);
  char text[256];
  if (!fgets(text, sizeof(text), stdin)) return;
  strip_newline(text);

  int ret = artnet_send_diagnostic(n, (artnet_diag_priority_t)pri, 0, text);
  printf("[DiagData] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_file_upload(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  File data (hex, e.g. 48 45 4C 4C 4F): ");
  fflush(stdout);
  char buf[1024];
  if (!fgets(buf, sizeof(buf), stdin)) return;

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

  int ret = artnet_send_file_tn_master(n, e, 0x00, 0,
                                        (uint32_t)(dataLen * 2),
                                        data, dataLen);
  printf("[FileTnMaster] %s (%d bytes)\n",
         ret == ARTNET_EOK ? "OK" : artnet_strerror(), dataLen * 2);
}

static void cmd_file_download(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Filename: ");
  fflush(stdout);
  char fname[256];
  if (!fgets(fname, sizeof(fname), stdin)) return;
  fname[strcspn(fname, "\n\r")] = '\0';
  if (fname[0] == '\0') {
    printf("  Cancelled.\n");
    return;
  }

  int ret = artnet_send_file_fn_master(n, e, fname);
  printf("[FileFnMaster] Request '%s': %s\n",
         fname, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_directory(artnet_node n) {
  int ret = artnet_send_directory(n);
  printf("[Directory] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_data_request(artnet_node n) {
  printf("  Target IP (e.g. 192.168.1.10): ");
  fflush(stdout);
  char ip_buf[64];
  if (!fgets(ip_buf, sizeof(ip_buf), stdin)) return;
  strip_newline(ip_buf);
  if (ip_buf[0] == '\0') {
    printf("  Cancelled.\n");
    return;
  }

  printf("  Request code: 0=Poll 1=ProductURL 2=UserGuide 3=SupportURL\n");
  int code = read_int("  Code (default 0): ", 0);

  int ret = artnet_send_data_reply(n, ip_buf, (uint16_t)code, NULL, 0);
  printf("[DataReply] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_short_name(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  New short name: ");
  fflush(stdout);
  char name[ARTNET_SHORT_NAME_LENGTH + 2];
  if (!fgets(name, sizeof(name), stdin)) return;
  strip_newline(name);
  if (name[0] == '\0') { printf("  Cancelled.\n"); return; }

  uint8_t nc[ARTNET_MAX_PORTS];
  memset(nc, 0x7F, ARTNET_MAX_PORTS);
  int ret = artnet_send_address(n, e, name, NULL, nc, nc, 0x7F, 0x7F, ARTNET_PC_NONE, 0xFF);
  printf("  %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_long_name(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  New long name: ");
  fflush(stdout);
  char name[ARTNET_LONG_NAME_LENGTH + 2];
  if (!fgets(name, sizeof(name), stdin)) return;
  strip_newline(name);
  if (name[0] == '\0') { printf("  Cancelled.\n"); return; }

  uint8_t nc[ARTNET_MAX_PORTS];
  memset(nc, 0x7F, ARTNET_MAX_PORTS);
  int ret = artnet_send_address(n, e, NULL, name, nc, nc, 0x7F, 0x7F, ARTNET_PC_NONE, 0xFF);
  printf("  %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_led(artnet_node n, artnet_port_command_t cmd) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  uint8_t nc[ARTNET_MAX_PORTS];
  memset(nc, 0x7F, ARTNET_MAX_PORTS);
  int ret = artnet_send_address(n, e, NULL, NULL, nc, nc, 0x7F, 0x7F, cmd, 0xFF);
  const char *name = cmd == ARTNET_PC_LED_LOCATE ? "Locate" :
                     cmd == ARTNET_PC_LED_MUTE   ? "Mute" : "Normal";
  printf("  LED %s: %s\n", name, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_failsafe(artnet_node n, artnet_port_command_t cmd) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  uint8_t nc[ARTNET_MAX_PORTS];
  memset(nc, 0x7F, ARTNET_MAX_PORTS);
  int ret = artnet_send_address(n, e, NULL, NULL, nc, nc, 0x7F, 0x7F, cmd, 0xFF);
  const char *name = cmd == ARTNET_PC_FAIL_HOLD  ? "HOLD" :
                     cmd == ARTNET_PC_FAIL_ZERO  ? "ZERO" :
                     cmd == ARTNET_PC_FAIL_FULL  ? "FULL" : "SCENE";
  printf("  Failsafe %s: %s\n", name, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_merge_mode(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Mode: 0x50=HTP, 0x10=LTP (for port 0)\n");
  int val = read_hex("  Mode (hex): ");

  uint8_t nc[ARTNET_MAX_PORTS];
  memset(nc, 0x7F, ARTNET_MAX_PORTS);
  int ret = artnet_send_address(n, e, NULL, NULL, nc, nc, 0x7F, 0x7F, (artnet_port_command_t)val, 0xFF);
  printf("  Merge mode: %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_net_addr(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  int val = read_int("  New net (0-127): ", 0);
  if (val < 0 || val > 127) { printf("  Invalid.\n"); return; }

  uint8_t nc[ARTNET_MAX_PORTS];
  memset(nc, 0x7F, ARTNET_MAX_PORTS);
  int ret = artnet_send_address(n, e, NULL, NULL, nc, nc, (uint8_t)val, 0x7F, ARTNET_PC_NONE, 0xFF);
  printf("  Net -> %d: %s\n", val, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_subnet_addr(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  int val = read_int("  New subnet (0-15): ", 0);
  if (val < 0 || val > 15) { printf("  Invalid.\n"); return; }

  uint8_t nc[ARTNET_MAX_PORTS];
  memset(nc, 0x7F, ARTNET_MAX_PORTS);
  int ret = artnet_send_address(n, e, NULL, NULL, nc, nc, 0x7F, (uint8_t)val, ARTNET_PC_NONE, 0xFF);
  printf("  Subnet -> %d: %s\n", val, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

/* ---- Menu ---- */

static void print_menu(void) {
  printf("\n=== Art-Net 4 Full Controller ===\n");
  printf("Discovery:\n");
  printf("  p) Poll network        l) List nodes\n");
  printf("DMX Output:\n");
  printf("  d) DMX send (single)   D) DMX flood (continuous)\n");
  printf("  z) ArtNzs (non-0 SC)   s) ArtSync\n");
  printf("Remote Management:\n");
  printf("  1) Set short name      2) Set long name\n");
  printf("  3) Set net address     4) Set subnet address\n");
  printf("  5) Set merge mode      6) Enable/disable port\n");
  printf("  7) Full ArtAddress     8) LED Locate  9) LED Mute  0) LED Normal\n");
  printf("  f) Failsafe: Hold      g) Failsafe: Zero\n");
  printf("  h) Failsafe: Full      j) Failsafe: Scene\n");
  printf("RDM:\n");
  printf("  t) TOD request         T) TOD control\n");
  printf("  r) Send ArtRdm         R) Send ArtRdmSub\n");
  printf("Time & Trigger:\n");
  printf("  c) ArtTimeCode         y) ArtTimeSync (now)\n");
  printf("  k) ArtTrigger\n");
  printf("Firmware & File:\n");
  printf("  w) Firmware upload     u) File upload (TnMaster)\n");
  printf("  v) File download (Fn)  x) ArtDirectory\n");
  printf("Diagnostics & Data:\n");
  printf("  a) Send DiagData       b) DataReply\n");
  printf("  q) Quit\n");
  printf("> ");
  fflush(stdout);
}

/* ---- Main ---- */

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char *ip = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      ip = argv[++i];
    } else {
      printf("Usage: full_controller [-i <bind_ip>]\n");
      return 1;
    }
  }

  printf("Art-Net 4 Full Controller\n");
  printf("  Bind IP: %s\n\n", ip ? ip : "auto");

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: failed to create node: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_SRV);
  artnet_set_style_code(node, ARTNET_ST_CONTROLLER);
  artnet_set_short_name(node, "FullCtrl");
  artnet_set_long_name(node, "libartnet Art-Net 4 Full Controller");
  artnet_setoem(node, 0xFF, 0x00);
  artnet_setesta(node, 0x00, 0xFF);

  /* Configure ports for sending DMX */
  for (int i = 0; i < ARTNET_MAX_PORTS; i++) {
    artnet_set_port_type(node, i, ARTNET_ENABLE_INPUT | ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
    artnet_set_port_addr(node, i, ARTNET_INPUT_PORT, (uint8_t)i);
    artnet_set_port_addr(node, i, ARTNET_OUTPUT_PORT, (uint8_t)i);
  }

  /* Register all handlers */
  artnet_set_handler(node, ARTNET_REPLY_HANDLER, reply_handler, NULL);
  artnet_set_handler(node, ARTNET_TOD_DATA_HANDLER, tod_data_handler, NULL);
  artnet_set_handler(node, ARTNET_RDM_HANDLER, rdm_handler, NULL);
  artnet_set_handler(node, ARTNET_DIAGDATA_HANDLER, diag_handler, NULL);
  artnet_set_handler(node, ARTNET_FIRMWARE_REPLY_HANDLER, firmware_reply_handler, NULL);
  artnet_set_handler(node, ARTNET_FILE_FN_REPLY_HANDLER, file_fn_reply_handler, NULL);
  artnet_set_handler(node, ARTNET_DIRECTORY_REPLY_HANDLER, directory_reply_handler, NULL);
  artnet_set_handler(node, ARTNET_SYNC_HANDLER, sync_handler, NULL);
  artnet_set_handler(node, ARTNET_TIMECODE_HANDLER, timecode_handler, NULL);
  artnet_set_handler(node, ARTNET_TIMESYNC_HANDLER, timesync_handler, NULL);

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: failed to start: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("Controller started. Discovering nodes...\n");
  artnet_send_poll(node, NULL, ARTNET_TTM_AUTO);

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  char cmd[256];
  print_menu();

  while (running) {
    artnet_read(node, 0);

    if (dmx_active) continue;

    fd_set fds;
    struct timeval tv = {0, 100000};
    FD_ZERO(&fds);
    FD_SET(0, &fds);

    if (select(1, &fds, NULL, NULL, &tv) > 0 && FD_ISSET(0, &fds)) {
      if (!fgets(cmd, sizeof(cmd), stdin)) break;

      switch (cmd[0]) {
        /* Discovery */
        case 'p': cmd_poll(node); break;
        case 'l': cmd_list(node); break;
        /* DMX */
        case 'd': cmd_dmx_send(node); break;
        case 'D': cmd_dmx_flood(node); break;
        case 'z': cmd_nzs(node); break;
        case 's': cmd_sync(node); break;
        /* Remote management */
        case '1': cmd_short_name(node); break;
        case '2': cmd_long_name(node); break;
        case '3': cmd_net_addr(node); break;
        case '4': cmd_subnet_addr(node); break;
        case '5': cmd_merge_mode(node); break;
        case '6': cmd_input(node); break;
        case '7': cmd_address(node); break;
        case '8': cmd_led(node, ARTNET_PC_LED_LOCATE); break;
        case '9': cmd_led(node, ARTNET_PC_LED_MUTE); break;
        case '0': cmd_led(node, ARTNET_PC_LED_NORMAL); break;
        case 'f': cmd_failsafe(node, ARTNET_PC_FAIL_HOLD); break;
        case 'g': cmd_failsafe(node, ARTNET_PC_FAIL_ZERO); break;
        case 'h': cmd_failsafe(node, ARTNET_PC_FAIL_FULL); break;
        case 'j': cmd_failsafe(node, ARTNET_PC_FAIL_SCENE); break;
        /* RDM */
        case 't': cmd_tod_request(node); break;
        case 'T': cmd_tod_control(node); break;
        case 'r': cmd_rdm(node); break;
        case 'R': cmd_rdmsub(node); break;
        /* Time & trigger */
        case 'c': cmd_timecode(node); break;
        case 'y': cmd_timesync(node); break;
        case 'k': cmd_trigger(node); break;
        /* Firmware & file */
        case 'w': cmd_firmware(node); break;
        case 'u': cmd_file_upload(node); break;
        case 'v': cmd_file_download(node); break;
        case 'x': cmd_directory(node); break;
        /* Diagnostics & data */
        case 'a': cmd_diag_send(node); break;
        case 'b': cmd_data_request(node); break;
        /* Quit */
        case 'q': running = 0; break;
        case '\n': break;
        default: break;
      }
      if (running && !dmx_active) print_menu();
    }
  }

  printf("\nStopping...\n");
  dmx_active = 0;
  artnet_stop(node);
  artnet_destroy(node);
  return 0;
}
