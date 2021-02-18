#include <device.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <power/reboot.h>
#include <storage/flash_map.h>
#include <string.h>
#include <zephyr.h>

#include "storage.h"

// Maybe use this as param for init function
#define SEC_COUNT 10U

#define STORED_CONTACTS_INFO_ID 0
#define CONTACTS_OFFSET 1
#define MAX_CONTACTS 65535

static struct nvs_fs fs;

// TODO lome: load this from flash
static stored_contacts_information_t contact_information;

inline storage_id_t convert_sn_to_storage_id(record_sequence_number_t sn) {
    return (storage_id_t)(sn % MAX_CONTACTS) + CONTACTS_OFFSET;
}

void increase_sn_counter() {
    contact_information.latest_contact++;
    if (contact_information.latest_contact - contact_information.oldest_contact >= MAX_CONTACTS ) {
        contact_information.oldest_contact++;
    }
}

int init_contact_storage(void) {
    int rc = 0;
    struct flash_pages_info info;
    // define the nvs file system
    fs.offset = FLASH_AREA_OFFSET(storage);
    rc = flash_get_page_info_by_offs(device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL), fs.offset, &info);

    if (rc) {
        return rc;
    }
    fs.sector_size = info.size * 4;
    fs.sector_count = SEC_COUNT;

    rc = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
    return rc;
}

int has_contact(record_sequence_number_t sn) {
    // Treat case of wrap-around and non-wrap-around different
    if (contact_information.oldest_contact <= contact_information.latest_contact) {
        return sn >= contact_information.oldest_contact && sn <= contact_information.latest_contact;
    } else {
        return sn <= contact_information.oldest_contact || sn >= contact_information.latest_contact;
    }
}

int load_contact(contact_t* dest, record_sequence_number_t sn) {
    storage_id_t id = convert_sn_to_storage_id(sn);
    int rc = nvs_read(&fs, id, dest, sizeof(*dest));
    if(rc <= 0) {
        return rc;
    }
    return 0;
}

int add_contact(contact_t* src) {
    record_sequence_number_t curr_sn = get_latest_sequence_number() + 1;
    storage_id_t id = convert_sn_to_storage_id(curr_sn);

    contact_t test;
    nvs_read(&fs, id, &test, sizeof(test));
    int rc = nvs_write(&fs, id, src, sizeof(*src));
    if (rc > 0) {
        increase_sn_counter();
        return 0;
    }
    return rc;
}

int delete_contact(record_sequence_number_t sn) {
    storage_id_t id = convert_sn_to_storage_id(sn);
    return nvs_delete(&fs, id);
}

record_sequence_number_t get_latest_sequence_number() {
    return contact_information.latest_contact;
}

record_sequence_number_t get_oldest_sequence_number() {
    return contact_information.oldest_contact;
}

uint32_t get_num_contacts() {
    return get_latest_sequence_number() - get_oldest_sequence_number();
}