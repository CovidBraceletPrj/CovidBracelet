/* 
 * Copyright (c) 2020 Max Kasperowski
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#define BT_GATT_DIS_MODEL "BlueCovid"
#define BT_GATT_DIS_MANUF ""
/*#define CONFIG_BT_GATT_DIS_SERIAL_NUMBER
#define BT_GATT_DIS_SERIAL_NUMBER_STR "TODO - serial number"
#define CONFIG_BT_GATT_DIS_FW_REV
#define BT_GATT_DIS_FW_REV_STR "TODO - firmware revision number"
#define CONFIG_BT_GATT_DIS_HW_REV
#define BT_GATT_DIS_HW_REV_STR "TODO - hardware revision"
#define CONFIG_BT_GATT_DIS_SW_REV
#define BT_GATT_DIS_SW_REV "TODO - software revision"
*/
static ssize_t read_str(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                        void *buf, u16_t len, u16_t offset)
{
  return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, 
                            strlen(attr->user_data));
}
BT_GATT_SERVICE_DEFINE(dis_svc,
  BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
  BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER,
                          BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
                          read_str, NULL, BT_GATT_DIS_MODEL),
  BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
                          BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
                          read_str, NULL, BT_GATT_DIS_MANUF));