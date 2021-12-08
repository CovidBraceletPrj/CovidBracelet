#include <device.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <logging/log.h>
#include <power/reboot.h>
#include <storage/flash_map.h>
#include <string.h>
#include <zephyr.h>

#include "ens_error.h"
#include "ens_fs.h"
#include "sequencenumber.h"
#include "storage.h"

#define STORED_CONTACTS_INFO_ID 0

static struct nvs_fs info_fs;
static struct k_mutex info_fs_lock;

static ens_fs_t ens_fs;

// Information about currently stored contacts
static stored_records_information_t record_information = {.oldest_contact = 0, .count = 0};

inline storage_id_t convert_sn_to_storage_id(record_sequence_number_t sn) {
    return (storage_id_t)(sn % CONFIG_ENS_MAX_CONTACTS);
}

/**
 * Load our initial storage information from flash.
 */
int load_storage_information() {
    k_mutex_lock(&info_fs_lock, K_FOREVER);
    size_t size = sizeof(record_information);
    int rc = nvs_read(&info_fs, STORED_CONTACTS_INFO_ID, &record_information, size);

    // Check, if read what we wanted
    if (rc != size) {
        // Write our initial data to storage
        int rc = nvs_write(&info_fs, STORED_CONTACTS_INFO_ID, &record_information, size);
        if (rc <= 0) {
            return rc;
        }
    }
    k_mutex_unlock(&info_fs_lock);
    return 0;
}

/**
 * Save our current storage infromation to flash.
 */
int save_storage_information() {
    k_mutex_lock(&info_fs_lock, K_FOREVER);
    int rc = nvs_write(&info_fs, STORED_CONTACTS_INFO_ID, &record_information, sizeof(record_information));
    if (rc <= 0) {
        printk("Something went wrong after saving storage information.\n");
    }
    k_mutex_unlock(&info_fs_lock);
    return rc;
}

int init_record_storage(bool clean) {
    int rc = 0;
    struct flash_pages_info info;
    // define the nvs file system
    info_fs.offset = FLASH_AREA_OFFSET(storage);
    rc =
        flash_get_page_info_by_offs(device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL), info_fs.offset, &info);

    if (rc) {
        // Error during retrieval of page information
        printk("Cannot retrieve page information (err %d)\n", rc);
        return rc;
    }
    info_fs.sector_size = info.size;
    info_fs.sector_count = FLASH_AREA_SIZE(storage) / info.size;

    rc = nvs_init(&info_fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
    k_mutex_init(&info_fs_lock);
    if (rc) {
        // Error during nvs_init
        printk("Cannot init NVS (err %d)\n", rc);
        return rc;
    }

    // Load the current storage information
    if (clean) {
        rc = save_storage_information();
        if (rc < 0) {
            printk("Clean init of storage failed (err %d)\n", rc);
            return rc;
        }
    } else {
        rc = load_storage_information();
        if (rc < 0) {
            printk("Cannot load storage information (err %d)\n", rc);
            return rc;
        }
    }

    printk("Currently %d contacts stored!\n", record_information.count);
    printk("Space available: %d\n", FLASH_AREA_SIZE(storage));

    rc = ens_fs_init(&ens_fs, FLASH_AREA_ID(ens_storage), sizeof(record_t));
    if (rc) {
        printk("Cannot init ens_fs (err %d)\n", rc);
    }
    return rc;
}

int reset_record_storage() {
    k_mutex_lock(&info_fs_lock, K_FOREVER);
    record_information.count = 0;
    record_information.oldest_contact = 0;
    save_storage_information();
    k_mutex_unlock(&info_fs_lock);
}

int load_record(record_t* dest, record_sequence_number_t sn) {
    storage_id_t id = convert_sn_to_storage_id(sn);
    int rc = ens_fs_read(&ens_fs, id, dest);
    if (rc < 0) {
        return rc;
    }
    return 0;
}

int add_record(record_t* src) {
    /**
     * Some information about the procedure in this function:
     *      1. we calculate the potential next sn and storage id
     *      2. we try to write our entry
     *          2.1 if write was successful, goto 5., otherwise continue at 3.
     *      3. if our id is already in use, we request the fs to make some space
     *      4. after making space, we adjust our storage information and try to write again
     *      5. we actually "increment" our stored contact information
     *
     * This order (first erase storage, then increment information) is important, because like this we keep a constant
     * state of our information about the stored contacts in combination with correct state of our flash.
     */

    record_t rec;
    memcpy(&rec, src, sizeof(rec));
    k_mutex_lock(&info_fs_lock, K_FOREVER);

    // Check, if next sn would be at start of page
    rec.sn =
        get_latest_sequence_number() == get_oldest_sequence_number() ? 0 : sn_increment(get_latest_sequence_number());
    storage_id_t potential_next_id = convert_sn_to_storage_id(rec.sn);
    // write our entry to flash and check, if the current entry is already in use
    int rc = ens_fs_write(&ens_fs, potential_next_id, &rec);
    // if our error does NOT indicate, that this address is already in use, we just goto end and do nothing
    if (rc && rc != -ENS_ADDRINU) {
        // TODO: maybe also increment, if there is an internal error?
        goto end;
    } else if (rc == -ENS_ADDRINU) {
        // the current id is already in use, so make some space for our new entry
        int deletedRecordsCount = ens_fs_make_space(&ens_fs, potential_next_id);
        if (deletedRecordsCount < 0) {
            // some interal error happened (e.g. we are not at a page start)
            rc = deletedRecordsCount;
            // we still need to increment our information, so we are not at the exact same id the entire time
            goto inc;
        } else if (deletedRecordsCount > 0 && get_num_records() == CONFIG_ENS_MAX_CONTACTS) {
            record_information.count -= deletedRecordsCount;
            record_information.oldest_contact = sn_increment_by(record_information.oldest_contact, deletedRecordsCount);
        }
        // after creating some space, try to write again
        rc = ens_fs_write(&ens_fs, potential_next_id, &rec);
        if (rc) {
            goto inc;
        }
    }

inc:
    // check, how we need to update our storage information
    if (record_information.count >= CONFIG_ENS_MAX_CONTACTS) {
        record_information.oldest_contact = sn_increment(record_information.oldest_contact);
    } else {
        record_information.count++;
    }
    save_storage_information();

end:
    k_mutex_unlock(&info_fs_lock);
    return rc;
}

int delete_record(record_sequence_number_t sn) {
    storage_id_t id = convert_sn_to_storage_id(sn);
    int rc = ens_fs_delete(&ens_fs, id);
    if (!rc) {
        k_mutex_lock(&info_fs_lock, K_FOREVER);
        if (sn_equal(sn, get_oldest_sequence_number())) {
            record_information.oldest_contact = sn_increment(record_information.oldest_contact);
            record_information.count--;
        }
        save_storage_information();
        k_mutex_unlock(&info_fs_lock);
    }
    return rc;
}

record_sequence_number_t get_latest_sequence_number() {
    return sn_increment_by(record_information.oldest_contact, record_information.count);
}

record_sequence_number_t get_oldest_sequence_number() {
    return record_information.oldest_contact;
}

int get_sequence_number_interval(record_sequence_number_t* oldest, record_sequence_number_t* latest) {
    int ret = -1;
    // we lock so that the interval is always valid (e.g. not overlapping)
    k_mutex_lock(&info_fs_lock, K_FOREVER);
    if (record_information.count > 0) {
        if (oldest) {
            *oldest = record_information.oldest_contact;
        }

        if (latest) {
            *latest = sn_increment_by(record_information.oldest_contact, record_information.count);
        }

        ret = 0;
    }
    k_mutex_unlock(&info_fs_lock);
    return ret;
}

uint32_t get_num_records() {
    return record_information.count;
}
