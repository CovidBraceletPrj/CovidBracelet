#include <device.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <power/reboot.h>
#include <storage/flash_map.h>
#include <string.h>
#include <zephyr.h>

#include "storage.h"

// Maybe use this as param for init function
#define SEC_COUNT 8U

#define STORED_CONTACTS_INFO_ID 0
#define CONTACTS_OFFSET 1
#define MAX_CONTACTS 65535
#define ADDRESS_ID 1

static struct nvs_fs fs;

// TODO lome: load this from flash
static stored_contacts_information_t contact_information;

inline storage_id_t convert_sn_to_storage_id(record_sequence_number_t sn) {
    return (storage_id_t)(sn % MAX_CONTACTS) + CONTACTS_OFFSET;
}

void increment_storaed_contact_counter() {
    if (contact_information.count >= MAX_CONTACTS) {
        contact_information.oldest_contact = sequence_number_increment(contact_information.oldest_contact);
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
    fs.sector_size = info.size;
    fs.sector_count = SEC_COUNT;

    rc = nvs_init(&fs, DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
    
    return rc;
}

int load_contact(contact_t* dest, record_sequence_number_t sn) {
    storage_id_t id = convert_sn_to_storage_id(sn);
    int rc = nvs_read(&fs, id, dest, sizeof(*dest));
    if (rc <= 0) {
        return rc;
    }
    return 0;
}

int add_contact(contact_t* src) {
    record_sequence_number_t curr_sn = get_latest_sequence_number() + 1;
    storage_id_t id = convert_sn_to_storage_id(curr_sn);

    int rc = nvs_write(&fs, id, src, sizeof(*src));
    if (rc > 0) {
        increment_storaed_contact_counter();
        return 0;
    }
    return rc;
}

// TODO handle start and end
int delete_contact(record_sequence_number_t sn) {
    storage_id_t id = convert_sn_to_storage_id(sn);
    return nvs_delete(&fs, id);
}

record_sequence_number_t get_latest_sequence_number() {
    return contact_information.oldest_contact + contact_information.count;
}

record_sequence_number_t get_oldest_sequence_number() {
    return contact_information.oldest_contact;
}

uint32_t get_num_contacts() {
    return contact_information.count;
}
