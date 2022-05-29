/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#define SYNC_ADV_INTERVAL_MS (60*1000)
#define SYNC_ADV_DURATION_MS 500
#define SYNC_CONN_INIT_WAIT_MS 250

K_TIMER_DEFINE(sync_adv_timer, NULL, NULL);

int sync_service_init(void) {
    // We init the timers (which should run periodically!)
    // we directly want to advertise ourselfs after the start -> should reduce unwanted delays
    k_timer_start(&sync_adv_timer, K_MSEC(SYNC_ADV_INTERVAL_MS), K_MSEC(SYNC_ADV_INTERVAL_MS));
    return 0;
}

void sync_service_handle_connection() {

    // TODO: Implement me!
}

uint32_t sync_service_run(void) {

    if (k_timer_status_get(&sync_adv_timer) > 0) {

         // TODO: START ADVERTISEMENTS
         //printk("Advertising Sync service...!\n");
         k_sleep(K_MSEC(SYNC_ADV_DURATION_MS));
         // TODO: STOP ADVERTISEMENTS
    }
     // TODO: CHECK IF A CONNECTION HAPPENED UNTIL NOW if so -> handle it!

    // we return the timer time so that main can sleep
    return k_timer_remaining_get(&sync_adv_timer);
}