/*
 * Copyright (c) 2020 Olaf Landsiedel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "exposure-notification.h"
#include "covid_types.h"
#include "contacts.h"
#include "covid.h"

typedef struct covid_adv_svd {
	uint16_t ens;
	rolling_proximity_identifier_t rolling_proximity_identifier;
	associated_encrypted_metadata_t associated_encrypted_metadata;
}  __packed covid_adv_svd_t;

const static bt_metadata_t bt_metadata= {
	.version = 0b00100000,
	.tx_power = 0, //TODO set to actual transmit power
	.rsv1 = 0,
	.rsv2 = 0,
};

#define COVID_ENS (0xFD6F)

static covid_adv_svd_t covid_adv_svd= {
	.ens = COVID_ENS,
	//do not initialiuze the rest of the packet, will write this later
};

static struct bt_data ad[] = {
     BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
     BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x6f, 0xfd),  //0xFD6F Exposure Notification Service
     BT_DATA(BT_DATA_SVC_DATA16, &covid_adv_svd, sizeof(covid_adv_svd_t))
 };

 static void scan_cb(const bt_addr_le_t *addr, s8_t rssi, u8_t adv_type, struct net_buf_simple *buf)
{
	if( adv_type == 3 ){
		u8_t len = 0;

		while (buf->len > 1) {
			u8_t type;

			len = net_buf_simple_pull_u8(buf);
			if (!len) {
				break;
			}
			/* Check if field length is correct */
			if (len > buf->len || buf->len < 1) {
				break;
			}
			type = net_buf_simple_pull_u8(buf);
			if (type == BT_DATA_SVC_DATA16 && len == sizeof(covid_adv_svd_t) + 1){
				covid_adv_svd_t* rx_adv = (covid_adv_svd_t*) buf->data;
				if (rx_adv->ens == COVID_ENS){
					check_add_contact(k_uptime_get() / 1000, &rx_adv->rolling_proximity_identifier, &rx_adv->associated_encrypted_metadata, rssi);
				}

			}
			net_buf_simple_pull(buf, len - 1); //consume the rest, note we already consumed one byte via net_buf_simple_pull_u8(buf) 
		}
	}
}

#define NUM_PERIOD_KEYS (14)
static period_t periods[NUM_PERIOD_KEYS];

static int current_period_index = 0;
static ENIntervalNumber currentInterval;
static unsigned int period_cnt = 0;

static ENPeriodMetadataEncryptionKey periodMetadataEncryptionKey;
static ENIntervalIdentifier intervalIdentifier;
static associated_encrypted_metadata_t encryptedMetadata;

static bool init = 1;
static bool infected = 0;

static void new_period_key(time_t currentTime ){
	printk("\n----------------------------------------\n\n");
	printk("\n----------------------------------------\n\n");
	printk("*** New Period\n");
	current_period_index = period_cnt % NUM_PERIOD_KEYS;
	periods[current_period_index].periodInterval = en_get_interval_number_at_period_start(currentTime);
	printk("periodInterval %u\n", periods[current_period_index].periodInterval);
	en_generate_period_key(&periods[current_period_index].periodKey);
	period_cnt++;
}

//To be called when new keys are needed
static void check_keys(struct k_work *work){

	// we check the current time to know if we actually need to regenerate anything
	// TODO: Use real unix timestamp!: currentTime = time(NULL);
	time_t currentTime = k_uptime_get() / 1000;
	ENIntervalNumber newInterval = en_get_interval_number(currentTime);
	if( currentInterval != newInterval || init){
		currentInterval = newInterval;
		bool newPeriod = ((currentInterval - periods[current_period_index].periodInterval) >= EN_TEK_ROLLING_PERIOD);
		// we check if we need to generate new keys
		if (newPeriod || init) {
			new_period_key(currentTime);
		}

		// we now generate the new interval identifier and re-encrypt the metadata
		en_derive_interval_identifier(&intervalIdentifier, &periods[current_period_index].periodKey, currentInterval);
		en_encrypt_interval_metadata(&periodMetadataEncryptionKey, &intervalIdentifier, (unsigned char*)&bt_metadata, (unsigned char*)&encryptedMetadata, sizeof(associated_encrypted_metadata_t));

		// broadcast intervalIdentifier plus encryptedMetada according to specs
		//printk("\n----------------------------------------\n\n");
		printk("Time: %llu, ", currentTime);
		printk("Interval: %u, ", currentInterval);
		printk("RPI: "); print_rpi((rolling_proximity_identifier_t*)&intervalIdentifier); printk(", ");
		printk("AEM: "); print_aem(&encryptedMetadata); printk("\n");

		//TODO do we have to worry about race conditions here?
		//worst case: we would be advertising a wrong key for a while
		memcpy(&covid_adv_svd.rolling_proximity_identifier, &intervalIdentifier, sizeof(rolling_proximity_identifier_t));
		memcpy(&covid_adv_svd.associated_encrypted_metadata, &encryptedMetadata, sizeof(associated_encrypted_metadata_t));

		if( !init ){
			key_change(current_period_index);
		}
		init = 0;

	}
}

K_WORK_DEFINE(my_work, check_keys);

static void my_timer_handler(struct k_timer *dummy){
    k_work_submit(&my_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

static const struct bt_le_scan_param scan_param = {
	.type       = BT_HCI_LE_SCAN_PASSIVE,
	.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
	.interval   = 0x0010, //Scan Interval (N * 0.625 ms), TODO: set to correct interval
	.window     = 0x0010, //Scan Window (N * 0.625 ms), TODO: set to correct interval
};

#define KEY_CHECK_INTERVAL (K_MSEC(EN_INTERVAL_LENGTH * 1000 / 10))

int init_covid(){
	// TODO: Use real unix timestamp!: currentTime = time(NULL);
	init = 1;
	period_cnt = 0;
	infected = 0;
	check_keys(NULL);

	int err = 0;
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return err;
	}

	k_timer_start(&my_timer, KEY_CHECK_INTERVAL, KEY_CHECK_INTERVAL);
	return 0;
}

int do_covid(){
	//printk("covid start\n");

    int err = 0;
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}		
		
	k_sleep(K_SECONDS(10));

	err = bt_le_adv_stop();
	if (err) {
		printk("Advertising failed to stop (err %d)\n", err);
		return err;
	}
	//printk("covid end\n");
	return 0;
}

bool get_infection(){
	return infected;
}

void set_infection(bool _infected){
	infected = _infected;
}

unsigned int get_period_cnt_if_infected(){
	if( !infected ){
		return 0;
	}
	return period_cnt;
}

period_t* get_period_if_infected(unsigned int id, size_t* size){
	if( !infected || id >= NUM_PERIOD_KEYS || id >= period_cnt ){
		*size = 0;
		return NULL;
	}
	*size = sizeof(period_t);
	return &periods[id];
}

int get_index_by_interval(ENIntervalNumber periodInterval){
	int index = 0;
	while( index < NUM_PERIOD_KEYS || index < period_cnt ){
		if( periods[index].periodInterval == periodInterval){
			return index;
		}
		index++;
	}
	return -1;
}
