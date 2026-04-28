/**
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * time_util.c
 * Millisecond-precision time utilities for libartnet.
 * Copyright (C) 2026 HeJianglong
 */

#include "time_util.h"

#ifndef _WIN32
#include <time.h>
#endif

artnet_mtime_t artnet_gettime_ms(void) {
#ifdef _WIN32
  return (artnet_mtime_t)GetTickCount64();
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (artnet_mtime_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

int64_t artnet_time_diff_ms(artnet_mtime_t a, artnet_mtime_t b) {
  return a - b;
}

int artnet_is_timeout(artnet_mtime_t now, artnet_mtime_t since, int timeout_ms) {
  return (int64_t)(now - since) >= timeout_ms;
}
