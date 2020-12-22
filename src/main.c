/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <sys/printk.h>
#include <bluetooth/hci.h>
#include <random/rand32.h>

#include "exposure-notification.h"
#include "covid_types.h"
#include "contacts.h"
#include "gatt_service.h"
#include "covid.h"
#include "io.h"

void main(void)
{
	int err = 0;
	printk("Starting Covid Contact Tracer\n");

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

	do{
		do_covid();
		do_gatt();
	} while (1);
	
}
