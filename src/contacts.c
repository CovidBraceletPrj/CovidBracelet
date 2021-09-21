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

void print_key(_ENBaseKey* key) {
    for (int i = 0; i < sizeof(key->b); i++) {
        printk("%02x", key->b[i]);
    }
}

void print_rpi(rolling_proximity_identifier_t* rpi) {
    for (int i = 0; i < sizeof(rolling_proximity_identifier_t); i++) {
        printk("%02x", rpi->data[i]);
    }
}

void print_aem(associated_encrypted_metadata_t* aem) {
    for (int i = 0; i < sizeof(associated_encrypted_metadata_t); i++) {
        printk("%02x", aem->data[i]);
    }
}

int register_record(record_t* record) {
    int rc = add_record(record);
    if (rc) {
        return rc;
    }
    // rc = bloom_add_record(record);
    return rc;
}

/**
 * Dumb implementation, where a single iterator is created for iterating over the entire flash.
 */
int get_number_of_infected_for_multiple_intervals_dumb(infected_for_period_key_ctx_t* ctx, int count) {
    record_iterator_t iterator;
    int rc = ens_records_iterator_init_timerange(&iterator, NULL, NULL);
    if (rc) {
        // there was a general error, so just do nothing
        return rc;
    }
    while (!iterator.finished) {
        for (int i = 0; i < count; i++) {
            if (memcmp(&iterator.current.rolling_proximity_identifier, &ctx[i].interval_identifier,
                       sizeof(rolling_proximity_identifier_t)) == 0) {
                ctx[i].infected++;
            }
        }
        ens_records_iterator_next(&iterator);
    }
    return 0;
}

/**
 * Simple implementation, where an iterator is created for every element in the passed arrray.
 */
int get_number_of_infected_for_multiple_intervals_simple(infected_for_period_key_ctx_t* ctx, int count) {
    record_iterator_t iterator;
    for (int i = 0; i < count; i++) {
        int rc = ens_records_iterator_init_timerange(&iterator, &ctx[i].search_start, &ctx[i].search_end);
        if (rc) {
            // on error, skip this rpi
            continue;
        }
        while (!iterator.finished) {
            if (memcmp(&iterator.current.rolling_proximity_identifier, &ctx[i].interval_identifier,
                       sizeof(rolling_proximity_identifier_t)) == 0) {
                ctx[i].infected++;
            }
            ens_records_iterator_next(&iterator);
        }
    }
    return 0;
}

/**
 * Optimized implementation, where overlapping search intervals for consecutive RPI are merged.
 */
int get_number_of_infected_for_multiple_intervals_optimized(infected_for_period_key_ctx_t* ctx, int count) {
    record_iterator_t iterator;
    int i = 0;
    while (i < count) {
        // determine start and end of iterator
        int start = i;
        int end = i;
        while (end + 1 < count && ctx[end + 1].search_start <= ctx[end].search_end) {
            end++;
        }
        // init iterator with start and end
        int rc = ens_records_iterator_init_timerange(&iterator, &ctx[start].search_start, &ctx[end].search_end);
        if (rc) {
            goto end;
        }
        while (!iterator.finished) {
            for (int j = start; j <= end; j++) {
                if (memcmp(&iterator.current.rolling_proximity_identifier, &ctx[j].interval_identifier,
                           sizeof(iterator.current.rolling_proximity_identifier)) == 0) {
                    ctx[j].infected++;
                }
            }
            ens_records_iterator_next(&iterator);
        }
    end:
        i = end + 1;
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
    while (!iterator.finished) {
        bloom_add_record(bloom, &iterator.current);
        ens_records_iterator_next(&iterator);
    }
}

/**
 * Fill the bloom filter with flash records and test passed RPIs against it.
 */
int64_t test_bloom_performance(infected_for_period_key_ctx_t* ctx, int count) {
    bloom_filter_t* bloom = bloom_init(get_num_records() * 2);
    if (!bloom) {
        printk("bloom init failed\n");
        return -1;
    }

    // Measure bloom creation time
    timing_t start_time, end_time;
    uint64_t total_cycles;
    uint64_t total_ns;
    start_time = timing_counter_get();

    fill_bloom_with_stored_records(bloom);

    end_time = timing_counter_get();
    total_cycles = timing_cycles_get(&start_time, &end_time);
    total_ns = timing_cycles_to_ns(total_cycles);
    printk("\nbloom init took %lld ms\n\n", total_ns / 1000000);

    // test bloom performance
    for (int i = 0; i < count; i++) {
        if (bloom_probably_has_record(bloom, &ctx[i].interval_identifier)) {
            ctx[i].infected++;
        }
    }

    int amount = 0;
    for (int i = 0; i < count; i++) {
        amount += ctx[i].infected;
    }
    printk("amount of infected records: %d\n", amount);

    int ret = get_number_of_infected_for_multiple_intervals_optimized(ctx, count);
cleanup:
    k_free(bloom);
    return ret;
}

/**
 * Fill bloom with passed RPIs and test flash records against it.
 */
int64_t test_bloom_reverse_performance(infected_for_period_key_ctx_t* ctx, int count) {
    bloom_filter_t* bloom = bloom_init(count * 2);
    if (!bloom) {
        printk("bloom init failed\n");
        return -1;
    }

    timing_t start_time, end_time;
    uint64_t total_cycles;
    uint64_t total_ns;

    start_time = timing_counter_get();
    for (int i = 0; i < count; i++) {
        bloom_add_record(bloom, &ctx[i].interval_identifier);
    }
    end_time = timing_counter_get();

    total_cycles = timing_cycles_get(&start_time, &end_time);
    total_ns = timing_cycles_to_ns(total_cycles);

    printk("\nbloom init took %lld ms\n\n", total_ns / 1000000);

    int64_t amount = 0;

    record_iterator_t iterator;
    int rc = ens_records_iterator_init_timerange(&iterator, NULL, NULL);
    if (rc) {
        printk("init iterator failed (err %d)\n", rc);
        amount = rc;
        goto cleanup;
    }

    while (!iterator.finished) {
        if (bloom_probably_has_record(bloom, &iterator.current.rolling_proximity_identifier)) {
            for (int i = 0; i < count; i++) {
                if (memcmp(&iterator.current.rolling_proximity_identifier, &ctx[i].interval_identifier,
                           sizeof(iterator.current.rolling_proximity_identifier)) == 0) {
                    amount++;
                    break;
                }
                if (iterator.current.timestamp > ctx[i].search_end) {
                    break;
                }
            }
        }
        ens_records_iterator_next(&iterator);
    }

cleanup:
    k_free(bloom);
    return amount;
}

////////////////////
// FILL TEST DATA //
////////////////////

static ENPeriodKey infectedPeriodKey = {
    .b = {0x75, 0xc7, 0x34, 0xc6, 0xdd, 0x1a, 0x78, 0x2d, 0xe7, 0xa9, 0x65, 0xda, 0x5e, 0xb9, 0x31, 0x25}};
static ENPeriodKey dummyPeriodKey = {
    .b = {0x89, 0xa7, 0x34, 0xc6, 0xdd, 0x1a, 0x14, 0xda, 0xe7, 0x00, 0x65, 0xda, 0x6a, 0x9b, 0x13, 0x52}};

static ENPeriodIdentifierKey infectedPik;
static ENPeriodIdentifierKey dummyPik;

void fill_test_rki_data(infected_for_period_key_ctx_t* infectedIntervals, int count) {
    int infectedCount = 50;
    int spread = count / infectedCount;
    for (int i = 0; i < infectedCount; i++) {
        int intervalNumber = (i + 2) * 2;
        float range = 1.5;
        for (int j = 0; j < spread; j++) {
            int offset = (EN_INTERVAL_LENGTH / spread) * j;
            infectedIntervals[i * spread + j].infected = 0;
            infectedIntervals[i * spread + j].search_start =
                (intervalNumber - range) * EN_INTERVAL_LENGTH + offset;  // start one and a half intervals before
            infectedIntervals[i * spread + j].search_end =
                (intervalNumber + range) * EN_INTERVAL_LENGTH + offset;  // end one and a half intervals after
            en_derive_interval_identifier(&infectedIntervals[i * spread + j].interval_identifier, &infectedPik,
                                          intervalNumber);
        }
    }
}

////////////////////
// MEASURING FUNC //
////////////////////

void measure_perf(test_func_t func, const char* label, infected_for_period_key_ctx_t* infectedIntervals, int count) {
    printk("---------------------------\n'%s': starting measurement\n", label);

    fill_test_rki_data(infectedIntervals, count);

    timing_t start_time, end_time;
    uint64_t total_cycles;
    uint64_t total_ns;

    timing_init();
    timing_start();
    start_time = timing_counter_get();

    func(infectedIntervals, count);

    end_time = timing_counter_get();

    total_cycles = timing_cycles_get(&start_time, &end_time);
    total_ns = timing_cycles_to_ns(total_cycles);

    timing_stop();

    printk("\n'%s' took %lld ms\n---------------------------\n", label, total_ns / 1000000);
}

void setup_test_data() {
    en_derive_period_identifier_key(&infectedPik, &infectedPeriodKey);
    en_derive_period_identifier_key(&dummyPik, &dummyPeriodKey);

// every 100th interval has an infected record
#define INTERVAL_SPREAD 100

    for (int i = 0; i < EN_TEK_ROLLING_PERIOD; i++) {
        // create infected record
        record_t infectedRecord;
        infectedRecord.timestamp = i * EN_INTERVAL_LENGTH;
        en_derive_interval_identifier((ENIntervalIdentifier*)&infectedRecord.rolling_proximity_identifier, &infectedPik,
                                      i);
        int rc;
        if (i % INTERVAL_SPREAD == 0 && (rc = add_record(&infectedRecord))) {
            printk("err %d\n", rc);
        }

        record_t dummyRecord;
        en_derive_interval_identifier((ENIntervalIdentifier*)&dummyRecord.rolling_proximity_identifier, &dummyPik, i);
        int spread = 1;

        for (int j = 0; j < EN_INTERVAL_LENGTH / spread; j++) {
            dummyRecord.timestamp = i * EN_INTERVAL_LENGTH + j * spread + 1;
            if ((rc = add_record(&dummyRecord))) {
                printk("err %d\n", rc);
            }
        }
        printk("period %d\n", i);
    }

#define INFECTED_INTERVALS_COUNT 500
    // setup our ordered array with infected RPIs
    static infected_for_period_key_ctx_t infectedIntervals[INFECTED_INTERVALS_COUNT];

    // measure_perf(get_number_of_infected_for_multiple_intervals_dumb, "dumb", infectedIntervals,
    //              INFECTED_INTERVALS_COUNT);
    // measure_perf(get_number_of_infected_for_multiple_intervals_simple, "simple", infectedIntervals,
    //              INFECTED_INTERVALS_COUNT);
    // measure_perf(get_number_of_infected_for_multiple_intervals_optimized, "optimized", infectedIntervals,
    //              INFECTED_INTERVALS_COUNT);

    measure_perf(test_bloom_performance, "bloom", infectedIntervals, INFECTED_INTERVALS_COUNT);

    measure_perf(test_bloom_reverse_performance, "bloom reverse", infectedIntervals, INFECTED_INTERVALS_COUNT);
}
