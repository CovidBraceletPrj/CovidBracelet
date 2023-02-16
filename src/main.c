/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <bluetooth/hci.h>
#include <random/rand32.h>
#include <sys/printk.h>

#include "record_storage.h"
#include "tek_storage.h"
#include "sync_service.h"
#include "tracing.h"
#include "bloom.h"

#include "mbedtls/platform.h"


#define BLOOM_TEST 0
#define CLEAN_INIT (BLOOM_TEST)

 #if BLOOM_TEST
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

void bloom_test() {
    printk("Filling record storage...\n");
    reset_record_storage();

    tek_t tek;
    ENPeriodIdentifierKey pik;

    tek_storage_get_latest_at_ts(&tek, 0);
    en_derive_period_identifier_key(&pik, &tek.tek);


    bloom_filter_t* bf = bloom_init(0); // size not used atm

    /// we now fill the whole flash memory with stupid data
    for (int i = 0; i < CONFIG_ENS_MAX_CONTACTS; ++i) {
        record_t dummy_record;
        en_derive_interval_identifier((ENIntervalIdentifier*)&dummy_record.rolling_proximity_identifier, &pik, i);
        dummy_record.timestamp = i; // very clever! ;)
        bloom_add_record(bf, &dummy_record.rolling_proximity_identifier);
        if (!bloom_probably_has_record(bf, &dummy_record.rolling_proximity_identifier)) {
            printk("error!  bloom_probably_has_record\n");
        }
    }
    printk("Done...\n");
    k_msleep(2000);

    printk("Creating bloom filter...\n");
    fill_bloom_with_stored_records(bf);
    printk("Done...\n");
    k_msleep(3000);

    do {
        printk("Checking keys...\n");
        for(int k = 0; k < 144; k++) {

            ENIntervalIdentifier rpi;
            en_derive_period_identifier_key(&pik, &tek.tek);
            // derive all RPIs
            for (int j = 0; j < 144; j++) {

                // we assume that at least one out of 144 RPIs was met (this is still a lot actually!)
                uint32_t interval = j == 0 ? (CONFIG_ENS_MAX_CONTACTS / 144)*k : (CONFIG_ENS_MAX_CONTACTS+1);
                en_derive_interval_identifier(&rpi, &pik, interval); //one of each is actually

                uint32_t num_met = 0;
                if (bloom_probably_has_record(bf, &rpi)) {
                    num_met++;
                    /*
                    record_iterator_t iterator;

                    time_t start = interval == 0 ? 0 : (interval+(CONFIG_ENS_MAX_CONTACTS-1)) % CONFIG_ENS_MAX_CONTACTS;
                    time_t end = (interval+(CONFIG_ENS_MAX_CONTACTS+1)) % CONFIG_ENS_MAX_CONTACTS;

                    int rc = ens_records_iterator_init_timerange(&iterator, &start, &end);
                    if (rc) {
                        // on error, skip this rpi
                        printk("iterator error! %d\n", rc);
                        continue;
                    }
                    record_t* current;
                    while ((current = ens_records_iterator_next(&iterator))) {
                        if (memcmp(&(current->rolling_proximity_identifier), rpi.b,
                                   sizeof(current->rolling_proximity_identifier)) == 0) {
                            num_met++;
                        }
                    }
                    ens_record_iterator_clear(&iterator);*/
                }
                if(j == 0 && num_met == 0) {
                    printk("Met mismatch! %u\n", num_met);
                }
            }

        }
        k_sleep(K_MSEC(2000));
    } while (1);
}
#endif

void main(void) {

    int err = 0;
    printk("Starting Contact-Tracing Wristband...\n");

     int ret = 0;

    if((ret = mbedtls_platform_setup(NULL)) != 0) {
        mbedtls_printf("Platform initialization failed with error %d\r\n", ret);
        return;
    }

    // Use custom randomization as the mbdet_tls context initialization messes with the Zeyhr BLE stack.
    err = en_init(sys_csrand_get);
    if (err) {
        printk("Crypto init failed (err %d)\n", err);
        return;
    }

    err = record_storage_init(CLEAN_INIT);
    if (err) {
        printk("init record storage failed (err %d)\n", err);
        return;
    }


    err = tek_storage_init(CLEAN_INIT);
    if (err) {
        printk("init key storage failed (err %d)\n", err);
        return;
    }

    #if BLOOM_TEST
    k_msleep(10000);
    bloom_test();
    return;
    #endif

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    /* Initialize the Tracing Subsystem */
    err = tracing_init();
    if (err) {
        printk("Tracing init failed (err %d)\n", err);
        return;
    }

    /* Initialize the Gatt Subsystem */
    err = sync_service_init();
    if (err) {
        printk("Sync Service init failed (err %d)\n", err);
        return;
    }

    printk("Components initialized! Starting Tracing and Gatt...\n");

    // We sleep just for one second
    k_sleep(K_MSEC(1000));

    do {
        uint32_t tracing_sleep_ms = tracing_run();
        uint32_t sync_sleep_ms = sync_service_run();

        uint32_t sleep_ms = MIN(tracing_sleep_ms, sync_sleep_ms);
        //printk("Sleeping a bit (%u ms)...\n", sleep_ms);
        k_sleep(K_MSEC(sleep_ms)); // TODO: what to put here?
    } while (1);
}