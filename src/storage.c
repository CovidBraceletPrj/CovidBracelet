#include <device.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <power/reboot.h>
#include <storage/flash_map.h>
#include <string.h>
#include <zephyr.h>

#include "storage.h"

// Maybe use this as param for init function
#define SEC_COUNT 10
#define PERIOD_COUNTER_ID 0
#define PERIOD_OFFSET 1

static struct nvs_fs fs;

int init_storage(void) {
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
    if (rc) {
        return rc;
    }
    return 0;
}

uint16_t get_current_period_nr() {
    uint16_t period_nr;
    int rc = nvs_read(&fs, PERIOD_COUNTER_ID, &period_nr, sizeof(period_nr));
    // current period not found, store 0
    if (rc <= 0) {
        period_nr = 0;
        nvs_write(&fs, PERIOD_COUNTER_ID, &period_nr, sizeof(period_nr));
    }
    return period_nr;
}

int store_period_contacts(period_contacts_t* contacts) {
    uint16_t period = get_current_period_nr();
    int rc = nvs_write(&fs, period * 2 + PERIOD_OFFSET, contacts->cnt, sizeof(contacts->cnt));
    if (rc != sizeof(contacts->cnt)) {
        return rc;
    }
    rc = nvs_write(&fs, period * 2 + 1 + PERIOD_OFFSET, contacts->period_contacts, sizeof(period_contact_t) * contacts->cnt);
    return rc;
}