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
 * tod.c
 * Functions to manipulate the TOD
 * Copyright (C) 2004-2005 Simon Newton
 */

#include <stdlib.h>

#include "tod.h"
#include "misc.h"

/**
 * Adds a uid to the table of devices.
 *
 * @param tod the TOD to add to
 * @param uid the RDM UID to add (ARTNET_RDM_UID_WIDTH bytes)
 * @return 0 on success, -1 on null tod, ARTNET_EMEM on allocation failure
 */
int add_tod_uid(tod_t *tod, uint8_t uid[ARTNET_RDM_UID_WIDTH]) {
  uint8_t *addr = NULL;
  int size = 0;

  if (tod == NULL) {
    return -1;
  }

  if (tod->data == NULL) {
    // malloc
    tod->data = malloc(ARTNET_RDM_UID_WIDTH * ARTNET_TOD_INITIAL_SIZE);

    if (tod->data == NULL) {
      artnet_error_malloc();
      return ARTNET_EMEM;
    }
    tod->length = 1;
    tod->max_length = ARTNET_TOD_INITIAL_SIZE;

  } else if (tod->length == tod->max_length) {
    // realloc
    size = (tod->max_length + ARTNET_TOD_INCREMENT);
    uint8_t *new_data = realloc(tod->data, size * ARTNET_RDM_UID_WIDTH);

    if (new_data == NULL) {
      artnet_error_realloc();
      return ARTNET_EMEM;
    }

    tod->data = new_data;
    tod->max_length = size;
    tod->length++;

  } else {
    tod->length++;
  }

  addr = tod->data + (tod->length-1) * ARTNET_RDM_UID_WIDTH;
  memcpy(addr, uid, ARTNET_RDM_UID_WIDTH);

  return 0;
}

/**
 * Remove a uid from the table of devices.
 *
 * @param tod the TOD to remove from
 * @param uid the RDM UID to remove (ARTNET_RDM_UID_WIDTH bytes)
 * @return 0 on success, -1 if not found or null tod
 */
int remove_tod_uid(tod_t *tod, uint8_t uid[ARTNET_RDM_UID_WIDTH]) {
  int i = 0;
  int offset = 0;
  uint8_t *last = NULL;

  if (tod == NULL) {
    return -1;
  }

  if (tod->data == NULL) {
    return -1;
  }

  for (i=0; i < tod->length; i++) {
    offset = i * ARTNET_RDM_UID_WIDTH;
    if (memcmp(tod->data + offset, uid, ARTNET_RDM_UID_WIDTH) == 0) {
      break;
    }
  }

  if (i == tod->length) {
    return -1;
  } else {

    last = tod->data + (tod->length-1) * ARTNET_RDM_UID_WIDTH;
    // copy the last entry over this one
    memcpy(tod->data + offset, last ,ARTNET_RDM_UID_WIDTH);

    tod->length--;
    return 0;
  }
}

/**
 * Clear the table of devices and free allocated memory.
 *
 * @param tod the TOD to flush
 * @return 0 on success, -1 if tod is NULL
 */
int flush_tod(tod_t *tod) {
  if (tod == NULL) {
    return -1;
  }

  free(tod->data);
  tod->data = NULL;
  tod->length = 0;
  tod->max_length = 0;

  return 0;
}


/**
 * Reset the TOD to initial empty state.
 *
 * @param tod the TOD to reset
 * @return 0 on success, -1 if tod is NULL
 */
int reset_tod(tod_t *tod) {
  if (tod == NULL) {
    return -1;
  }

  free(tod->data);
  tod->data = NULL;
  tod->length = 0;
  tod->max_length = 0;
  return 0;
}
