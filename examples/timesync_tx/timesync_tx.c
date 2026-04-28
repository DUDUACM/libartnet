/*
 * timesync_tx.c
 * Art-Net 4 TimeSync transmitter example
 *
 * Periodically sends ArtTimeSync packets with the current system date/time.
 *
 * Usage: timesync_tx [-i <bind_ip>] [-r <rate_ms>]
 *   -i  IP address to bind to (default: first non-loopback interface)
 *   -r  Send interval in milliseconds (default: 1000)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <artnet/artnet.h>
#include <artnet/common.h>

#define DEFAULT_INTERVAL_MS 1000

static volatile int running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static void print_usage(const char *prog) {
  printf("Usage: %s [-i <bind_ip>] [-r <rate_ms>]\n", prog);
  printf("  -i  IP address to bind to (default: auto)\n");
  printf("  -r  Send interval in ms (default: %d)\n", DEFAULT_INTERVAL_MS);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char *ip = NULL;
  int interval = DEFAULT_INTERVAL_MS;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      ip = argv[++i];
    } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
      interval = atoi(argv[++i]);
    } else {
      print_usage(argv[0]);
      return 1;
    }
  }

  if (interval < 100) interval = 100;
  if (interval > 60000) interval = 60000;

  printf("Art-Net 4 TimeSync Transmitter\n");
  printf("  Bind IP  : %s\n", ip ? ip : "auto");
  printf("  Interval : %d ms\n\n", interval);

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_NODE);
  artnet_set_style_code(node, ARTNET_ST_NODE);
  artnet_set_short_name(node, "TimeSync TX");
  artnet_set_long_name(node, "libartnet Art-Net 4 TimeSync Transmitter");

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("TimeSync transmitter started. (Ctrl+C to stop)\n");

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  while (running) {
    artnet_read(node, 0);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    int ret = artnet_send_timesync(node,
      (uint8_t)t->tm_sec, (uint8_t)t->tm_min, (uint8_t)t->tm_hour,
      (uint8_t)t->tm_mday, (uint8_t)t->tm_mon, (uint8_t)(t->tm_year % 100));

    if (ret != ARTNET_EOK) {
      printf("[TimeSync] Error: %s\n", artnet_strerror());
    } else {
      printf("[TimeSync] %04d-%02d-%02d %02d:%02d:%02d\n",
             1900 + t->tm_year, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    }

#ifdef WIN32
    Sleep(interval);
#else
    struct timespec ts;
    ts.tv_sec = interval / 1000;
    ts.tv_nsec = (interval % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
  }

  printf("\nStopping...\n");
  artnet_stop(node);
  artnet_destroy(node);
  return 0;
}
