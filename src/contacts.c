/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include "covid_types.h"
#include "contacts.h"
#include "exposure-notification.h"
#include "covid.h"

void print_key(_ENBaseKey* key){
    for( int i = 0; i < sizeof(key->b); i++){
        printk("%02x", key->b[i]);
    }
}

void print_rpi(rolling_proximity_identifier_t* rpi){
    for( int i = 0; i < sizeof(rolling_proximity_identifier_t); i++){
        printk("%02x", rpi->data[i]);
    }
}

void print_aem(associated_encrypted_metadata_t* aem){
    for( int i = 0; i < sizeof(associated_encrypted_metadata_t); i++){
        printk("%02x", aem->data[i]);
    }
}
