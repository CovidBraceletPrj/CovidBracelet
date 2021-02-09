/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <sys/printk.h>
#include <bluetooth/hci.h>
#include <random/rand32.h>

#include "battery.h"
#include "exposure-notification.h"
#include "covid_types.h"
#include "contacts.h"
#include "gatt_service.h"
#include "covid.h"
#include "io.h"
#include "display.h"

void main(void)
{
	int err = 0;
	printk("Starting Covid Contact Tracer\n");

	err = platform_display_init();
	if (err) {
		printk("Failed to initialize display: %d\n", err);
		return;
	}

	platform_display_set_powersave(false);
	platform_display_set_brightness(0xff);
	platform_display_draw_string(0, 0, "Hello World!");
	platform_display_draw_string(0, DISPLAY_LINE_LAST_CONTACTS_START, "Seen identifiers:");

#ifdef BATTERY_SUPPORTED
	err = battery_init();
	if (err) {
		printk("Failed to initialize battery gauging: %d\n", err);
		return;
	}

	err = battery_update();
	if (err) {
		printk("Failed to update battery voltage: %d\n", err);
		return;
	}

	platform_display_draw_string(0, DISPLAY_LINE_BATTERY_VOLTAGE, "Battery voltage:");
#ifdef BATTERY_SOC_SUPPORTED
	platform_display_draw_string(0, DISPLAY_LINE_BATTERY_VOLTAGE + 2, "Battery SOC:");
#endif
#endif

    // first init everything
	// Use custom randomization as the mbdet_tls context initialization messes with the Zeyhr BLE stack.
    err = en_init(sys_csrand_get);
	if (err) {
		printk("Cyrpto init failed (err %d)\n", err);
		return;
	}

	printk("init contacts\n");
	init_contacts();
	err = init_io();
	if(err){
		printk("Button init failed (err %d)\n", err);
		return;
	}

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = init_gatt();
	if (err) {
		printk("init gatt failed(err %d)\n", err);
		return;
	}

	err = init_covid();
	if (err) {
		printk("init covid failed (err %d)\n", err);
		return;
	}

	do {
#ifdef BATTERY_SUPPORTED
		err = battery_update();
		if (err) {
			printk("Failed to update battery voltage: %d\n", err);
		} else {
			char tmpstr[12] = { 0 };

			snprintf(tmpstr, sizeof(tmpstr), "%04u mV", battery_get_voltage_mv());
			platform_display_draw_string(0, DISPLAY_LINE_BATTERY_VOLTAGE + 1, tmpstr);

#ifdef BATTERY_SOC_SUPPORTED
			snprintf(tmpstr, sizeof(tmpstr), "%3u%%", battery_voltage_mv_to_soc(battery_get_voltage_mv()));
			platform_display_draw_string(0, DISPLAY_LINE_BATTERY_VOLTAGE + 3, tmpstr);
		}
#endif
#endif
		do_covid();
		do_gatt();
	} while (1);
	
}
