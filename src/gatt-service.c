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

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "covid.h"
#include "contacts.h"

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	/* My Service UUID (same as above) */
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
		0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0xF2),
};

static void connected(struct bt_conn *conn, uint8_t err)
{

	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

int init_gatt(void)
{
	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	return 0;
}

int do_gatt(void){
	//printk("gatt start\n");
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}
	
	k_sleep(K_SECONDS(1));

	err = bt_le_adv_stop();
	if (err) {
		printk("Advertising failed to stop (err %d)\n", err);
		return err;
	}

	//printk("gatt end\n");
	return 0;
}