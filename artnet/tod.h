/*
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
 * tod.h
 * RDM Table of Devices (TOD) management
 * Copyright (C) 2004-2005 Simon Newton
 */

#ifndef ARTNET_TOD_H
#define ARTNET_TOD_H

#include <stdint.h>
#include "common.h"

/** Initial capacity of the TOD data array */
enum { ARTNET_TOD_INITIAL_SIZE = 100 };
/** Number of UIDs to add when reallocating the TOD data array */
enum { ARTNET_TOD_INCREMENT = 50 };

/**
 * Table of Devices (TOD) - stores discovered RDM device UIDs
 */
typedef struct {
  uint8_t *data;      /**< Flat array of RDM UIDs, each ARTNET_RDM_UID_WIDTH bytes */
  int length;         /**< Current number of UIDs in the table */
  int max_length;     /**< Current allocated capacity */
} tod_t;


/**
 * Add a UID to the TOD. Sends ArtTodData if the port is enabled.
 * @return ARTNET_EOK on success, or a negative error code
 */
extern int add_tod_uid(tod_t *tod, uint8_t uid[ARTNET_RDM_UID_WIDTH]);

/**
 * Remove a UID from the TOD.
 * @return ARTNET_EOK on success, or a negative error code
 */
extern int remove_tod_uid(tod_t *tod, uint8_t uid[ARTNET_RDM_UID_WIDTH]);

/**
 * Clear all UIDs from the TOD and free memory.
 * @return ARTNET_EOK on success
 */
extern int flush_tod(tod_t *tod);

/**
 * Re-initialize the TOD (same as flush_tod).
 * @return ARTNET_EOK on success
 */
extern int reset_tod(tod_t *tod);

#endif
