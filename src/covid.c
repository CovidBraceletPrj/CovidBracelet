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
#include "ens/storage.h"

#ifndef COVID_MEASURE_PERFORMANCE
#define COVID_MEASURE_PERFORMANCE 0
#endif

typedef struct covid_adv_svd
{
	uint16_t ens;
	rolling_proximity_identifier_t rolling_proximity_identifier;
	associated_encrypted_metadata_t associated_encrypted_metadata;
} __packed covid_adv_svd_t;

const static bt_metadata_t bt_metadata = {
	.version = 0b00100000,
	.tx_power = 0, //TODO set to actual transmit power
	.rsv1 = 0,
	.rsv2 = 0,
};

#define COVID_ENS (0xFD6F)

static covid_adv_svd_t covid_adv_svd = {
	.ens = COVID_ENS,
	//do not initialiuze the rest of the packet, will write this later
};

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x6f, 0xfd), //0xFD6F Exposure Notification Service
	BT_DATA(BT_DATA_SVC_DATA16, &covid_adv_svd, sizeof(covid_adv_svd_t))};

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type, struct net_buf_simple *buf)
{
	if (adv_type == 3)
	{
		uint8_t len = 0;

		while (buf->len > 1)
		{
			uint8_t type;

			len = net_buf_simple_pull_u8(buf);
			if (!len)
			{
				break;
			}
			/* Check if field length is correct */
			if (len > buf->len || buf->len < 1)
			{
				break;
			}
			type = net_buf_simple_pull_u8(buf);
			if (type == BT_DATA_SVC_DATA16 && len == sizeof(covid_adv_svd_t) + 1)
			{
				covid_adv_svd_t *rx_adv = (covid_adv_svd_t *)buf->data;
				if (rx_adv->ens == COVID_ENS)
				{
                    printk("Attempting to store contact...\n");
                    record_t contact;
                    uint32_t timestamp = k_uptime_get() / 1000;
                    memcpy(&contact.rssi, &rssi, sizeof(contact.rssi));
                    memcpy(&contact.associated_encrypted_metadata, &rx_adv->associated_encrypted_metadata, sizeof(contact.associated_encrypted_metadata));
                    memcpy(&contact.rolling_proximity_identifier, &rx_adv->rolling_proximity_identifier, sizeof(contact.rolling_proximity_identifier));
                    memcpy(&contact.timestamp, &timestamp, sizeof(contact.timestamp));
                    int rc = add_record(&contact);
                    printk("Contact stored (err %d)\n", rc);
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

static void test_against_fixtures(void)
{
	// First define base values
	ENIntervalNumber intervalNumber = 2642976;
	ENPeriodKey periodKey = {.b = {0x75, 0xc7, 0x34, 0xc6, 0xdd, 0x1a, 0x78, 0x2d, 0xe7, 0xa9, 0x65, 0xda, 0x5e, 0xb9, 0x31, 0x25}};
	unsigned char metadata[4] = {0x40, 0x08, 0x00, 0x00};

	// define the expected values
	ENPeriodIdentifierKey expectedPIK = {.b = {0x18, 0x5a, 0xd9, 0x1d, 0xb6, 0x9e, 0xc7, 0xdd, 0x04, 0x89, 0x60, 0xf1, 0xf3, 0xba, 0x61, 0x75}};
	ENPeriodMetadataEncryptionKey expectedPMEK = {.b = {0xd5, 0x7c, 0x46, 0xaf, 0x7a, 0x1d, 0x83, 0x96, 0x5b, 0x9b, 0xed, 0x8b, 0xd1, 0x52, 0x93, 0x6a}};

	ENIntervalIdentifier expectedIntervalIdentifier = {.b = {0x8b, 0xe6, 0xcd, 0x37, 0x1c, 0x5c, 0x89, 0x16, 0x04, 0xbf, 0xbe, 0x49, 0xdf, 0x84, 0x50, 0x96}};
	unsigned char expectedEncryptedMetadata[4] = {0x72, 0x03, 0x38, 0x74};

	ENPeriodIdentifierKey pik;
	en_derive_period_identifier_key(&pik, &periodKey);

	printk("expectedPIK: ");
	print_key(&expectedPIK);
	printk(", ");
	printk("actualPIK: ");
	print_key(&pik);
	printk(", ");

	ENIntervalIdentifier intervalIdentifier;
	en_derive_interval_identifier(&intervalIdentifier, &pik, intervalNumber);

	printk("expectedRPI: ");
	print_key(&expectedIntervalIdentifier);
	printk(", ");
	printk("actualRPI: ");
	print_key(&intervalIdentifier);
	printk(", ");

	/*ENPeriodMetadataEncryptionKey pmek;
    en_derive_period_metadata_encryption_key(&pmek, &periodKey);
    TEST_ASSERT_EQUAL_KEY(expectedPMEK, pmek);

    unsigned char encryptedMetadata[sizeof(metadata)] = {0};
    en_encrypt_interval_metadata(&pmek, &intervalIdentifier, metadata, encryptedMetadata, sizeof(metadata));
    TEST_ASSERT_EQUAL_CHAR_ARRAY(expectedEncryptedMetadata, encryptedMetadata, sizeof(expectedEncryptedMetadata));*/
}

static void new_period_key(time_t currentTime)
{
	printk("\n----------------------------------------\n\n");
	printk("\n----------------------------------------\n\n");
	printk("*** New Period\n");
	current_period_index = period_cnt % NUM_PERIOD_KEYS;
	periods[current_period_index].periodInterval = en_get_interval_number_at_period_start(currentTime);
	printk("periodInterval %u\n", periods[current_period_index].periodInterval);
	en_generate_period_key(&periods[current_period_index].periodKey);
	period_cnt++;
}

#if COVID_MEASURE_PERFORMANCE
static void measure_performance()
{

	u32_t runs = 100;
	u32_t start_time;
	u32_t cycles_spent;
	u32_t nanoseconds_spent;

	ENPeriodKey pk;

	ENPeriodIdentifierKey pik;
	ENIntervalIdentifier intervalIdentifier;
	ENIntervalNumber intervalNumber = 2642976;
	ENIntervalIdentifier id;
	ENPeriodMetadataEncryptionKey pmek;
	unsigned char metadata[4] = {0x40, 0x08, 0x00, 0x00};
	unsigned char encryptedMetadata[sizeof(metadata)] = {0};

	printk("\n----------------------------------------\n");
	printk("MEASURING PERFORMANCE\n");

	// Measure en_generate_period_key
	{
		start_time = k_cycle_get_32();

		for (int i = 0; i < runs; i++)
		{
			en_generate_period_key(&pk);
		}

		nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32() - start_time);
		printk("en_generate_period_key %d ns\n", nanoseconds_spent/runs);
	}

	// Measure en_derive_period_identifier_key
	{
		start_time = k_cycle_get_32();

		for (int i = 0; i < runs; i++)
		{
			en_derive_period_identifier_key(&pik, &pk);
		}

		nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32() - start_time);
		printk("en_derive_period_identifier_key %d ns\n", nanoseconds_spent/runs);
	}

	// Measure en_derive_interval_identifier
	{
		start_time = k_cycle_get_32();

		for (int i = 0; i < runs; i++)
		{
			en_derive_interval_identifier(&intervalIdentifier, &pik, intervalNumber);
		}

		nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32() - start_time);
		printk("en_derive_interval_identifier %d ns\n", nanoseconds_spent/runs);
	}

	// Measure en_derive_period_metadata_encryption_key
	{
		start_time = k_cycle_get_32();

		for (int i = 0; i < runs; i++)
		{
			en_derive_period_metadata_encryption_key(&pmek, &pk);
		}

		nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32() - start_time);
		printk("en_derive_period_metadata_encryption_key %d ns\n", nanoseconds_spent/runs);
	}

	// Measure en_encrypt_interval_metadata
	{
		start_time = k_cycle_get_32();

		for (int i = 0; i < runs; i++)
		{
			en_encrypt_interval_metadata(&pmek, &intervalIdentifier, metadata, encryptedMetadata, sizeof(metadata));
		}

		nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32() - start_time);
		printk("en_encrypt_interval_metadata %d ns\n", nanoseconds_spent/runs);
	}

	// Measure Full key generation
	{
		start_time = k_cycle_get_32();

		for (int i = 0; i < runs; i++)
		{
			ENPeriodKey pk;
			en_generate_period_key(&pk);
			ENPeriodIdentifierKey ik;
			en_derive_period_identifier_key(&ik, &pk);

			for(int iv = 0; iv < EN_TEK_ROLLING_PERIOD; iv++) {
				ENIntervalNumber intervalNumber = en_get_interval_number(iv);
				ENIntervalIdentifier id;
				en_derive_interval_identifier(&id, &ik, intervalNumber);
			}
		}

		nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32() - start_time);
		printk("Full key generation %d ns\n", nanoseconds_spent/runs);
	}

	printk("\FINISHED\n");
	printk("----------------------------------------\n\n");
}
#endif

//To be called when new keys are needed
static void check_keys(struct k_work *work)
{

	// we check the current time to know if we actually need to regenerate anything
	// TODO: Use real unix timestamp!: currentTime = time(NULL);
	time_t currentTime = k_uptime_get() / 1000;
	ENIntervalNumber newInterval = en_get_interval_number(currentTime);
	if (currentInterval != newInterval || init)
	{
		currentInterval = newInterval;
		bool newPeriod = ((currentInterval - periods[current_period_index].periodInterval) >= EN_TEK_ROLLING_PERIOD);
		// we check if we need to generate new keys
		if (newPeriod || init)
		{
			new_period_key(currentTime);
		}

		// we now generate the new interval identifier and re-encrypt the metadata
		// TODO: The period identifier key does not need to be derived everytime!
		ENPeriodIdentifierKey pik;
		en_derive_period_identifier_key(&pik, &periods[current_period_index].periodKey);
		en_derive_interval_identifier(&intervalIdentifier, &pik, currentInterval);
		
		en_derive_period_metadata_encryption_key(&periodMetadataEncryptionKey, &periods[current_period_index].periodKey);
		en_encrypt_interval_metadata(&periodMetadataEncryptionKey, &intervalIdentifier, (unsigned char*)&bt_metadata, (unsigned char*)&encryptedMetadata, sizeof(associated_encrypted_metadata_t));
		

		// broadcast intervalIdentifier plus encryptedMetada according to specs
		//printk("\n----------------------------------------\n\n");
		printk("Time: %llu, ", currentTime);
		printk("Interval: %u, ", currentInterval);
		printk("TEK: ");
		print_rpi((rolling_proximity_identifier_t *)&periods[current_period_index].periodKey);
		printk(", ");
		printk("RPI: ");
		print_rpi((rolling_proximity_identifier_t *)&intervalIdentifier);
		printk(", ");
		printk("AEM: ");
		print_aem(&encryptedMetadata);
		printk("\n");

		//TODO do we have to worry about race conditions here?
		//worst case: we would be advertising a wrong key for a while
		memcpy(&covid_adv_svd.rolling_proximity_identifier, &intervalIdentifier, sizeof(rolling_proximity_identifier_t));
		memcpy(&covid_adv_svd.associated_encrypted_metadata, &encryptedMetadata, sizeof(associated_encrypted_metadata_t));

		init = 0;
	}
}

K_WORK_DEFINE(my_work, check_keys);

static void my_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&my_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

static const struct bt_le_scan_param scan_param = {
	.type = BT_HCI_LE_SCAN_PASSIVE,
	.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
	.interval = 0x0010, //Scan Interval (N * 0.625 ms), TODO: set to correct interval
	.window = 0x0010,	//Scan Window (N * 0.625 ms), TODO: set to correct interval
};

#define KEY_CHECK_INTERVAL (K_MSEC(EN_INTERVAL_LENGTH * 1000 / 10))

int init_covid()
{

#if COVID_MEASURE_PERFORMANCE
	measure_performance();
#endif

	// TODO: Use real unix timestamp!: currentTime = time(NULL);
	init = 1;
	period_cnt = 0;
	infected = 0;

	test_against_fixtures();

	check_keys(NULL);

	int err = 0;
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err)
	{
		printk("Starting scanning failed (err %d)\n", err);
		return err;
	}

	k_timer_start(&my_timer, KEY_CHECK_INTERVAL, KEY_CHECK_INTERVAL);
	return 0;
}

int do_covid()
{
	//printk("covid start\n");

	int err = 0;
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}

	k_sleep(K_SECONDS(10));

	err = bt_le_adv_stop();
	if (err)
	{
		printk("Advertising failed to stop (err %d)\n", err);
		return err;
	}
	//printk("covid end\n");
	return 0;
}

bool get_infection()
{
	return infected;
}

void set_infection(bool _infected)
{
	infected = _infected;
}

unsigned int get_period_cnt_if_infected()
{
	if (!infected)
	{
		return 0;
	}
	return period_cnt;
}

period_t *get_period_if_infected(unsigned int id, size_t *size)
{
	if (!infected || id >= NUM_PERIOD_KEYS || id >= period_cnt)
	{
		*size = 0;
		return NULL;
	}
	*size = sizeof(period_t);
	return &periods[id];
}

int get_index_by_interval(ENIntervalNumber periodInterval)
{
	int index = 0;
	while (index < NUM_PERIOD_KEYS || index < period_cnt)
	{
		if (periods[index].periodInterval == periodInterval)
		{
			return index;
		}
		index++;
	}
	return -1;
}
