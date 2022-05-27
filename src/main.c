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

void main(void) {

    int err = 0;
    printk("Starting Contact-Tracing Wristband...\n");

    // Use custom randomization as the mbdet_tls context initialization messes with the Zeyhr BLE stack.
    err = en_init(sys_csrand_get);
    if (err) {
        printk("Crypto init failed (err %d)\n", err);
        return;
    }

    err = record_storage_init(false);
    if (err) {
        printk("init record storage failed (err %d)\n", err);
        return;
    }

    err = tek_storage_init(false);
    if (err) {
        printk("init key storage failed (err %d)\n", err);
        return;
    }

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

    do {
        tracing_run();
        sync_service_run();
    } while (1);
}