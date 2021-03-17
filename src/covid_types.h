/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COVID_TYPES_H
#define COVID_TYPES_H

#include <zephyr/types.h>

#define COVID_ROLLING_PROXIMITY_IDENTIFIER_LEN 16

typedef struct rolling_proximity_identifier { 
    uint8_t data[COVID_ROLLING_PROXIMITY_IDENTIFIER_LEN]; 
} __packed rolling_proximity_identifier_t;

typedef struct bt_metadata {
	uint8_t version;
	uint8_t tx_power;
	uint8_t rsv1;
	uint8_t rsv2;
}  __packed bt_metadata_t;

//typedef struct bt_metadata bt_metadata_t;

typedef struct associated_encrypted_metadata { 
    uint8_t data[sizeof(bt_metadata_t)]; 
} __packed associated_encrypted_metadata_t;

typedef uint8_t rssi_t;

#endif
