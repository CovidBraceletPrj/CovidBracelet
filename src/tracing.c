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
#include <kernel.h>

#include "exposure-notification.h"
#include "tracing.h"
#include "record_storage.h"
#include "tek_storage.h"

#include "utility/util.h"

#define COVID_ENS (0xFD6F)

typedef ENIntervalIdentifier ENIntervalIdentifier;

#define RPI_ROTATION_MS_MIN (500*1000)
#define RPI_ROTATION_MS_MAX (1250*1000)
#define RPI_ROTATION_MS (600*1000)
#define SCAN_INTERVAL_MS (5*60*1000)
#define SCAN_DURATION_MS 2000
#define ADV_INTERVAL_MS 250


K_TIMER_DEFINE(rpi_timer, NULL, NULL);
K_TIMER_DEFINE(scan_timer, NULL, NULL);

static int on_rpi();
static int on_scan();

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_vs.h>

#include <sys/byteorder.h>

#define DEVICE_BEACON_TXPOWER_NUM  6
static const int8_t txp[DEVICE_BEACON_TXPOWER_NUM] = {0, -4, -8,
                                                      -16, -20, -40};

static void set_tx_power(int8_t tx_pwr_lvl)
{
    struct bt_hci_cp_vs_write_tx_power_level *cp;
    struct bt_hci_rp_vs_write_tx_power_level *rp;
    struct net_buf *buf, *rsp = NULL;
    int err;

    buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
                            sizeof(*cp));
    if (!buf) {
        printk("Unable to allocate command buffer\n");
        return;
    }

    cp = net_buf_add(buf, sizeof(*cp));
    cp->handle = sys_cpu_to_le16(0);
    cp->handle_type = BT_HCI_VS_LL_HANDLE_TYPE_ADV;
    cp->tx_power_level = tx_pwr_lvl;

    err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
                               buf, &rsp);
    if (err) {
        uint8_t reason = rsp ?
                         ((struct bt_hci_rp_vs_write_tx_power_level *)
                                 rsp->data)->status : 0;
        printk("Set Tx power err: %d reason 0x%02x\n", err, reason);
        return;
    }

    rp = (void *)rsp->data;
    printk("Actual Tx Power: %d\n", rp->selected_tx_power);

    net_buf_unref(rsp);
}


static int cur_tx_pwr = 0;



typedef struct period{
    ENPeriodKey periodKey;
    ENIntervalNumber periodInterval;
} __packed period_t;


typedef struct covid_adv_svd
{
    uint16_t ens;
    ENIntervalIdentifier rolling_proximity_identifier;
    associated_encrypted_metadata_t associated_encrypted_metadata;
} __packed covid_adv_svd_t;

const static bt_metadata_t bt_metadata = {
        .version = 0b00100000,
        .tx_power = 0, //TODO set to actual transmit power
        .rsv1 = 0,
        .rsv2 = 0,
};

static covid_adv_svd_t covid_adv_svd = {
        .ens = COVID_ENS,
        //do not initialiuze the rest of the packet, will write this later
};




static struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x6f, 0xfd), //0xFD6F Exposure Notification Service
        BT_DATA(BT_DATA_SVC_DATA16, &covid_adv_svd, sizeof(covid_adv_svd_t))};

int adv_start() {

   return bt_le_adv_start(BT_LE_ADV_PARAM(0, (ADV_INTERVAL_MS-10)/0.625, (ADV_INTERVAL_MS+10)/0.625, NULL), ad, ARRAY_SIZE(ad), NULL, 0);
}

int adv_stop() {
    return bt_le_adv_stop();
}

int tracing_init()
{
    // We init the timers (which should run periodically!)
    k_timer_start(&rpi_timer, K_NO_WAIT, K_MSEC(RPI_ROTATION_MS)); // we directly init the rpi timer, to be sure that this is triggered at the beginning
    k_timer_start(&scan_timer, K_MSEC(SCAN_INTERVAL_MS), K_MSEC(SCAN_INTERVAL_MS));


    int err = 0;
    err = adv_start();
    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return err;
    }


    return 0;
}

uint32_t tracing_run()
{
    if (k_timer_status_get(&rpi_timer) > 0) {
        int err = adv_stop();

        if (err)
        {
            printk("Advertising failed to stop (err %d)\n", err);
        }

        on_rpi();

        // TODO: Enable power randomization!
        //cur_tx_pwr = (cur_tx_pwr +1) % DEVICE_BEACON_TXPOWER_NUM;
        //set_tx_power(txp[cur_tx_pwr]);

        adv_start();
    }

    if (k_timer_status_get(&scan_timer) > 0) {
        int err = adv_stop();

        if (err)
        {
            printk("Advertising failed to stop (err %d)\n", err);
        }

        on_scan();

        adv_start();
    }

    // we return the minimum timer time so that main can sleep
    return MIN( k_timer_remaining_get(&rpi_timer), k_timer_remaining_get(&scan_timer));
}





int on_rpi() {

    printk("\n----------------------------------------\n\n");
    printk("*** New Interval\n");

    // we first get the relevant TEK
    uint32_t currentTime = time_get_unix_seconds();

    tek_t tek;
    int err = tek_storage_get_latest_at_ts(&tek, currentTime);

    if (err != 0) {
        printk("ERROR: COULD NOT DETERMINE TEK!!!\n");
        return err;
    }

    ENIntervalNumber currentInterval = en_get_interval_number(currentTime);

    // we now generate the new interval identifier and re-encrypt the metadata
    // TODO: The period identifier key and the MetadataEncryptionKey do not need to be derived everytime!

    ENPeriodMetadataEncryptionKey periodMetadataEncryptionKey;
    ENPeriodIdentifierKey pik;
    ENIntervalIdentifier intervalIdentifier;
    en_derive_period_identifier_key(&pik, &tek.tek);
    en_derive_interval_identifier(&intervalIdentifier, &pik, currentInterval);


    associated_encrypted_metadata_t encryptedMetadata;

    en_derive_period_metadata_encryption_key(&periodMetadataEncryptionKey, &tek.tek);
    en_encrypt_interval_metadata(&periodMetadataEncryptionKey, &intervalIdentifier, (unsigned char*)&bt_metadata, (unsigned char*)&encryptedMetadata, sizeof(associated_encrypted_metadata_t));


    // broadcast intervalIdentifier plus encryptedMetada according to specs
    //printk("\n----------------------------------------\n\n");
    printk("Time: %u, ", currentTime);
    printk("Interval: %u, ", currentInterval);
    printk("TEK: ");
    print_rpi((ENIntervalIdentifier *)&tek.tek);
    printk(", ");
    printk("RPI: ");
    print_rpi((ENIntervalIdentifier *)&intervalIdentifier);
    printk(", ");
    printk("AEM: ");
    print_aem(&encryptedMetadata);
    printk("\n");

    memcpy(&covid_adv_svd.rolling_proximity_identifier, &intervalIdentifier, sizeof(ENIntervalIdentifier));
    memcpy(&covid_adv_svd.associated_encrypted_metadata, &encryptedMetadata, sizeof(associated_encrypted_metadata_t));
    return 0;
}

static const struct bt_le_scan_param scan_param = {
        .type = BT_HCI_LE_SCAN_PASSIVE,
        .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = 0x0C80, //Scan Interval (N * 0.625 ms), TODO: set to correct interval
        .window = 0x0C80,	//Scan Window (N * 0.625 ms), TODO: set to correct interval
};

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
                    //printk("Attempting to store record...\n");
                    record_t record;
                    uint32_t timestamp = time_get_unix_seconds();
                    memcpy(&record.rssi, &rssi, sizeof(record.rssi));
                    memcpy(&record.associated_encrypted_metadata, &rx_adv->associated_encrypted_metadata, sizeof(record.associated_encrypted_metadata));
                    memcpy(&record.rolling_proximity_identifier, &rx_adv->rolling_proximity_identifier, sizeof(record.rolling_proximity_identifier));
                    memcpy(&record.timestamp, &timestamp, sizeof(record.timestamp));
                    int rc = add_record(&record);
                    if (rc != 0) {
                        printk("ERROR: Storing record failed (err %d)\n", rc);
                    }
                }
            }
            net_buf_simple_pull(buf, len - 1); //consume the rest, note we already consumed one byte via net_buf_simple_pull_u8(buf)
        }
    }
}

int on_scan() {

    uint32_t num_devs = get_num_records();
    printk("Scanning for devices...\n");
    int err = 0;
    err = bt_le_scan_start(&scan_param, scan_cb);
    if (err)
    {
        printk("Starting scanning failed (err %d)\n", err);
        return err;
    }

    k_sleep(K_MSEC(SCAN_DURATION_MS)); // TODO: what to put here?

    err = bt_le_scan_stop();
    if (err)
    {
        printk("Stopping scan failed (err %d)\n", err);
        return err;
    }

    printk("Scanning done... %u devices found\n", get_num_records()-num_devs);
    return 0;
}
