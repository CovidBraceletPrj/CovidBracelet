/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <inttypes.h>
#include <bluetooth/hci.h>
#include <zephyr/types.h>

#include "exposure-notification.h"
#include "covid_types.h"
#include "contacts.h"
#include "gatt_service.h"
#include "covid.h"

/*
 * Devicetree helper macro which gets the 'flags' cell from a 'gpios'
 * property, or returns 0 if the property has no 'flags' cell.
 */

#define FLAGS_OR_ZERO(node)						\
	COND_CODE_1(DT_PHA_HAS_CELL(node, gpios, flags),		\
		    (DT_GPIO_FLAGS(node, gpios)),			\
		    (0))

/*
 * Get button configuration from the devicetree sw0 alias.
 *
 * At least a GPIO device and pin number must be provided. The 'flags'
 * cell is optional.
 */

//on my NRF board this is button 1
#define SW0_NODE	DT_ALIAS(sw0)

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW0_GPIO_LABEL	DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS	(GPIO_INPUT | FLAGS_OR_ZERO(SW0_NODE))
#else
#error "Unsupported board: sw0 devicetree alias is not defined"
#define SW0_GPIO_LABEL	""
#define SW0_GPIO_PIN	0
#define SW0_GPIO_FLAGS	0
#endif

//on my NRF board this is button 2
#define SW1_NODE	DT_ALIAS(sw1)

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
#define SW1_GPIO_LABEL	DT_GPIO_LABEL(SW1_NODE, gpios)
#define SW1_GPIO_PIN	DT_GPIO_PIN(SW1_NODE, gpios)
#define SW1_GPIO_FLAGS	(GPIO_INPUT | FLAGS_OR_ZERO(SW1_NODE))
#else
#error "Unsupported board: sw1 devicetree alias is not defined"
#define SW1_GPIO_LABEL	""
#define SW1_GPIO_PIN	0
#define SW1_GPIO_FLAGS	0
#endif

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#if DT_PHA_HAS_CELL(LED0_NODE, gpios, flags)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#endif
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#endif

#ifndef FLAGS
#define FLAGS	0
#endif


static struct gpio_callback button_0_cb_data;
static struct gpio_callback button_1_cb_data;

void button_0_pressed(struct device *dev, struct gpio_callback *cb, u32_t pins){
	set_infection(true);
	gpio_pin_set(dev, PIN, (int)1);
	printk("Button 0 (=infected) pressed at %" PRIu32 "\n", k_cycle_get_32());
}

void button_1_pressed(struct device *dev, struct gpio_callback *cb, u32_t pins){
	set_infection(false);
	gpio_pin_set(dev, PIN, (int)0);
	printk("Button 1 (=healthy) pressed at %" PRIu32 "\n", k_cycle_get_32());
}

int init_io(){
	struct device *button0, *button1;
	int err = 0;
	//struct device *led;

	button0 = device_get_binding(SW0_GPIO_LABEL);
	if (button0 == NULL) {
		printk("Error: didn't find %s device\n", SW0_GPIO_LABEL);
		return -1;
	}
	button1 = device_get_binding(SW1_GPIO_LABEL);
	if (button1 == NULL) {
		printk("Error: didn't find %s device\n", SW1_GPIO_LABEL);
		return -1;
	}

	err = gpio_pin_configure(button0, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
	if (err != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       err, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return err;
	}

	err = gpio_pin_configure(button1, SW1_GPIO_PIN, SW1_GPIO_FLAGS);
	if (err != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       err, SW1_GPIO_LABEL, SW1_GPIO_PIN);
		return err;
	}


	err = gpio_pin_interrupt_configure(button0,
					   SW0_GPIO_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			err, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return err;
	}

	err = gpio_pin_interrupt_configure(button1,
					   SW1_GPIO_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			err, SW1_GPIO_LABEL, SW1_GPIO_PIN);
		return err;
	}

	gpio_init_callback(&button_0_cb_data, button_0_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button0, &button_0_cb_data);
	printk("Set up button at %s pin %d\n", SW0_GPIO_LABEL, SW0_GPIO_PIN);

	gpio_init_callback(&button_1_cb_data, button_1_pressed, BIT(SW1_GPIO_PIN));
	gpio_add_callback(button1, &button_1_cb_data);
	printk("Set up button at %s pin %d\n", SW1_GPIO_LABEL, SW1_GPIO_PIN);

	struct device *dev;
	dev = device_get_binding(LED0);
	if (dev == NULL) {
		return -1;
	}

	err = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (err < 0) {
		printk("Error %d: failed to configure leds\n", err);
		return err;
	}
	//Turn LED 0 off
	gpio_pin_set(dev, PIN, (int)0);
	return 0;
}

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
