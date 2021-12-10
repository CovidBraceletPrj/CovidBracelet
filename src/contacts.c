/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <timing/timing.h>
#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include "bloom.h"
#include "contacts.h"
#include "covid.h"
#include "covid_types.h"
#include "ens/records.h"
#include "ens/storage.h"
#include "exposure-notification.h"

// #define CONFIG_INTERVAL_SPREAD 100

void print_key(_ENBaseKey* key) {
    for (int i = 0; i < sizeof(key->b); i++) {
        printk("%02x", key->b[i]);
    }
}

void print_rpi(rolling_proximity_identifier_t* rpi) {
    for (int i = 0; i < sizeof(rolling_proximity_identifier_t); i++) {
        printk("%02x", rpi->b[i]);
    }
}

void print_aem(associated_encrypted_metadata_t* aem) {
    for (int i = 0; i < sizeof(associated_encrypted_metadata_t); i++) {
        printk("%02x", aem->data[i]);
    }
}

int register_record(record_t* record) {
    return add_record(record);
}

/**
 * Simple implementation, where an iterator is created for every element in the passed arrray.
 */
int get_number_of_infected_for_multiple_intervals_simple(infected_for_interval_ident_ctx_t* ctx, int count) {
    record_iterator_t iterator;
    for (int i = 0; i < count; i++) {
        if (!ctx[i].met) {
            continue;
        }
        ctx[i].met = 0;
        int rc = ens_records_iterator_init_timerange(&iterator, &ctx[i].search_start, &ctx[i].search_end);
        if (rc) {
            // on error, skip this rpi
            continue;
        }
        record_t* current;
        while ((current = ens_records_iterator_next(&iterator))) {
            if (memcmp(&(current->rolling_proximity_identifier), &ctx[i].interval_identifier,
                       sizeof(rolling_proximity_identifier_t)) == 0) {
                ctx[i].met++;
            }
        }
    }
    return 0;
}

/**
 * Fill the bloom filter with all stored records.
 */
void fill_bloom_with_stored_records(bloom_filter_t* bloom) {
    // init iterator for filling bloom filter
    record_iterator_t iterator;
    int rc = ens_records_iterator_init_timerange(&iterator, NULL, NULL);
    if (rc) {
        printk("init iterator failed0 (err %d)\n", rc);
        return;
    }

    // fill bloom filter with records
    record_t* current;
    while ((current = ens_records_iterator_next(&iterator))) {
        bloom_add_record(bloom, &(current->rolling_proximity_identifier));
    }
}

/**
 * Fill the bloom filter with flash records and test passed RPIs against it.
 */
int64_t bloom_filter(infected_for_interval_ident_ctx_t* ctx, int count) {
    bloom_filter_t* bloom = bloom_init(get_num_records() * 2);
    if (!bloom) {
        printk("bloom init failed\n");
        return -1;
    }

    fill_bloom_with_stored_records(bloom);

    // test bloom performance
    for (int i = 0; i < count; i++) {
        if (bloom_probably_has_record(bloom, &ctx[i].interval_identifier)) {
            ctx[i].met++;
        }
    }
    bloom_destroy(bloom);

    return get_number_of_infected_for_multiple_intervals_simple(ctx, count);
}

////////////////////
// FILL TEST DATA //
////////////////////

static ENPeriodKey infectedPeriodKey = {
    .b = {0x75, 0xc7, 0x34, 0xc6, 0xdd, 0x1a, 0x78, 0x2d, 0xe7, 0xa9, 0x65, 0xda, 0x5e, 0xb9, 0x31, 0x25}};
static ENPeriodKey dummyPeriodKey = {
    .b = {0x89, 0xa7, 0x34, 0xc6, 0xdd, 0x1a, 0x14, 0xda, 0xe7, 0x00, 0x65, 0xda, 0x6a, 0x9b, 0x13, 0x52}};
static ENPeriodKey testKey = {
    .b = {0x89, 0xa7, 0x72, 0xc6, 0xdd, 0x10, 0x14, 0xda, 0xe7, 0x00, 0x65, 0xda, 0x8a, 0x9b, 0x13, 0x52}};

static ENPeriodIdentifierKey infectedPik;
static ENPeriodIdentifierKey dummyPik;
static ENPeriodIdentifierKey testPik;

void fill_test_rki_data(infected_for_interval_ident_ctx_t* infectedIntervals, int count) {
    int totalTime = EN_TEK_ROLLING_PERIOD * EN_INTERVAL_LENGTH;
    int stepSize = totalTime / count;
    for (int i = 0; i < count; i++) {
        int intervalNumber = (i * stepSize) / EN_INTERVAL_LENGTH;
        en_derive_interval_identifier(&infectedIntervals[i].interval_identifier, &infectedPik, intervalNumber);
        infectedIntervals[i].met = 0;
        infectedIntervals[i].search_start = i < 3 ? 0 : (i - 2) * stepSize;
        infectedIntervals[i].search_end = (i + 2) * stepSize;
    }
}

////////////////////
// MEASURING FUNC //
////////////////////

void measure_perf(infected_for_interval_ident_ctx_t testIntervals[], int count) {
    const char* label = "bloom filter";
    printk("---------------------------\n'%s': starting measurement\n", label);

    // fill_test_rki_data(infectedIntervals, count);

    // setup our ordered array with met RPIs
    printk("Starting measurements with %d RPIs to seach and an infection rate of %d\n", count,
           CONFIG_TEST_INFECTED_RATE);

    timing_t start_time, end_time;
    uint64_t total_cycles;
    uint64_t total_ns;

    timing_init();
    timing_start();
    start_time = timing_counter_get();

    check_possible_contacts_for_intervals(testIntervals, count);

    end_time = timing_counter_get();

    total_cycles = timing_cycles_get(&start_time, &end_time);
    total_ns = timing_cycles_to_ns(total_cycles);

    timing_stop();

    printk("\n'%s' took %lld ms\n---------------------------\n", label, total_ns / 1000000);
}

void check_results(infected_for_interval_ident_ctx_t testIntervals[], int count) {
    int counter = 0;
    for (int i = 0; i < count / 2; i++) {
        counter += CONFIG_TEST_RECORDS_PER_INTERVAL * CONFIG_TEST_INFECTED_RATE;
        int met = counter / 100;
        counter %= 100;
        if (testIntervals[i].met != met) {
            printk("interval %d should have been met %d times (met %d)\n", i, met, testIntervals[i].met);
            return;
        }
    }

    for (int j = count / 2; j < count; j++) {
        if (testIntervals[j].met) {
            printk("infected interval should not have been met (interval %d)\n", j);
            return;
        }
    }
    printk("all results are as expected!\n");
}

int reverse_bloom_filter(infected_for_interval_ident_ctx_t* ctx, int count) {
    bloom_filter_t* bloom = bloom_init(count * 2);
    if (!bloom) {
        printk("bloom init failed\n");
        return -1;
    }

    // init bloom filter with passed records
    for (int i = 0; i < count; i++) {
        bloom_add_record(bloom, &ctx[i].interval_identifier);
    }

    int rc = 0;

    // init iterator over the entire storage
    record_iterator_t iterator;
    rc = ens_records_iterator_init_timerange(&iterator, NULL, NULL);
    if (rc) {
        printk("init iterator failed (err %d)\n", rc);
        goto cleanup;
    }

    record_t* current;
    while ((current = ens_records_iterator_next(&iterator))) {
        if (bloom_probably_has_record(bloom, &(current->rolling_proximity_identifier))) {
            for (int i = 0; i < count; i++) {
                if (memcmp(&(current->rolling_proximity_identifier), &ctx[i].interval_identifier,
                           sizeof(current->rolling_proximity_identifier)) == 0) {
                    ctx[i].met++;
                }
            }
        }
    }

cleanup:
    // destroy bloom filter after things are finished
    bloom_destroy(bloom);
    return rc;
}

////////////////////
// SETUP DATA     //
////////////////////

void setup_test_data(infected_for_interval_ident_ctx_t testIntervals[], int count) {
    en_derive_period_identifier_key(&infectedPik, &infectedPeriodKey);
    en_derive_period_identifier_key(&dummyPik, &dummyPeriodKey);
    en_derive_period_identifier_key(&testPik, &testKey);

    int counter = 0;
    for (int i = 0; i < EN_TEK_ROLLING_PERIOD; i++) {
        record_t infectedRecord;
        record_t dummyRecord;
        en_derive_interval_identifier((ENIntervalIdentifier*)&infectedRecord.rolling_proximity_identifier, &infectedPik,
                                      i);
        en_derive_interval_identifier((ENIntervalIdentifier*)&dummyRecord.rolling_proximity_identifier, &dummyPik, i);

        for (int j = 0; j < CONFIG_TEST_RECORDS_PER_INTERVAL; j++) {
            counter += CONFIG_TEST_INFECTED_RATE;
            record_t* curRecord;
            if (counter >= 100) {
                counter -= 100;
                curRecord = &infectedRecord;
            } else {
                curRecord = &dummyRecord;
            }
            curRecord->timestamp = i * EN_INTERVAL_LENGTH + j * (EN_INTERVAL_LENGTH / CONFIG_TEST_RECORDS_PER_INTERVAL);
            int rc = add_record(curRecord);
            if (rc) {
                printk("err %d\n", rc);
            }
        }
    }

    for (int i = 0; i < count / 2; i++) {
        en_derive_interval_identifier(&testIntervals[i].interval_identifier, &infectedPik, i);
        testIntervals[i].met = 0;
        testIntervals[i].search_start = MAX(0, (i * EN_INTERVAL_LENGTH - 2 * 60 * 60));
        testIntervals[i].search_end = i * EN_INTERVAL_LENGTH + 2 * 60 * 60;

        int j = i + count / 2;
        en_derive_interval_identifier(&testIntervals[j].interval_identifier, &testPik, i);
        testIntervals[j].met = 0;
        testIntervals[j].search_start = MAX(0, (i * EN_INTERVAL_LENGTH - 2 * 60 * 60));
        testIntervals[j].search_end = i * EN_INTERVAL_LENGTH + 2 * 60 * 60;
    }
}

int init_contacts() {
#if CONFIG_CONTACTS_PERFORM_RISC_CHECK_TEST
    static infected_for_interval_ident_ctx_t testIntervals[EN_TEK_ROLLING_PERIOD * 2];

    reset_record_storage();
    setup_test_data(testIntervals, EN_TEK_ROLLING_PERIOD * 2);

    measure_perf(testIntervals, EN_TEK_ROLLING_PERIOD * 2);
    check_results(testIntervals, EN_TEK_ROLLING_PERIOD * 2);
#endif
    return 0;
}

int check_possible_contacts_for_intervals(infected_for_interval_ident_ctx_t* ctx, int count) {
#if CONFIG_CONTACTS_BLOOM_REVERSE
    return reverse_bloom_filter(ctx, count);
#else
    return bloom_filter(ctx, count);
#endif
}

int check_possible_contacts_for_periods(period_key_information_t periodKeyInformation[], int count) {
    for (int i = 0; i < count; i++) {
        static infected_for_interval_ident_ctx_t intervalIdents[EN_TEK_ROLLING_PERIOD];
        int periodStart = en_get_interval_number(periodKeyInformation[i].start);
        for (int interval = 0; interval < EN_TEK_ROLLING_PERIOD; interval++) {
            en_derive_interval_identifier(&intervalIdents[interval].interval_identifier,
                                          &periodKeyInformation[i].periodKey, periodStart + interval);
        }
        int rc = check_possible_contacts_for_intervals(intervalIdents, 0);
        if (rc < 0) {
            return rc;
        }
        periodKeyInformation[i].met = rc;
    }
    return 0;
}
