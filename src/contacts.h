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

typedef struct {
    ENIntervalIdentifier interval_identifier;
    int met;
    time_t search_start;
    time_t search_end;
} __packed infected_for_interval_ident_ctx_t;

typedef struct {
    ENPeriodKey periodKey;
    time_t start;
    int met;
} __packed period_key_information_t;

typedef int (*test_func_t)(infected_for_interval_ident_ctx_t* infectedIntervals, int count);

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
 * Initialize the contacts module.
 */
int init_contacts();

/**
 * Check for a list of specified interval identifiers, whether they were probably met or not.
 * @param ctx list of interval identifiers to check
 * @param count amount of identifiers to check
 * @return the amount of met intervals, -ERRNO on error
 */
int check_possible_contacts_for_intervals(infected_for_interval_ident_ctx_t* ctx, int count);

/**
 * Check for a list of specified period keys, whether they were probably met or not.
 * @param ctx list of period keys and their meta information to check
 * @param count amount of period keys to check
 * @return -ERRNO on error, 0 otherwise
 */
int check_possible_contacts_for_periods(period_key_information_t periodKeyInformation[], int count);

#endif
