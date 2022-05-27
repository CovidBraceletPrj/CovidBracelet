#include <device.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <logging/log.h>
#include <power/reboot.h>
#include <storage/flash_map.h>
#include <string.h>
#include <zephyr.h>
#include <sys/types.h>

#include "utility/ens_fs.h"
#include "record_storage.h"

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

int record_storage_init(bool clean) {
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

void reset_record_storage() {
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





int ens_records_iterator_init_range(record_iterator_t* iterator,
                                    record_sequence_number_t* opt_start,
                                    record_sequence_number_t* opt_end) {
    // prevent any changes during initialization
    int rc = get_sequence_number_interval(&iterator->sn_next, &iterator->sn_end);
    if (rc == 0) {
        iterator->finished = false;

        // we override start and end with the optional values
        if (opt_start) {
            iterator->sn_next = *opt_start;
        }
        if (opt_end) {
            iterator->sn_end = *opt_end;
        }
    } else {
        iterator->finished = true;
    }

    return 0;
}

int64_t get_timestamp_for_sn(record_sequence_number_t sn) {
    record_t rec;
    if (load_record(&rec, sn) == 0) {
        return rec.timestamp;
    } else {
        return -1;
    }
}

enum record_timestamp_search_mode {
    RECORD_TIMESTAMP_SEARCH_MODE_MIN,
    RECORD_TIMESTAMP_SEARCH_MODE_MAX,
};

/**
 * Find an entry via binary search for the timestamp.
 *
 * @param record pointer to the location, where the found sn shall be stored
 * @param target timestamp for which to find the nearest entry for
 * @param greater flag for indicating, if the loaded sn shall correspond to a greater (1) or smaller (0) timestamp
 */
int find_sn_via_binary_search(record_sequence_number_t* sn_dest,
                              uint32_t target,
                              enum record_timestamp_search_mode search_mode) {
    record_sequence_number_t start_sn;
    record_sequence_number_t end_sn;

    // prevent any changes during binary search initialization

    int rc = get_sequence_number_interval(&start_sn, &end_sn);

    if (rc) {
        return rc;
    }

    record_sequence_number_t last_sn =
            start_sn;  // used to check if ran into issues, e.g. could not load the entry or rounding errors

    while (!sn_equal(start_sn, end_sn)) {
        // calculate the sn in the middle between start and end
        record_sequence_number_t cur_sn = sn_get_middle_sn(start_sn, end_sn);

        if (sn_equal(cur_sn, last_sn)) {
            // if we already checked this entry -> we reduce our boundaries and try again
            // this also solves issues with rounding
            // TODO: This is not the best way...
            if (search_mode == RECORD_TIMESTAMP_SEARCH_MODE_MIN) {
                int64_t start_ts = get_timestamp_for_sn(start_sn);
                if (start_ts == -1 || start_ts < target) {
                    // we could not load this entry or this entry is strictly smaller than our target
                    start_sn = sn_increment(start_sn);  // we can safely increment as start_sn < end_sn
                } else {
                    // we actually found the wanted entry!
                    end_sn = start_sn;  // this will break our loop
                }
            } else {
                // we search for the biggest value among them
                int64_t end_ts = get_timestamp_for_sn(end_sn);
                if (end_ts == -1 || end_ts > target) {
                    // we could not load this entry or this entry is strictly bigger than our target
                    end_sn = sn_decrement(end_sn);  // we can safely decrement as start_sn < end_sn
                } else {
                    // we actually found the wanted entry!
                    start_sn = end_sn;  // this will break our loop
                }
            }
        } else {
            int64_t mid_ts = get_timestamp_for_sn(cur_sn);

            if (mid_ts >= 0) {
                if (target < mid_ts) {
                    end_sn = cur_sn;
                } else if (target > mid_ts) {
                    start_sn = cur_sn;
                } else {
                    // target == mid_ts
                    if (search_mode == RECORD_TIMESTAMP_SEARCH_MODE_MIN) {
                        // we search for the smallest value among them -> look before this item
                        end_sn = cur_sn;
                    } else {
                        // we search for the biggest value among them -> look after this item
                        start_sn = cur_sn;
                    }
                }
            } else {
                // some errors -> we keep the current sn and try to narrow our boundaries
            }
        }
        last_sn = cur_sn;
    }

    *sn_dest = start_sn;  // == end_sn

    return 0;
}

// TODO: This iterator does neither check if the sequence numbers wrapped around while iteration. As a result, first
// results could have later timestamps than following entries
int ens_records_iterator_init_timerange(record_iterator_t* iterator, time_t* ts_start, time_t* ts_end) {
    record_sequence_number_t oldest_sn = 0;
    record_sequence_number_t newest_sn = 0;

    // assure that *ts_end > *ts_start
    if (ts_start && ts_end && *ts_end < *ts_start) {
        return 1;
    }

    if (ts_start) {
        int rc = find_sn_via_binary_search(&oldest_sn, *ts_start, RECORD_TIMESTAMP_SEARCH_MODE_MIN);
        if (rc) {
            return rc;
        }
    } else {
        oldest_sn = get_oldest_sequence_number();
    }

    if (ts_end) {
        int rc = find_sn_via_binary_search(&newest_sn, *ts_end, RECORD_TIMESTAMP_SEARCH_MODE_MAX);
        if (rc) {
            return rc;
        }
    } else {
        newest_sn = get_latest_sequence_number();
    }

    return ens_records_iterator_init_range(iterator, &oldest_sn, &newest_sn);
}

record_t* ens_records_iterator_next(record_iterator_t* iter) {
    record_t* next = NULL;

    while (next == NULL && !iter->finished) {
        record_t contact;
        // try to load the next contact
        int res = load_record(&contact, iter->sn_next);

        if (!res) {
            next = &iter->current;
            memcpy(next, &contact, sizeof(record_t));
        }

        if (sn_equal(iter->sn_next, iter->sn_end)) {
            iter->finished = true;  // this iterator will finish after this execution
        } else {
            // increase the current sn
            iter->sn_next = sn_increment(iter->sn_next);
        }
    }

    return next;
}

int ens_record_iterator_clear(record_iterator_t* iter) {
    // clear all relevant fields in the iterator
    iter->finished = true;
    iter->sn_next = 0;
    iter->sn_end = 0;
    memset(&iter->current, 0, sizeof(iter->current));
    return 0;
}

uint8_t ens_records_iterate_with_callback(record_iterator_t* iter, ens_record_iterator_cb_t cb, void* userdata) {
    record_t* cur = ens_records_iterator_next(iter);
    bool cont = true;

    while (cur != NULL && cont) {
        int cb_res = cb(cur, userdata);
        if (cb_res == ENS_RECORD_ITER_STOP) {
            cont = false;
        }
    }

    if (cont) {
        cb(NULL, userdata);  // we call the callback one last time but with null data
    }
    return 0;
}