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

//TODO: change device name in project conf, set id dynamicaally, so that not all devices have the same name...

// 0651C79E-ADA7-42EA-986A-9F69790D11F2
static struct bt_uuid_128 covid_service_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0xF2);

static struct bt_uuid_128 next_key_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0xF3);

static struct bt_uuid_128 new_key_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0xF4);

static struct bt_uuid_128 infected_key_cnt_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0xF5);
static struct bt_uuid_128 infected_key_0_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x00);
static struct bt_uuid_128 infected_key_1_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x01);
static struct bt_uuid_128 infected_key_2_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x02);
static struct bt_uuid_128 infected_key_3_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x03);
static struct bt_uuid_128 infected_key_4_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x04);
static struct bt_uuid_128 infected_key_5_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x05);
static struct bt_uuid_128 infected_key_6_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x06);
static struct bt_uuid_128 infected_key_7_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x07);
static struct bt_uuid_128 infected_key_8_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x08);
static struct bt_uuid_128 infected_key_9_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x09);
static struct bt_uuid_128 infected_key_10_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x0A);
static struct bt_uuid_128 infected_key_11_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x0B);
static struct bt_uuid_128 infected_key_12_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x0C);
static struct bt_uuid_128 infected_key_13_uuid = BT_UUID_INIT_128(
	0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
	0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0x0D);



#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	/* My Service UUID (same as above) */
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		0x06, 0x51, 0xC7, 0x9E, 0xAD, 0xA7, 0x42, 0xEA, 
		0x98, 0x6A, 0x9F, 0x69, 0x79, 0x0D, 0x11, 0xF2),
};

static ssize_t read_next_key(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	u32_t id = get_next_infected_key_id();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &id, sizeof(id));
}

static ssize_t read_key_cnt(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	unsigned int cnt = get_period_cnt_if_infected();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &cnt, sizeof(unsigned int));
}
static ssize_t read_key_0(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(0, &size);
	// printk("read key 0, size %u, pointer %p\n", size, p);
	// printk("Interval: %u\n", p->periodInterval);
	// printk("RPI: "); print_rpi((rolling_proximity_identifier_t*)&p->periodKey); printk("\n");
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_1(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(1, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_2(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(2, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_3(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(3, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_4(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(4, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_5(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(5, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_6(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(6, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_7(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(7, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_8(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(8, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_9(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(9, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, p, size);
}
static ssize_t read_key_10(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(10, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &p, size);
}
static ssize_t read_key_11(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(11, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &p, size);
}
static ssize_t read_key_12(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(12, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &p, size);
}
static ssize_t read_key_13(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, u16_t len, u16_t offset){
	size_t size = 0;
	period_t* p = get_period_if_infected(13, &size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &p, size);
}

//TODO: copy more than one key
static ssize_t write_new_key(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, u16_t len, u16_t offset,
			      u8_t flags)
{
	//printk("write key\n");
	if( offset != 0 ){
		printk("error: invalid offset %u\n", offset);
		return len;
	}
	if( len != sizeof(period_t) ){
		printk("error: invalid length %u (expected %u)\n", len, sizeof(period_t));
		return len;
	}
	period_t* period = (period_t*)buf;
	add_infected_key(period);
	return len;
}

static void connected(struct bt_conn *conn, u8_t err)
{

	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		//printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	//printk("Disconnected (reason 0x%02x)\n", reason);
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

BT_GATT_SERVICE_DEFINE(covid_svc,

	BT_GATT_PRIMARY_SERVICE(&covid_service_uuid),
	//TODO: switch on crypto and switch to signed keys
	BT_GATT_CHARACTERISTIC(&next_key_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_next_key, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&new_key_uuid.uuid, BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE, NULL, write_new_key, NULL),
	
	BT_GATT_CHARACTERISTIC(&infected_key_cnt_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_cnt, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_0_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_0, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_1_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_1, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_2_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_2, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_3_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_3, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_4_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_4, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_5_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_5, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_6_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_6, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_7_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_7, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_8_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_8, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_9_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_9, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_10_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_10, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_11_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_11, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_12_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_12, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&infected_key_13_uuid.uuid, BT_GATT_CHRC_READ, 
		BT_GATT_PERM_READ, read_key_13, NULL, NULL),
);
