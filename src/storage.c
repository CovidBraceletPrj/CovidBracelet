#include <device.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <power/reboot.h>
#include <storage/flash_map.h>
#include <string.h>
#include <zephyr.h>

#include "sequencenumber.h"
#include "storage.h"


// Get external flash device
#if (CONFIG_SPI_NOR - 0) ||				\
	DT_NODE_HAS_STATUS(DT_INST(0, jedec_spi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, jedec_spi_nor))
#define FLASH_NAME "JEDEC SPI-NOR"
#elif (CONFIG_NORDIC_QSPI_NOR - 0) || \
	DT_NODE_HAS_STATUS(DT_INST(0, nordic_qspi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, nordic_qspi_nor))
#define FLASH_NAME "JEDEC QSPI-NOR"
#else
#error Unsupported flash driver
#endif

// Maybe use this as param for init function
#define SEC_COUNT 8U

#define STORED_CONTACTS_INFO_ID 0
#define CONTACTS_OFFSET 1
#define MAX_CONTACTS 65535

static struct nvs_fs fs;

// Information about currently stored contacts
static stored_contacts_information_t contact_information = {.oldest_contact = 0, .count = 0};

inline storage_id_t convert_sn_to_storage_id(record_sequence_number_t sn) {
    return (storage_id_t)(sn % MAX_CONTACTS) + CONTACTS_OFFSET;
}

/**
 * Load our initial storage information from flash.
 */
int load_storage_information() {
    size_t size = sizeof(contact_information);
    int rc = nvs_read(&fs, STORED_CONTACTS_INFO_ID, &contact_information, size);

    // Check, if read what we wanted
    if (rc != size) {
        // Write our initial data to storage
        rc = nvs_write(&fs, STORED_CONTACTS_INFO_ID, &contact_information, size);
        if (rc <= 0) {
            return rc;
        }
    }
    return 0;
}

/**
 * Save our current storage infromation to flash.
 */
int save_storage_information() {
    int rc = nvs_write(&fs, STORED_CONTACTS_INFO_ID, &contact_information, sizeof(contact_information));
    if (rc <= 0) {
        printk("Something went wrong after saving storage information.\n");
    }
}

void increment_stored_contact_counter() {
    if (contact_information.count >= MAX_CONTACTS) {
        contact_information.oldest_contact = sn_increment(contact_information.oldest_contact);
    } else {
        contact_information.count++;
    }
    save_storage_information();
}

int init_contact_storage(void) {
    int rc = 0;
    struct flash_pages_info info;
    // define the nvs file system
    fs.offset = FLASH_AREA_OFFSET(storage);
    rc = flash_get_page_info_by_offs(device_get_binding(FLASH_DEVICE), fs.offset, &info);

    if (rc) {
        // Error during retrieval of page information
        return rc;
    }
    fs.sector_size = info.size;
    fs.sector_count = SEC_COUNT;

    rc = nvs_init(&fs, FLASH_DEVICE);
    if (rc) {
        // Error during nvs_init
        return rc;
    }

    // Load the current storage information
    rc = load_storage_information();

    printk("Currently %d contacts stored!\n", contact_information.count);
    printk("Space available: %d\n", FLASH_AREA_SIZE(storage));

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
    record_sequence_number_t curr_sn = sn_increment(get_latest_sequence_number());
    storage_id_t id = convert_sn_to_storage_id(curr_sn);

    int rc = nvs_write(&fs, id, src, sizeof(*src));
    if (rc > 0) {
        increment_stored_contact_counter();
        return 0;
    }
    return rc;
}

// TODO handle start and end
int delete_contact(record_sequence_number_t sn) {
    storage_id_t id = convert_sn_to_storage_id(sn);
    int rc = nvs_delete(&fs, id);
    if (!rc) {
        // TODO lome: what happens, if contacts in the middle are deleted?
        // Magic
        contact_information.count--;
        if (sn_equal(sn, get_oldest_sequence_number())) {
            contact_information.oldest_contact = sn_increment(contact_information.oldest_contact);
        }
        save_storage_information();
    }
    return rc;
}

record_sequence_number_t get_latest_sequence_number() {
    return sn_mask(contact_information.oldest_contact + contact_information.count);
}

record_sequence_number_t get_oldest_sequence_number() {
    return contact_information.oldest_contact;
}

uint32_t get_num_contacts() {
    return contact_information.count;
}
