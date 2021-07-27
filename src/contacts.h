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
#include "ens/storage.h"
#include "exposure-notification.h"

typedef struct infected_for_period_key_ctx {
    ENIntervalIdentifier interval_identifier;
    int infected;
    time_t search_start;
    time_t search_end;
} infected_for_period_key_ctx_t;

void print_key(_ENBaseKey* key);
void print_rpi(rolling_proximity_identifier_t* rpi);
void print_aem(associated_encrypted_metadata_t* aem);

/**
 * Register a new record in the system. This includes adding it to the storage and adding it to the bloom filter.
 *
 * @param record record to add
 * @returns 0 in case of success, -ERRNO in case of an error
 */
int register_record(record_t* record);

/**
 * Get the number of infected records for a given PeriodKey.
 *
 * @param key the PeriodKey
 * @param timestamp the timestamp of the period
 *
 * @returns the number of infected records or -ERRNO in case of an error
 */
int get_number_of_infected_for_period(ENPeriodKey* key, time_t timestamp);

/**
 * Get the number of infected records for multiple IntervalIdentifier.
 *
 * @param ctx array of period keys and timestamps with field "infected" for the number of infected records
 * @param count the number of periods in the array
 *
 * @returns 0 in case of success, -ERRNO in case of an error
 */
int get_number_of_infected_for_multiple_intervals(infected_for_period_key_ctx_t* ctx, int count);

/**
 * Setup fixed test data for storage.
 */
void setup_test_data();

#endif