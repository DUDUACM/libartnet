/*
 * node_manager.c
 * Art-Net 4 node manager example (interactive controller)
 *
 * Discovers Art-Net nodes and allows interactive remote management:
 *   - Change short name / long name
 *   - Change net / subnet / universe addresses
 *   - Enable/disable ports
 *   - Send LED locate / mute / normal commands
 *   - Set failsafe mode
 *
 * Usage: node_manager [-i <bind_ip>]
 *   -i  IP address to bind to (default: first non-loopback interface)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <artnet/artnet.h>
#include <artnet/common.h>

static volatile int running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static void print_discovered(artnet_node n) {
  artnet_node_list nl = artnet_get_nl(n);
  int count = artnet_nl_get_length(nl);
  if (count == 0) {
    printf("  No nodes discovered.\n");
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
      int p;
      printf("  in:");
      for (p = 0; p < entry->numbports; p++)
        printf(" %d", entry->swIn[p] & 0x0F);
      printf("  out:");
      for (p = 0; p < entry->numbports; p++)
        printf(" %d", entry->swOut[p] & 0x0F);
    }
    printf("\n");
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

  char buf[32];
  if (!fgets(buf, sizeof(buf), stdin))
    return NULL;
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

static void strip_newline(char *s) {
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
    s[--len] = '\0';
}

static void cmd_poll(artnet_node n) {
  int ret = artnet_send_poll(n, NULL, ARTNET_TTM_AUTO);
  printf("[Poll] %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_list(artnet_node n) {
  print_discovered(n);
}

static void cmd_short_name(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Current: %.18s\n", (char *)e->shortName);
  printf("  New short name: ");
  fflush(stdout);

  char name[ARTNET_SHORT_NAME_LENGTH + 2];
  if (!fgets(name, sizeof(name), stdin)) return;
  strip_newline(name);
  if (name[0] == '\0') {
    printf("Cancelled.\n");
    return;
  }

  uint8_t nochange[ARTNET_MAX_PORTS];
  memset(nochange, 0x7F, ARTNET_MAX_PORTS);

  int ret = artnet_send_address(n, e, name, NULL, nochange, nochange,
                                0x7F, 0x7F, ARTNET_PC_NONE);
  printf("  Set short name: %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_long_name(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Current: %.64s\n", (char *)e->longName);
  printf("  New long name: ");
  fflush(stdout);

  char name[ARTNET_LONG_NAME_LENGTH + 2];
  if (!fgets(name, sizeof(name), stdin)) return;
  strip_newline(name);
  if (name[0] == '\0') {
    printf("Cancelled.\n");
    return;
  }

  uint8_t nochange[ARTNET_MAX_PORTS];
  memset(nochange, 0x7F, ARTNET_MAX_PORTS);

  int ret = artnet_send_address(n, e, NULL, name, nochange, nochange,
                                0x7F, 0x7F, ARTNET_PC_NONE);
  printf("  Set long name: %s\n", ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_net(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Current net: %d\n", e->netSwitch);
  printf("  New net (0-127): ");
  fflush(stdout);

  char buf[16];
  if (!fgets(buf, sizeof(buf), stdin)) return;
  int val = atoi(buf);
  if (val < 0 || val > 127) {
    printf("Invalid net.\n");
    return;
  }

  uint8_t nochange[ARTNET_MAX_PORTS];
  memset(nochange, 0x7F, ARTNET_MAX_PORTS);

  int ret = artnet_send_address(n, e, NULL, NULL, nochange, nochange,
                                (uint8_t)val, 0x7F, ARTNET_PC_NONE);
  printf("  Set net to %d: %s\n", val, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_subnet(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Current subnet: %d\n", e->subSwitch & 0x0F);
  printf("  New subnet (0-15): ");
  fflush(stdout);

  char buf[16];
  if (!fgets(buf, sizeof(buf), stdin)) return;
  int val = atoi(buf);
  if (val < 0 || val > 15) {
    printf("Invalid subnet.\n");
    return;
  }

  uint8_t nochange[ARTNET_MAX_PORTS];
  memset(nochange, 0x7F, ARTNET_MAX_PORTS);

  int ret = artnet_send_address(n, e, NULL, NULL, nochange, nochange,
                                0x7F, (uint8_t)val, ARTNET_PC_NONE);
  printf("  Set subnet to %d: %s\n", val, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_universe(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Current out ports:");
  for (int p = 0; p < e->numbports; p++)
    printf(" %d", e->swOut[p] & 0x0F);
  printf("\n");

  printf("  Port index (0-%d): ", ARTNET_MAX_PORTS - 1);
  fflush(stdout);
  char buf_port[16];
  if (!fgets(buf_port, sizeof(buf_port), stdin)) return;
  int port = atoi(buf_port);
  if (port < 0 || port >= ARTNET_MAX_PORTS) {
    printf("Invalid port.\n");
    return;
  }

  printf("  New universe (0-15): ");
  fflush(stdout);
  char buf_uni[16];
  if (!fgets(buf_uni, sizeof(buf_uni), stdin)) return;
  int uni = atoi(buf_uni);
  if (uni < 0 || uni > 15) {
    printf("Invalid universe.\n");
    return;
  }

  uint8_t nochange[ARTNET_MAX_PORTS];
  memset(nochange, 0x7F, ARTNET_MAX_PORTS);

  uint8_t inAddr[ARTNET_MAX_PORTS];
  memset(inAddr, 0x7F, ARTNET_MAX_PORTS);
  inAddr[port] = (uint8_t)(uni | 0x80);

  uint8_t outAddr[ARTNET_MAX_PORTS];
  memset(outAddr, 0x7F, ARTNET_MAX_PORTS);
  outAddr[port] = (uint8_t)(uni | 0x80);

  int ret = artnet_send_address(n, e, NULL, NULL, inAddr, outAddr,
                                0x7F, 0x7F, ARTNET_PC_NONE);
  printf("  Set port %d universe to %d: %s\n", port, uni,
         ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_port_enable(artnet_node n, int enable) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Port index (0-%d): ", ARTNET_MAX_PORTS - 1);
  fflush(stdout);
  char buf[16];
  if (!fgets(buf, sizeof(buf), stdin)) return;
  int port = atoi(buf);
  if (port < 0 || port >= ARTNET_MAX_PORTS) {
    printf("Invalid port.\n");
    return;
  }

  uint8_t settings[ARTNET_MAX_PORTS];
  memset(settings, 0, ARTNET_MAX_PORTS);
  if (!enable)
    settings[port] = 0x01;

  int ret = artnet_send_input(n, e, settings);
  printf("  %s port %d: %s\n", enable ? "Enable" : "Disable", port,
         ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_led(artnet_node n, artnet_port_command_t cmd) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  uint8_t nochange[ARTNET_MAX_PORTS];
  memset(nochange, 0x7F, ARTNET_MAX_PORTS);

  int ret = artnet_send_address(n, e, NULL, NULL, nochange, nochange,
                                0x7F, 0x7F, cmd);
  const char *name = cmd == ARTNET_PC_LED_LOCATE ? "Locate" :
                     cmd == ARTNET_PC_LED_MUTE   ? "Mute" : "Normal";
  printf("  LED %s: %s\n", name, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_failsafe(artnet_node n, artnet_port_command_t cmd) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  uint8_t nochange[ARTNET_MAX_PORTS];
  memset(nochange, 0x7F, ARTNET_MAX_PORTS);

  int ret = artnet_send_address(n, e, NULL, NULL, nochange, nochange,
                                0x7F, 0x7F, cmd);
  const char *name = cmd == ARTNET_PC_FAIL_HOLD  ? "HOLD" :
                     cmd == ARTNET_PC_FAIL_ZERO  ? "ZERO" :
                     cmd == ARTNET_PC_FAIL_FULL  ? "FULL" : "SCENE";
  printf("  Failsafe %s: %s\n", name, ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_trigger(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Key type: 0=ASCII, 1=Macro, 2=Soft, 3=Show (default 1): ");
  fflush(stdout);
  char buf[16];
  if (!fgets(buf, sizeof(buf), stdin)) return;
  int key = buf[0] == '\n' ? 1 : atoi(buf);

  printf("  Sub key (0-255, default 0): ");
  fflush(stdout);
  if (!fgets(buf, sizeof(buf), stdin)) return;
  int sub_key = buf[0] == '\n' ? 0 : atoi(buf);

  int ret = artnet_send_trigger(n, 0xFF, 0xFF,
                                (uint8_t)key, (uint8_t)sub_key, NULL, 0);
  printf("  Trigger key=%d sub=%d: %s\n", key, sub_key,
         ret == ARTNET_EOK ? "OK" : artnet_strerror());
}

static void cmd_ipprog(artnet_node n) {
  artnet_node_entry e = select_node(n);
  if (!e) return;

  printf("  Current IP: %d.%d.%d.%d\n", e->ip[0], e->ip[1], e->ip[2], e->ip[3]);
  printf("  New IP (e.g. 192.168.1.50): ");
  fflush(stdout);
  char buf[64];
  if (!fgets(buf, sizeof(buf), stdin)) return;
  strip_newline(buf);
  if (buf[0] == '\0') {
    printf("Cancelled.\n");
    return;
  }

  unsigned int a, b, c, d;
  if (sscanf(buf, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a > 255 || b > 255 || c > 255 || d > 255) {
    printf("Invalid IP address.\n");
    return;
  }

  /* ArtIpProg is sent via artnet_send_address with a raw packet approach.
     Use the library's internal send by constructing the IP prog command.
     For now, we send an ArtAddress with the IP change via a workaround. */
  printf("  Note: ArtIpProg requires direct packet construction.\n");
  printf("  Target IP would be: %u.%u.%u.%u\n", a, b, c, d);
}

static void print_menu(void) {
  printf("\n--- Node Manager ---\n");
  printf("  p) Poll network\n");
  printf("  l) List discovered nodes\n");
  printf("  1) Set short name\n");
  printf("  2) Set long name\n");
  printf("  3) Set net address\n");
  printf("  4) Set subnet address\n");
  printf("  5) Set port universe\n");
  printf("  6) Enable port\n");
  printf("  7) Disable port\n");
  printf("  8) LED: Locate (flash)\n");
  printf("  9) LED: Mute (off)\n");
  printf("  0) LED: Normal\n");
  printf("  f) Failsafe: Hold\n");
  printf("  g) Failsafe: Zero\n");
  printf("  h) Failsafe: Full\n");
  printf("  j) Failsafe: Scene\n");
  printf("  t) Send ArtTrigger\n");
  printf("  i) Send ArtIpProg (change IP)\n");
  printf("  q) Quit\n");
  printf("  ?> ");
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char *ip = NULL;
  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      ip = argv[++i];
    } else {
      printf("Usage: node_manager [-i <bind_ip>]\n");
      return 1;
    }
  }

  printf("Art-Net 4 Node Manager\n");
  printf("  Bind IP: %s\n\n", ip ? ip : "auto");

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: failed to create node: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_SRV);
  artnet_set_style_code(node, ARTNET_ST_CONTROLLER);
  artnet_set_short_name(node, "NodeManager");
  artnet_set_long_name(node, "libartnet Art-Net 4 Node Manager");

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: failed to start: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("Manager started. Discovering nodes...\n");
  artnet_send_poll(node, NULL, ARTNET_TTM_AUTO);

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  char cmd[256];
  while (running) {
    fd_set fds;
    struct timeval tv = {0, 100000}; /* 100ms */

    FD_ZERO(&fds);
    FD_SET(0, &fds);
    int maxfd = 0;

    /* non-blocking check for stdin */
    if (select(maxfd + 1, &fds, NULL, NULL, &tv) > 0 && FD_ISSET(0, &fds)) {
      if (!fgets(cmd, sizeof(cmd), stdin)) break;

      /* process any pending network packets first */
      artnet_read(node, 0);

      switch (cmd[0]) {
        case 'p': cmd_poll(node); break;
        case 'l': cmd_list(node); break;
        case '1': cmd_short_name(node); break;
        case '2': cmd_long_name(node); break;
        case '3': cmd_net(node); break;
        case '4': cmd_subnet(node); break;
        case '5': cmd_universe(node); break;
        case '6': cmd_port_enable(node, 1); break;
        case '7': cmd_port_enable(node, 0); break;
        case '8': cmd_led(node, ARTNET_PC_LED_LOCATE); break;
        case '9': cmd_led(node, ARTNET_PC_LED_MUTE); break;
        case '0': cmd_led(node, ARTNET_PC_LED_NORMAL); break;
        case 'f': cmd_failsafe(node, ARTNET_PC_FAIL_HOLD); break;
        case 'g': cmd_failsafe(node, ARTNET_PC_FAIL_ZERO); break;
        case 'h': cmd_failsafe(node, ARTNET_PC_FAIL_FULL); break;
        case 'j': cmd_failsafe(node, ARTNET_PC_FAIL_SCENE); break;
        case 't': cmd_trigger(node); break;
        case 'i': cmd_ipprog(node); break;
        case 'q': running = 0; break;
        case '\n': break;
        default: print_menu(); break;
      }
      if (running) print_menu();
    } else {
      artnet_read(node, 0);
    }
  }

  printf("\nStopping...\n");
  artnet_stop(node);
  artnet_destroy(node);
  return 0;
}
