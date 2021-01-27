/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONTACTS_H
#define CONTACTS_H

#include <zephyr.h>
#include <zephyr/types.h>

#include "covid_types.h"
#include "covid.h"

typedef struct period_contact {
    uint32_t duration;
    uint16_t cnt;
    int8_t max_rssi; //TODO also store avg rssi?
    rolling_proximity_identifier_t rolling_proximity_identifier;
    associated_encrypted_metadata_t associated_encrypted_metadata;
} period_contact_t;

void init_contacts();
int check_add_contact(uint32_t contact_time, rolling_proximity_identifier_t* rpi, associated_encrypted_metadata_t* aem, int8_t rssi);
void key_change(int current_period_index);

void add_infected_key(period_t* period);
uint32_t get_next_infected_key_id();

void print_rpi(rolling_proximity_identifier_t* rpi);
void print_aem(associated_encrypted_metadata_t* aem);

#endif