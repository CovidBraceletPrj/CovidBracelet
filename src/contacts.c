/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include "covid_types.h"
#include "contacts.h"
#include "exposure-notification.h"
#include "covid.h"

static contact_t contacts[MAX_CONTACTS];
static uint32_t contact_count = 0;

static period_contacts_t period_contacts[PERIODS];
static int period_index = 0;
static int32_t next_infected_key_id = 0;

void print_key(_ENBaseKey* key){
    for( int i = 0; i < sizeof(key->b); i++){
        printk("%02x", key->b[i]);
    }
}

void print_rpi(rolling_proximity_identifier_t* rpi){
    for( int i = 0; i < sizeof(rolling_proximity_identifier_t); i++){
        printk("%02x", rpi->data[i]);
    }
}

void print_aem(associated_encrypted_metadata_t* aem){
    for( int i = 0; i < sizeof(associated_encrypted_metadata_t); i++){
        printk("%02x", aem->data[i]);
    }
}

contact_t* find_contact(rolling_proximity_identifier_t* rpi, associated_encrypted_metadata_t* aem){
    for( int i = 0; i < contact_count; i++ ){
        //print_rpi(&contacts[i].rolling_proximity_identifier);
        //print_aem(&contacts[i].associated_encrypted_metadata);
        if ( memcmp(&contacts[i].rolling_proximity_identifier, rpi, sizeof(rolling_proximity_identifier_t)) == 0 &&
        memcmp(&contacts[i].associated_encrypted_metadata, aem, sizeof(associated_encrypted_metadata_t)) == 0 ) {
            return &contacts[i];
        }
    }
    return NULL;
}

int check_add_contact(uint32_t contact_time, rolling_proximity_identifier_t* rpi, associated_encrypted_metadata_t* aem, int8_t rssi){
    contact_t* contact = find_contact(rpi, aem);
    if( contact == NULL ){
        if( contact_count >= MAX_CONTACTS ){
            printk("out of contact buffers\n");
            return -1;
        }
        printk("adding contact: rpi ");
        print_rpi(rpi);
        printk(" aem ");
        print_aem(aem);
        printk(" rssi %i \n", rssi);

        contacts[contact_count].most_recent_contact_time = contact_time;
        contacts[contact_count].first_contact_time = contact_time;
        contacts[contact_count].cnt = 1;
        contacts[contact_count].max_rssi = rssi;
        memcpy(&contacts[contact_count].rolling_proximity_identifier, rpi, sizeof(rolling_proximity_identifier_t));
        memcpy(&contacts[contact_count].associated_encrypted_metadata, aem, sizeof(associated_encrypted_metadata_t));
        contact_count++;
    } else {
        contact->most_recent_contact_time = contact_time;
        if( contacts->cnt < 0xFFFF ){ //avoid overflows
            contacts->cnt++;
        }
        if( rssi > contact->max_rssi ){
            contact->max_rssi = rssi;
        }
        // printk("update contact: rpi ");
        // print_rpi(rpi);
        // printk(" aem ");
        // print_aem(aem);
        // printk(" rssi %i, cnt %i \n", rssi, contacts->cnt);
    }
    return 0;
}



//10 minutes are over and we got new keys. Time to also sort our short term contacts and move them to long-term log
//TODO: move long-term storage to flash, as we have limited space in RAM
void key_change(int current_period_index){
    if(current_period_index >= PERIODS){
        printk("error, current periods index %i too large", current_period_index);
        return;
    }
    if(current_period_index != period_index){
        //printk("new period index\n");
        period_index = current_period_index;
        period_contacts[period_index].cnt = 0;
    }
    
    //check all short-term contacts (as long as we have space in our long-term storage)
    for( int i = 0; i < contact_count && period_contacts[period_index].cnt < MAX_PERIOD_CONTACTS; i++){
        //printk("check contact %i, duration %i\n", i, contacts[i].most_recent_contact_time - contacts[i].first_contact_time);
        int index = period_contacts[period_index].cnt;
        if( (contacts[i].most_recent_contact_time - contacts[i].first_contact_time) > (EN_INTERVAL_LENGTH / 2)){
            period_contacts[period_index].period_contacts[index].duration = contacts[i].most_recent_contact_time - contacts[i].first_contact_time;
            period_contacts[period_index].period_contacts[index].cnt = contacts[i].cnt;
            period_contacts[period_index].period_contacts[index].max_rssi = contacts[i].max_rssi;
            memcpy(&period_contacts[period_index].period_contacts[index].rolling_proximity_identifier,  &contacts[i].rolling_proximity_identifier, sizeof(rolling_proximity_identifier_t));
            memcpy(&period_contacts[period_index].period_contacts[index].associated_encrypted_metadata,  &contacts[i].associated_encrypted_metadata, sizeof(associated_encrypted_metadata_t));

            printk("store contact %i as exposure %i: rpi ", i, period_contacts[period_index].cnt);
            print_rpi(&period_contacts[period_index].period_contacts[index].rolling_proximity_identifier);
            printk(" aem ");
            print_aem(&period_contacts[period_index].period_contacts[index].associated_encrypted_metadata);
            printk(" max rssi %i, cnt %u, duration %u\n", period_contacts[period_index].period_contacts[index].max_rssi, period_contacts[period_index].period_contacts[index].cnt, period_contacts[period_index].period_contacts[index].duration);
            period_contacts[period_index].cnt++;
        }
    }
    contact_count = 0;
}

/* The devicetree node identifier for the "led1" alias. */
#define LED1_NODE DT_ALIAS(led1)

#if DT_NODE_HAS_STATUS(LED1_NODE, okay)
#define LED1	DT_GPIO_LABEL(LED1_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED1_NODE, gpios)
#if DT_PHA_HAS_CELL(LED1_NODE, gpios, flags)
#define FLAGS	DT_GPIO_FLAGS(LED1_NODE, gpios)
#endif
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED1	""
#define PIN	0
#endif

#ifndef FLAGS
#define FLAGS	0
#endif

struct device *dev;

void add_infected_key(period_t* period){
    
	//printk("Interval: %u\n", period->periodInterval);
	//printk("RPI: "); print_rpi((rolling_proximity_identifier_t*)&period->periodKey); printk("\n");
    next_infected_key_id++;
    //find correct "day", TODO: also check a bit before and after
    int index = get_index_by_interval(period->periodInterval);
    if( index < 0 ){
        printk("Exposure check: period %i not found\n", period->periodInterval);
        return;
    }
    for( int i = 0; i < EN_TEK_ROLLING_PERIOD; i++){
        static ENIntervalIdentifier intervalIdentifier;
        en_derive_interval_identifier(&intervalIdentifier, &period->periodKey, period->periodInterval + i);
        //go through all long-term contacts for this day and check if I have seen the intervalIdentifier
        for( int j = 0; j < period_contacts[index].cnt; j++){
            int ret = memcmp(&period_contacts[index].period_contacts[j].rolling_proximity_identifier, &intervalIdentifier, sizeof(rolling_proximity_identifier_t));
            if( ret == 0 ){
                printk("Found exposure: rpi ");
                print_rpi(&period_contacts[index].period_contacts[j].rolling_proximity_identifier);
                printk(" aem ");
                print_aem(&period_contacts[index].period_contacts[j].associated_encrypted_metadata);
                printk(" max rssi %i, cnt %u, duration %u\n", period_contacts[index].period_contacts[j].max_rssi, period_contacts[index].period_contacts[j].cnt, period_contacts[index].period_contacts[j].duration);
                gpio_pin_set(dev, PIN, (int)1);
            }
        }
    }

}

uint32_t get_next_infected_key_id(){
    return next_infected_key_id;
}

void init_contacts(){
    contact_count = 0;
    period_index = 0;

	dev = device_get_binding(LED1);
	if (dev == NULL) {
		return;
	}

	int ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0) {
		return;
	}

    gpio_pin_set(dev, PIN, (int)0);

}
