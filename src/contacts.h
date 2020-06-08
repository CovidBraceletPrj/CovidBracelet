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

void init_contacts();
int check_add_contact(u32_t contact_time, rolling_proximity_identifier_t* rpi, associated_encrypted_metadata_t* aem, s8_t rssi);
void key_change(int current_period_index);

void add_infected_key(period_t* period);
u32_t get_next_infected_key_id();

void print_rpi(rolling_proximity_identifier_t* rpi);
void print_aem(associated_encrypted_metadata_t* aem);

#endif