/*
 * timecode_tx.c
 * Art-Net 4 TimeCode transmitter example
 *
 * Periodically sends SMPTE/EBU timecode frames to the network.
 *
 * Usage: timecode_tx [-i <bind_ip>] [-t <type>] [-s <stream_id>]
 *   -i  IP address to bind to (default: first non-loopback interface)
 *   -t  TimeCode type: 0=Film(24fps), 1=EBU(25fps), 2=DF(29.97fps), 3=SMPTE(30fps)
 *       (default: 1)
 *   -s  Stream ID 0-255 (default: 0 = master)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <artnet/artnet.h>
#include <artnet/common.h>

static volatile int running = 1;

static void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

static const char *type_name(artnet_timecode_type_t type) {
  switch (type) {
    case ARTNET_TIMECODE_FILM:  return "Film (24fps)";
    case ARTNET_TIMECODE_EBU:   return "EBU (25fps)";
    case ARTNET_TIMECODE_DF:    return "DF (29.97fps)";
    case ARTNET_TIMECODE_SMPTE: return "SMPTE (30fps)";
  }
  return "Unknown";
}

static int frame_interval_ms(artnet_timecode_type_t type) {
  switch (type) {
    case ARTNET_TIMECODE_FILM:  return 42;   /* 1000/24  ~= 41.67 */
    case ARTNET_TIMECODE_EBU:   return 40;   /* 1000/25  =  40    */
    case ARTNET_TIMECODE_DF:    return 33;   /* 1000/29.97 ~= 33.37 */
    case ARTNET_TIMECODE_SMPTE: return 33;   /* 1000/30  ~= 33.33 */
  }
  return 40;
}

static void print_usage(const char *prog) {
  printf("Usage: %s [-i <bind_ip>] [-t <type>] [-s <stream_id>]\n", prog);
  printf("  -i  IP address to bind to (default: auto)\n");
  printf("  -t  TimeCode type 0-3 (default: 1 = EBU 25fps)\n");
  printf("      0=Film(24fps), 1=EBU(25fps), 2=DF(29.97fps), 3=SMPTE(30fps)\n");
  printf("  -s  Stream ID 0-255 (default: 0 = master)\n");
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char *ip = NULL;
  int type_val = ARTNET_TIMECODE_EBU;
  int stream_id = 0;
  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      ip = argv[++i];
    } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
      type_val = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
      stream_id = atoi(argv[++i]);
    } else {
      print_usage(argv[0]);
      return 1;
    }
  }

  if (type_val < 0 || type_val > 3) {
    printf("Error: type must be 0-3\n");
    return 1;
  }
  artnet_timecode_type_t tc_type = (artnet_timecode_type_t)type_val;

  printf("Art-Net 4 TimeCode Transmitter\n");
  printf("  Bind IP  : %s\n", ip ? ip : "auto");
  printf("  Type     : %s\n", type_name(tc_type));
  printf("  Stream ID: %d\n\n", stream_id);

  artnet_node node = artnet_new(ip, 0);
  if (!node) {
    printf("Error: failed to create node: %s\n", artnet_strerror());
    return 1;
  }

  artnet_set_node_type(node, ARTNET_NODE);
  artnet_set_style_code(node, ARTNET_ST_NODE);
  artnet_set_short_name(node, "LibArtNet TC-TX");
  artnet_set_long_name(node, "libartnet Art-Net 4 TimeCode Transmitter");

  if (artnet_start(node) != ARTNET_EOK) {
    printf("Error: failed to start node: %s\n", artnet_strerror());
    artnet_destroy(node);
    return 1;
  }

  printf("TimeCode transmitter started. (Ctrl+C to stop)\n\n");

  signal(SIGINT, signal_handler);
#ifdef WIN32
  signal(SIGBREAK, signal_handler);
#endif

  uint8_t hours = 0, minutes = 0, seconds = 0, frames = 0;
  uint8_t max_frame;
  switch (tc_type) {
    case ARTNET_TIMECODE_FILM:  max_frame = 24; break;
    case ARTNET_TIMECODE_EBU:   max_frame = 25; break;
    case ARTNET_TIMECODE_DF:    max_frame = 30; break;
    case ARTNET_TIMECODE_SMPTE: max_frame = 30; break;
    default:              max_frame = 25; break;
  }
  int interval_ms = frame_interval_ms(tc_type);

  while (running) {
    artnet_read(node, 0);

    int ret = artnet_send_timecode(node, frames, seconds, minutes, hours,
                                   tc_type, (uint8_t)stream_id);
    if (ret != ARTNET_EOK) {
      printf("[TX] Error: %s\n", artnet_strerror());
    } else {
      printf("[TX] %02d:%02d:%02d:%02d  %s  stream=%d\n",
             hours, minutes, seconds, frames,
             type_name(tc_type), stream_id);
    }

    /* advance timecode */
    frames++;
    if (frames >= max_frame) {
      frames = 0;
      seconds++;
      if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 60) {
          minutes = 0;
          hours++;
          if (hours >= 24)
            hours = 0;
        }
      }
    }

#ifdef WIN32
    Sleep(interval_ms);
#else
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = interval_ms * 1000000L;
    nanosleep(&ts, NULL);
#endif
  }

  printf("\nStopping...\n");
  artnet_stop(node);
  artnet_destroy(node);
  return 0;
}
