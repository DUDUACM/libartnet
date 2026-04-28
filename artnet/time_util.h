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
 * time_util.h
 * Provides a monotonic millisecond clock and timeout helpers, replacing
 * the mixed use of time_t (seconds) and clock_t (ticks) throughout the
 * library with a single consistent time source.
 * Copyright (C) 2026 HeJianglong
 */

#ifndef ARTNET_TIME_UTIL_H
#define ARTNET_TIME_UTIL_H

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
typedef ULONGLONG artnet_mtime_t;
#else
typedef uint64_t artnet_mtime_t;
#endif

/**
 * @brief Get the current monotonic time in milliseconds.
 *
 * Uses GetTickCount64() on Windows, clock_gettime(CLOCK_MONOTONIC) on POSIX.
 * The value is guaranteed to be non-decreasing across calls.
 *
 * @return Current time in milliseconds since an unspecified epoch
 */
artnet_mtime_t artnet_gettime_ms(void);

/**
 * @brief Compute the difference between two timestamps in milliseconds.
 *
 * @param a The later timestamp
 * @param b The earlier timestamp
 * @return a - b in milliseconds (may be negative if b > a)
 */
int64_t artnet_time_diff_ms(artnet_mtime_t a, artnet_mtime_t b);

/**
 * @brief Check whether a timeout has expired.
 *
 * @param now        Current time from artnet_gettime_ms()
 * @param since      Timestamp when the timer started
 * @param timeout_ms Timeout duration in milliseconds
 * @return 1 if (now - since) >= timeout_ms, 0 otherwise
 */
int artnet_is_timeout(artnet_mtime_t now, artnet_mtime_t since, int timeout_ms);

#endif /* ARTNET_TIME_UTIL_H */
