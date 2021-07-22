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
    rc = bloom_add_record(record);
    return rc;
}

int get_number_of_infected_for_multiple_intervals(infected_for_period_key_ctx_t* ctx, int count) {
    record_iterator_t iterator;
    for (int i = 0; i < count; i++) {
        int rc = ens_records_iterator_init_timerange(&iterator, &ctx[i].search_start, &ctx[i].search_end);
        if (rc) {
            return rc;
        }
        while (!iterator.finished) {
            if (memcmp(&iterator.current.rolling_proximity_identifier, &ctx[i].interval_identifier,
                       sizeof(iterator.current.rolling_proximity_identifier)) == 0) {
                ctx[i].infected++;
                break;
            }
            ens_records_iterator_next(&iterator);
        }
    }
    return 0;
}

void setup_test_data() {
    ENPeriodKey infectedPeriodKey = {
        .b = {0x75, 0xc7, 0x34, 0xc6, 0xdd, 0x1a, 0x78, 0x2d, 0xe7, 0xa9, 0x65, 0xda, 0x5e, 0xb9, 0x31, 0x25}};
    ENPeriodKey dummyPeriodKey = {
        .b = {0x89, 0xa7, 0x34, 0xc6, 0xdd, 0x1a, 0x14, 0xda, 0xe7, 0x00, 0x65, 0xda, 0x6a, 0x9b, 0x13, 0x52}};

    ENPeriodIdentifierKey infectedPik;
    ENPeriodIdentifierKey dummyPik;
    en_derive_period_identifier_key(&infectedPik, &infectedPeriodKey);
    en_derive_period_identifier_key(&dummyPik, &dummyPeriodKey);

    for (int i = 0; i < EN_TEK_ROLLING_PERIOD; i++) {
        // create infected record which
        record_t infectedRecord;
        infectedRecord.timestamp = i * EN_INTERVAL_LENGTH;
        en_derive_interval_identifier((ENIntervalIdentifier*)&infectedRecord.rolling_proximity_identifier, &infectedPik,
                                      i);
        record_t dummyRecord;
        en_derive_interval_identifier((ENIntervalIdentifier*)&dummyRecord.rolling_proximity_identifier, &dummyPik, i);
        int rc;
        if (i % 3 == 0) {
            if ((rc = add_record(&infectedRecord))) {
                printk("err %d\n", rc);
            }
        }

        int spread = 4;

        for (int j = 0; j < EN_INTERVAL_LENGTH / spread; j++) {
            dummyRecord.timestamp = i * EN_INTERVAL_LENGTH + j * spread + 1;
            if ((rc = add_record(&dummyRecord))) {
                printk("err %d\n", rc);
            }
        }
        printk("period %d\n", i);
    }

    printk("starting measurement\n");

    timing_t start_time, end_time;
    uint64_t total_cycles;
    uint64_t total_ns;

    timing_init();
    timing_start();
    start_time = timing_counter_get();

    // printk("found %d infected\n", get_number_of_infected_for_period(&infectedPeriodKey, 200));

    end_time = timing_counter_get();

    total_cycles = timing_cycles_get(&start_time, &end_time);
    total_ns = timing_cycles_to_ns(total_cycles);

    timing_stop();

    printk("timing took %lld ns\n", total_ns);
}
