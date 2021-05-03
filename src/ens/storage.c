#include <device.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <logging/log.h>
#include <power/reboot.h>
#include <storage/flash_map.h>
#include <string.h>
#include <zephyr.h>

#include "ens_fs.h"
#include "sequencenumber.h"
#include "storage.h"

// Maybe use this as param for init function
#define SEC_COUNT 8U

#define STORED_CONTACTS_INFO_ID 0
#define MAX_CONTACTS 65536

static struct nvs_fs info_fs;
static struct k_mutex info_fs_lock;

static ens_fs_t ens_fs;

// Information about currently stored contacts
static stored_records_information_t record_information = {.oldest_contact = 0, .count = 0};

inline storage_id_t convert_sn_to_storage_id(record_sequence_number_t sn) {
    return (storage_id_t)(sn % MAX_CONTACTS);
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
        rc = nvs_write(&info_fs, STORED_CONTACTS_INFO_ID, &record_information, size);
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

record_sequence_number_t get_next_sequence_number() {
    k_mutex_lock(&info_fs_lock, K_FOREVER);
    if (record_information.count >= MAX_CONTACTS) {
        record_information.oldest_contact = sn_increment(record_information.oldest_contact);
    } else {
        record_information.count++;
    }
    save_storage_information();
    record_sequence_number_t next_sn = get_latest_sequence_number();
    k_mutex_unlock(&info_fs_lock);
    return next_sn;
}

int init_record_storage(void) {
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
    info_fs.sector_count = SEC_COUNT;

    rc = nvs_init(&info_fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
    k_mutex_init(&info_fs_lock);
    if (rc) {
        // Error during nvs_init
        printk("Cannot init NVS (err %d)\n", rc);
        return rc;
    }

    // Load the current storage information
    rc = load_storage_information();
    if (rc) {
        printk("Cannot load storage information (err %d)\n", rc);
        return rc;
    }

    printk("Currently %d contacts stored!\n", record_information.count);
    printk("Space available: %d\n", FLASH_AREA_SIZE(storage));

    rc = ens_fs_init(&ens_fs, FLASH_AREA_ID(ens_storage), sizeof(record_t));
    if (rc) {
        printk("Cannot init ens_fs (err %d)\n", rc);
    }
    return rc;
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
    int sectorsToDelete = 1;

    k_mutex_lock(&info_fs_lock, K_FOREVER);

    // Check, if next sn would be at start of page
    record_sequence_number_t potential_next_sn = sn_increment(get_latest_sequence_number());
    storage_id_t potential_next_id = convert_sn_to_storage_id(potential_next_sn);
    if (((potential_next_id * ens_fs.entry_size) % ens_fs.sector_size) == 0) {
        // If we are at start of a page, we need to erase it first
        ens_fs_page_erase(&ens_fs, potential_next_id, sectorsToDelete);

        // if our storage is full, we need to adjust our information about stored records
        if (get_num_records() == MAX_CONTACTS) {
            int deletedRecordsCount = (ens_fs.sector_size / ens_fs.entry_size) * sectorsToDelete;
            record_information.count -= deletedRecordsCount;
            record_information.oldest_contact =
                GET_MASKED_SN((record_information.oldest_contact + deletedRecordsCount));
            save_storage_information();
        }
    }

    // Actually increment sn
    record_sequence_number_t curr_sn = get_next_sequence_number();
    src->sn = curr_sn;
    storage_id_t id = convert_sn_to_storage_id(curr_sn);

    k_mutex_unlock(&info_fs_lock);
    return ens_fs_write(&ens_fs, id, src);
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

// TODO lome: do we need lock here aswell?
record_sequence_number_t get_latest_sequence_number() {
    return GET_MASKED_SN((record_information.oldest_contact + record_information.count));
}

record_sequence_number_t get_oldest_sequence_number() {
    return record_information.oldest_contact;
}

uint32_t get_num_records() {
    return record_information.count;
}
