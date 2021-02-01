/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONTACTS_H
#define CONTACTS_H

#include <zephyr.h>
#include <zephyr/types.h>

#include "covid.h"
#include "covid_types.h"

//short term contacts: number of people we meet in the 5 minute window. Records any contact, will later be checked whether this contact is longer than 5 minutes
#ifndef MAX_CONTACTS
#define MAX_CONTACTS 1000
#endif

//number of contact during on day.
#ifndef MAX_PERIOD_CONTACTS
#define MAX_PERIOD_CONTACTS 400
#endif

//number of perdios (=days) we record data for
#ifndef PERIODS
#define PERIODS 14
#endif

typedef struct period_contact {
    uint32_t duration;
    uint16_t cnt;
    int8_t max_rssi;  //TODO also store avg rssi?
    rolling_proximity_identifier_t rolling_proximity_identifier;
    associated_encrypted_metadata_t associated_encrypted_metadata;
} period_contact_t;

typedef struct period_contacts {
    int cnt;
    period_contact_t period_contacts[MAX_PERIOD_CONTACTS];
} period_contacts_t;

typedef struct contact {
    uint32_t most_recent_contact_time;  //TODO: what is the correct type here?
    uint32_t first_contact_time;        //TODO: what is the correct type here?
    uint16_t cnt;
    int8_t max_rssi;
    rolling_proximity_identifier_t rolling_proximity_identifier;
    associated_encrypted_metadata_t associated_encrypted_metadata;
} contact_t;

void init_contacts();
int check_add_contact(uint32_t contact_time, rolling_proximity_identifier_t* rpi, associated_encrypted_metadata_t* aem, int8_t rssi);
void key_change(int current_period_index);

void add_infected_key(period_t* period);
uint32_t get_next_infected_key_id();

void print_rpi(rolling_proximity_identifier_t* rpi);
void print_aem(associated_encrypted_metadata_t* aem);

#endif